#!/usr/bin/env python3

import struct;
import argparse;
import lzma;
import gzip;
import shlex;
import datetime;
import traceback;

## versioning
rrasm_name = "rrasm";
rrasm_version = "0.2"; # annoying CHANGEME

## constants

WORD_SIZE = 4; # power of 2
WORD_ADDRESSED = True; # for example: (with 4 byte words) word 2 starts at `0x00001` rather than `0x00004`
ADDRESS_SHIFT = len(bin(WORD_SIZE - 1)[2:]) if WORD_ADDRESSED else 0; # change WORD_SIZE and WORD_ADDRESSED instead

### transformer constants
FPGASYNTH_WORD_SIZE = WORD_SIZE;
MI_WORD_SIZE = WORD_SIZE;
# ELF_MACHINE_TYPE = 0x23; - lemon: since we now support multiple arches/cpus (machines), this is now compiler flag dependant
ELF_OBJ_TYPE = 0x00001; # our very recognised and stardard object type (unlinked relocatable - like nasm)
ELF_OSABI_TYPE = 0x80; # out (unrecognised, non-standard) OS/ABI type
ELF_OSABI_VERSION = 0x00; # this is osabi specific anyway
ELF_COMMENT = b"rrisc assembler (rrasm v1.1)"; # elf comment
ELF_RRISC_REL_8 = 0x00; # rrisc arch relocation types
ELF_RRISC_REL_16 = 0x01;
ELF_RRISC_REL_32 = 0x02;
ELF_RRISC_REL_64 = 0x03;
ELF_RRISC_REL_PC16 = 0x80;

## default variable values

padded_size = 0; # size to pad to (0 means don't pad, -p)
vmembase = 0x0000; # executable base (org || -b)
entrypoint_name = "_start"; # entry point (-e entry_point)
pic = True; # pic setting (generate position independent code, doesn't affect linking) (--no-pic)
pie = False; # pie setting (generate position independent executables, doesn't affect code generation) (-pie)
output_format = "bin"; # output format (-f bin)
output_filename = "a.out"; # output filename (-o file.txt)
ignored_warnings = []; # ignored warnings (-w warningname)
export_all = False; # export all symbols? (-E)
compressor = "none"; # file compressor (-z)
compression_level = 1; # compression level (-l)
target_arch="rrisc" # target architecture (-a)
target_cpu=None # target cpu (-c)
hlvi=True # enable high-level virtual instructions (--no-hlvi)

## misc
architecture = None;
cpu = None;

## preproccesor

PREPROCESSOR_PASSTHROUGH = 0;
PREPROCESSOR_IFDEF = 1;
PREPROCESSOR_DEFINE = 2;

preprocessor_state = PREPROCESSOR_PASSTHROUGH;
preprocessor_hungry = False;
preprocessor_accepted = ["ifdef", "ifndef", "define", "elifdef", "elifndef"];
preprocessor_controls = [PREPROCESSOR_IFDEF];
preprocessor_stack = [];

# error enum
ERR_DECODE_FAILED = 0;
ERR_RESOLUTION_FAILED = 1;
ERR_UNKNOWN_DECODE_ERROR = 2;
ERR_INCORRECT_ARG_COUNT = 3;
ERR_UNSUPPORTED_ARGS = 4;
ERR_SERIALISATION_FAILED = 5;
ERR_SYM_NOT_FOUND = 6;
ERR_SYM_REDEFINED = 7;
ERR_UNIMPLEMENTED = 8;

ERR_GENERIC = -0xff;

symbols = {}; # symbols - filled at runtime
symtype = {}; # symbol types - runtime variable

macros = {
	"__TIMESTAMP__": {"type": "str", "value": datetime.datetime.now().strftime("%Y/%m/%d %H:%M:%S")},
	"__TIME__": {"type": "str", "value": datetime.datetime.now().strftime("%H:%M:%S")},
	"__DATE__": {"type": "str", "value": datetime.datetime.now().strftime("%Y/%m/%d")},
	# ass name...
	"__AS_NAME__": {"type": "str", "value": rrasm_name},
	"__AS_VER__": {"type": "str", "value": rrasm_version}
};

unresolved = []; # unresolved symbols - same
exports = []; # exports - same

def int_coerce(s, signed=False, encoding="utf-8"):
	if (type(s) == int):
		return {"type": "int", "value": s};
	if (type(s) == str):
		return {"type": "int", "value": int.from_bytes(bytes(s, encoding), byteorder="big", signed=signed)};
	if (type(s) == bytes):
		return {"type": "int", "value": int.from_bytes(s, byteorder="big", signed=signed)};
	if (type(s) == dict and "type" in s):
		if (s["type"] != "str"):
			return s;
		return {"type": "int", "value": int.from_bytes(bytes(s["value"], encoding), byteorder="big", signed=signed)};
	return s;

def int_forcecast(s, signed=False):
	if (type(s) == int):
		return s;
	if (type(s) == str):
		return int.from_bytes(bytes(s, "utf-8"), byteorder="big", signed=signed);
	if (type(s) == bytes):
		return int.from_bytes(s, byteorder="big", signed=signed);
	if (type(s) == dict and "type" in s):
		if (s["type"] == "int"):
			return s["value"];
		if (s["type"] != "str"):
			raise NotImplementedError;
		return int.from_bytes(bytes(s["value"], "utf-8"), byteorder="big", signed=signed);
	raise NotImplementedError;

# print error
def print_error(warntype, text, ignoreflag=False):
	if (ignoreflag != False):
		if (ignoreflag in ignored_warnings):
			return;

		print("%s: %s [-w %s]" % (warntype, text, ignoreflag));
	else:
		print("%s: %s" % (warntype, text));

def incbin_virtual(a):
	if (a["type"] != "str"):
		return {"type": "error", "value": ERR_UNSUPPORTED_ARGS};

	try:
		f = open(a["value"], "rb");
		b = f.read();
		f.close();
		return {"type": "data", "value": b};
	except Exception as e:
		print_error("error", "Could not open file '%s'" % a["value"]);
		return {"type": "error", "value": ERR_RESOLUTION_FAILED};

# generic register, #imm virtual resolver
def r_imm_virtual_resolver(a, b, c, name):
	if (c == False):
		c = b;
		b = a;

	c = int_coerce(c);

	if (a["type"] != "reg" and b["type"] != "reg"):
		return {"type": "error", "value": ERR_UNSUPPORTED_ARGS};

	if (c["type"] == "int" or c["type"] == "sym"):
		return {"type": "instruction", "name": name, "operands": [a, b, c]};

	return {"type": "error", "value": ERR_UNSUPPORTED_ARGS};

# generic register, register || register, #imm virtual resolver, resolve to rr or imm depending on operand types
def rr_imm_virtual_resolver(a, b, c, rr, imm):
	if (c == False):
		c = b;
		b = a;

	c = int_coerce(c);

	if (a["type"] != "reg" and b["type"] != "reg"):
		return {"type": "error", "value": ERR_UNSUPPORTED_ARGS};

	if (c["type"] == "reg"):
		return {"type": "instruction", "name": rr, "operands": [a, b, c]};

	if (c["type"] == "int" or c["type"] == "sym"):
		return {"type": "instruction", "name": imm, "operands": [a, b, c]};

	return {"type": "error", "value": ERR_UNSUPPORTED_ARGS};

def negate(n, numbits=8):
    return ((1 << numbits) + n) & (1 << numbits) - 1;

def float2fix(f, bitcount):
	return int(f * (1 << bitcount));

# generic operand serialiser, serialises an operand of type to bytesize-bytes
def serialise(bytesize, operand, offset):
	value = operand["value"];

	# decimal - fixed-points
	if (operand["type"] == "decimal"):
		decimalformats = {
			1: ">b",
			2: ">h",
			4: ">i",
			8: ">q"
		};

		fixed = float2fix(operand["value"], bytesize * 4);
		return struct.pack(decimalformats[bytesize], fixed);

	# floats
	if (operand["type"] == "float"):
		if (bytesize == 1):
			print_error("error", "serialise(): Size specifier too small for float, 8-bit float not supported");
			return {"type": "error", "value": ERR_SERIALISATION_FAILED};

		floatformats = {
			2: ">e",
			4: ">f",
			8: ">d"
		};
		return struct.pack(floatformats[bytesize], operand["value"]);

	if (operand["type"] == "int" or operand["type"] == "sym"):
		if (operand["type"] == "sym"):
			reltype = { 1: "abs8", 2: "abs16", 4: "abs32", 8: "abs64" };
			if (value not in symbols or (symtype[value] != "const" and pie)):
				unresolved.append({"type": reltype[bytesize], "symname": value, "address": offset});
				return b"\x00" * bytesize;
			value = symbols[value];

		format = {
			1: "B",
			2: "H",
			4: "I",
			8: "Q",
		};
		return struct.pack(">" + format[bytesize], value);

	if (operand["type"] == "str"):
		if (bytesize == 8):
			print_error("error", "serialise(): Don't know how to serialise 8-byte per character strings...");
			return {"type": "error", "value": ERR_SERIALISATION_FAILED};

		encodings = {
			1: "utf-8",
			2: "utf-16_be",
			4: "utf-32-be"
		}
		return bytes(value, encodings[bytesize]);

	return {"type": "error", "value": ERR_SERIALISATION_FAILED};

def align_virtual(offset, alignment):
	if (alignment["type"] != "int"):
		return {"type": "error", "value": ERR_UNSUPPORTED_ARGS};

	alignment = int_coerce(alignment);
	wanted_alignment = alignment["value"];
	return {"type": "data", "value": b"\x00" * (wanted_alignment - (offset % wanted_alignment))};

# `org` virtual instruction, set vmembase to int operand
def org_virtual(origin):
	global vmembase;

	if (pie):
		print_error("warning", "org_virtual(): Setting memory start address for PIE executable, is this a mistake?", "fixed-address-pie");

	origin = int_coerce(origin);

	if (origin["type"] != "int"):
		return {"type": "error", "value": ERR_UNSUPPORTED_ARGS};

	if (int_bound_check(origin["value"], cpu.vmemdepth, 0)):
		return {"type": "error", "value": ERR_UNSUPPORTED_ARGS};

	vmembase = origin["value"];
	return {"type": "null"};

# `extern` virtual instruction, export sym operand
def extern_virtual(sym):
	if (sym["type"] != "sym"):
		return {"type": "error", "value": ERR_UNSUPPORTED_ARGS};

	if (output_format != "elf"):
		print_error("warning", "Encountered `extern` when assembling static binary, this is likely a mistake", "static-extern");

	exports.append(sym["value"]);
	return {"type": "null"};

# define-data virtual instruction, serialise operands of type
def dd_virtual(offset, bytesize, *operands):
	data = b"";
	for operand in operands:
		serialised = serialise(bytesize, operand, offset + len(data));
		if (type(serialised) != bytes):
			return serialised;
		data += serialised;

	return {"type": "data", "value": data};

# built-in virtual instruction list (see arch/)
builtin_virtual = {
	"org": {"args": [1], "resolve": org_virtual},
	"extern": {"args": [1], "resolve": extern_virtual},
	"incbin": {"args": [1], "resolve": incbin_virtual},

	"align": {"args": [1], "wantsoffset": True, "resolve": lambda offset, alignment: align_virtual(offset, alignment)},

	"db": {"args": False, "wantsoffset": True, "resolve": lambda offset, *operands: dd_virtual(offset, 1, *operands)},
	"dw": {"args": False, "wantsoffset": True, "resolve": lambda offset, *operands: dd_virtual(offset, 2, *operands)},
	"dd": {"args": False, "wantsoffset": True, "resolve": lambda offset, *operands: dd_virtual(offset, 4, *operands)},
	"dq": {"args": False, "wantsoffset": True, "resolve": lambda offset, *operands: dd_virtual(offset, 8, *operands)},
};

# split `obj` ever `n` whatevers
def splitn(obj, n):
	return [obj[i:i + n] for i in range(0, len(obj), n)];

# pad binary with nulls to `padlen`
def null_pad(bin, padlen):
	return bin + (b"\x00" * (padlen - len(bin)));

def fpgasynth_wordiser_helper(bin, wordsize):
	words = splitn(bin, wordsize);
	wordised = b"";
	for word in words:
		word = null_pad(word, wordsize);
		i = 0;
		while (i < wordsize): # fix endian
			byte = hex(word[i])[2:].zfill(2).upper();
			wordised += bytes(byte, "utf-8");
			i += 1;
		wordised += b"\n";
	return wordised;

def mi_generate_header(bin, wordsize):
	words = splitn(bin, wordsize);
	header = b"#File_format=Hex\n";
	header += b"#Address_depth=" + bytes(str(len(words)), "utf-8") + b"\n";
	header += b"#Data_width=" + bytes(str(wordsize * 8), "utf-8") + b"\n";
	return header;

# transform bin to fpgasynth format
def fpgasynth_format_transformer(bin):
	return fpgasynth_wordiser_helper(bin, FPGASYNTH_WORD_SIZE);

# transform bin to .mi format
def mi_format_transformer(bin):
	return mi_generate_header(bin, MI_WORD_SIZE) + fpgasynth_wordiser_helper(bin, MI_WORD_SIZE);

# transfrom bin to .vmd format
def vmd_format_transformer(bin):
	header = b"\xfeVmD";
	if (WORD_ADDRESSED):
		header += struct.pack(">I", vmembase << ADDRESS_SHIFT);
	else:
		header += struct.pack(">I", vmembase);

	output = header + bin;
	return output;

# transform bin to .roadrun format
def roadrun_format_transformer(bin):
	header = b"\x7fROADRUN";
	if (WORD_ADDRESSED):
		header += struct.pack(">I", vmembase << ADDRESS_SHIFT);
		header += struct.pack(">I", search_for_entrypoint() << ADDRESS_SHIFT);
	else:
		header += struct.pack(">I", vmembase);
		header += struct.pack(">I", search_for_entrypoint());

	output = header + bin;
	return output;

# look for entry point symbols, no entry point is also perfectly valid
def search_for_entrypoint():
	if (entrypoint_name in symbols):
		return symbols[entrypoint_name];

	print_error("warning", "Entry point '%s' not found" % entrypoint_name, "no-entry");
	return 0x00000000;

# transform bin to .elf format
def elf_format_transformer(bin):
	elf_header_size = 0x34;
	elf_pheader_size = 0x40; # len(program_header);

	strtab = b"\x00.shstrtab\x00.dynstr\x00.dynsym\x00.comment\x00.text\x00.rel\x00";
	dynsym = b"";
	dynstr = b"";
	rel = b"";
	exported_symbols = 0;

	dynsym_index = 0;
	for sym in symbols.keys():
		if (symtype[sym] == "const" or sym not in exports and export_all == False):
			continue;
		
		dynsym += struct.pack(">I", len(dynstr));
		dynsym += struct.pack(">I", symbols[sym] << ADDRESS_SHIFT);
		dynsym += b"\x00\x00\x00\x00";
		dynsym += b"\x11\x00\x00\x01";
		
		dynstr += bytes(sym, "utf-8");
		dynstr += b"\x00";
		dynsym_index += 1;
		exported_symbols += 1;

	unresolved_exported = [];
	for sym in unresolved:
		symname = sym["symname"];
		if (symname not in exports and export_all == False):
			return False;

		name_index = len(dynstr);
		if (symname in unresolved_exported):
			name_index = dynstr.index(bytes(symname, "utf-8"));
		else:
			dynstr += bytes(symname, "utf-8");
			dynstr += b"\x00";
			unresolved_exported.append(symname);

		dynsym += struct.pack(">I", name_index);
		if (sym["type"] in ["abs8", "abs16", "abs32", "abs64"]):
			dynsym += b"\x00\x00\x00\x00";
		else:
			dynsym += struct.pack(">i", sym["relbase"] << ADDRESS_SHIFT);
		dynsym += b"\x00\x00\x00\x00";
		dynsym += b"\x11\x00\x00\x00";
	
		absreltypes = {
			"abs8": ELF_RRISC_REL_8,
			"abs16": ELF_RRISC_REL_16,
			"abs32": ELF_RRISC_REL_32,
			"abs64": ELF_RRISC_REL_64,

			"rel16": ELF_RRISC_REL_PC16,
		};

		reltype = absreltypes[sym["type"]];
		rel += struct.pack(">I", (vmembase << ADDRESS_SHIFT) + sym["address"]);
		rel += struct.pack(">I", (dynsym_index << 8) | reltype);

		dynsym_index += 1;
		exported_symbols += 1;
	
	comment = ELF_COMMENT;

	real_vmembase = vmembase << ADDRESS_SHIFT;
	section_header_entry_size = 0x28;
	section_header_entry_count = 5 if (exported_symbols > 0) else 3;
	if (len(rel) > 0):
		section_header_entry_count += 1
	elf_section_offset = elf_header_size + elf_pheader_size;
	elf_section_data_offset = elf_header_size + elf_pheader_size + (section_header_entry_size * section_header_entry_count);
	section_data_vmemaddr = real_vmembase + len(bin);

	section_header = b"\x00\x00\x00\x1b"; # .comment
	section_header += b"\x00\x00\x00\x01";
	section_header += b"\x00\x00\x00\x05";
	section_header += struct.pack(">I", section_data_vmemaddr + len(dynstr) + len(dynsym) + len(strtab));
	section_header += struct.pack(">I", elf_section_data_offset + len(dynstr) + len(dynsym) + len(strtab));
	section_header += struct.pack(">I", len(comment));
	section_header += b"\x00\x00\x00\x00";
	section_header += b"\x00\x00\x00\x00";
	section_header += b"\x00\x00\x00\x00";
	section_header += b"\x00\x00\x00\x00";

	section_header += b"\x00\x00\x00\x24"; # .text
	section_header += b"\x00\x00\x00\x01";
	section_header += b"\x00\x00\x00\x07";
	section_header += struct.pack(">I", real_vmembase);
	section_header += struct.pack(">I", elf_section_data_offset + len(dynstr) + len(dynsym) + len(strtab) + len(comment) + len(rel));
	section_header += struct.pack(">I", len(bin));
	section_header += b"\x00\x00\x00\x00";
	section_header += b"\x00\x00\x00\x00";
	section_header += b"\x00\x00\x00\x00";
	section_header += b"\x00\x00\x00\x00";

	section_header += b"\x00\x00\x00\x01"; # .shstrtab
	section_header += b"\x00\x00\x00\x03";
	section_header += b"\x00\x00\x00\x07";
	section_header += struct.pack(">I", section_data_vmemaddr + len(dynstr) + len(dynsym));
	section_header += struct.pack(">I", elf_section_data_offset + len(dynstr) + len(dynsym));
	section_header += struct.pack(">I", len(strtab));
	section_header += b"\x00\x00\x00\x00";
	section_header += b"\x00\x00\x00\x00";
	section_header += b"\x00\x00\x00\x00";
	section_header += b"\x00\x00\x00\x00";

	if (exported_symbols > 0):
		section_header += b"\x00\x00\x00\x0b"; # .dynstr
		section_header += b"\x00\x00\x00\x03";
		section_header += b"\x00\x00\x00\x07";
		section_header += struct.pack(">I", section_data_vmemaddr);
		section_header += struct.pack(">I", elf_section_data_offset);
		section_header += struct.pack(">I", len(dynstr));
		section_header += b"\x00\x00\x00\x00";
		section_header += b"\x00\x00\x00\x00";
		section_header += b"\x00\x00\x00\x00";
		section_header += b"\x00\x00\x00\x00";

		section_header += b"\x00\x00\x00\x13"; # .dynsym
		section_header += b"\x00\x00\x00\x0b";
		section_header += b"\x00\x00\x00\x07";
		section_header += struct.pack(">I", section_data_vmemaddr + len(dynstr));
		section_header += struct.pack(">I", elf_section_data_offset + len(dynstr));
		section_header += struct.pack(">I", len(dynsym));
		section_header += b"\x00\x00\x00\x03";
		section_header += b"\x00\x00\x00\x00";
		section_header += b"\x00\x00\x00\x00";
		section_header += b"\x00\x00\x00\x10";

		if (len(rel) > 0):
			section_header += b"\x00\x00\x00\x2a"; # .rel
			section_header += b"\x00\x00\x00\x09";
			section_header += b"\x00\x00\x00\x05";
			section_header += struct.pack(">I", section_data_vmemaddr + len(dynstr) + len(dynsym) + len(strtab) + len(comment));
			section_header += struct.pack(">I", elf_section_data_offset + len(dynstr) + len(dynsym) + len(strtab) + len(comment));
			section_header += struct.pack(">I", len(rel));
			section_header += b"\x00\x00\x00\x04";
			section_header += b"\x00\x00\x00\x01";
			section_header += b"\x00\x00\x00\x00";
			section_header += b"\x00\x00\x00\x08";

	program_header = b"\x00\x00\x00\x01"; # type
	program_header += struct.pack(">I", elf_section_data_offset + len(strtab) + len(dynstr) + len(dynsym) + len(comment) + len(rel)); # segment file offset
	program_header += struct.pack(">I", real_vmembase); # virtual address
	program_header += struct.pack(">I", real_vmembase); # physical address
	program_header += struct.pack(">I", len(bin)); # segment file size
	program_header += struct.pack(">I", len(bin)); # segment mem size
	program_header += b"\x00\x00\x00\x07"; # protection flags
	program_header += b"\x00\x00\x00\x00"; # alignment

	program_header += b"\x60\x00\x00\x00"; # type
	program_header += struct.pack(">I", elf_section_offset); # segment file offset
	program_header += struct.pack(">I", section_data_vmemaddr); # virtual address
	program_header += struct.pack(">I", section_data_vmemaddr); # physical address
	program_header += struct.pack(">I", len(section_header) + len(dynstr) + len(dynsym) + len(strtab)); # segment file size
	program_header += struct.pack(">I", len(section_header) + len(dynstr) + len(dynsym) + len(strtab)); # segment mem size
	program_header += b"\x00\x00\x00\x07"; # protection flags
	program_header += b"\x00\x00\x00\x00"; # alignment

	header = b"\x7fELF";
	header += bytes([0x01, 0x02, 0x01, ELF_OSABI_TYPE]); # class | data | version | ident
	header += bytes([ELF_OSABI_VERSION, 0x00, 0x00, 0x00]); # abiver | pad
	header += b"\x00\x00\x00\x00"; # pad
	header += struct.pack(">H", ELF_OBJ_TYPE) + struct.pack(">H", cpu.ELF_MACHINE_TYPE); # type | machine
	header += b"\x00\x00\x00\x01"; # version
	header += struct.pack(">I", search_for_entrypoint()); # entry
	header += struct.pack(">I", elf_header_size); # program header file offset
	header += struct.pack(">I", elf_header_size + elf_pheader_size); # section header file offset
	header += b"\x00\x00\x00\x00"; # flags
	header += struct.pack(">H", elf_header_size) + b"\x00\x20"; # header size | program header entry size
	header += struct.pack(">H", int(elf_pheader_size / 0x20)) + b"\x00\x28"; # program header entries | section header entry size
	header += struct.pack(">H", section_header_entry_count) + b"\x00\x02"; # section header entries | section header string table index

	output = header;
	output += program_header;
	output += section_header;
	output += dynstr;
	output += dynsym;
	output += strtab;
	output += comment;
	output += rel;
	output += bin;
	return output;

# format table
formats = {
	"bin": lambda bin: bin, # already binary, no transform
	"fpgasynth": fpgasynth_format_transformer,
	"roadrun": roadrun_format_transformer,
	"mi": mi_format_transformer,
	"vmd": vmd_format_transformer,
	"elf": elf_format_transformer,
};

# format option help text
formats_help = "set output format [ %s ]" % (", ".join(formats.keys()));

# xz compressor assembly transformer
def xz_compressor_transformer(bin, level):
	return lzma.compress(bin, format=lzma.FORMAT_XZ, preset=level);

# gzip compressor assembly transformer
def gz_compressor_transformer(bin, level):
	return gzip.compress(bin, compresslevel=level);

# compressor aliases

compressor_aliases = {
	"lzma": "xz",
	"xzip": "xz",
	"xlib": "xz",

	"zlib": "gz",
	"gzip": "gz",
	"glib": "gz"
};

# compressor table
compressors = {
	"none": lambda bin, level: bin, # no compression
	"xz": xz_compressor_transformer,
	"gz": gz_compressor_transformer,
};

# compressor option help text
compressors_help = "set output compressor [ %s ]" % (", ".join(compressors.keys()));

# bound check int
def int_bound_check(int, bits, signed):
	if ((int < 0) and signed):
		int = abs(int) - 1;
	maxnum = (2 ** bits) - 1;
	if ((int < 0) or (int > maxnum)):
		return True;
	return False;

# decode decimal operand from string
def decode_decimal_operand(operand):
	try:
		if (operand[-1].lower() == 'f'):
			return {"type": "float", "value": float(operand[:-1])};

		return {"type": "decimal", "value": float(operand)};
	except Exception as e:
		return {"type": "error", "value": ERR_DECODE_FAILED};

# decode str operand from string
def decode_str_operand(operand):
	try:
		lexer_output = shlex.split(operand); # this is incorrect use of shlex
		if (len(lexer_output) != 1):
			print_error("error", "decode_str_operand(): Error parsing string (%s)" % operand);
			return {"type": "error", "value": ERR_DECODE_FAILED};
		return {"type": "str", "value": lexer_output[0]};
	except Exception as e:
		print_error("error", "decode_str_operand(): Error parsing string (%s)" % operand);
		return {"type": "error", "value": ERR_DECODE_FAILED};

# decode chr operand from string
def decode_chr_operand(operand):
	try:
		lexer_output = shlex.split(operand); # this is incorrect use of shlex
		if (len(lexer_output) != 1):
			print_error("error", "decode_chr_operand(): Error parsing ASCII integer (%s)" % operand);
			return {"type": "error", "value": ERR_DECODE_FAILED};
		encoded = bytes(lexer_output[0], "utf-8");
		if (len(encoded) != 1):
			print_error("error", "decode_chr_operand(): Error parsing ASCII integer (%s)" % operand);
			return {"type": "error", "value": ERR_DECODE_FAILED};
		return encoded[0];
	except Exception as e:
		print_error("error", "decode_chr_operand(): Error parsing ASCII integer (%s)" % operand);
		return {"type": "error", "value": ERR_DECODE_FAILED};

# decode int operand from string
def decode_int_helper(num):
	num = num.strip();
	try:
		if (num.startswith("0x")):
			return int(num, 16);

		if (num.startswith("0o")):
			return int(num, 8);

		if (num.startswith("0b")):
			return int(num, 2);

		if (num[0] == '\'' and num[-1] == '\''):
			return decode_chr_operand(num);

		return int(num, 10);
	except Exception as e:
		return False;

# split mem ref lexographically
def lex_mem_ref(operand):
	LEXER_STATE_PARSING = 1;
	LEXER_STATE_CLEAN = 2;
	LEXER_STATE_SUBREFERENCE = 3;
	LEXER_STATE_ENTER_SUBREFERENCE = 4;

	state = LEXER_STATE_PARSING;
	components = [];
	component = "";
	for c in operand:
		if (state == LEXER_STATE_ENTER_SUBREFERENCE):
			component = "";
			state = LEXER_STATE_SUBREFERENCE;
		if (state == LEXER_STATE_CLEAN):
			component = "";
			if (c == "["):
				state = LEXER_STATE_SUBREFERENCE;
			elif (c == " "):
				pass;
			else:
				state = LEXER_STATE_PARSING;
		if (state == LEXER_STATE_PARSING):
			if (c == '+'):
				state = LEXER_STATE_CLEAN;
				components.append(decode_operand(component));
	
			component = component + c;
		if (state == LEXER_STATE_SUBREFERENCE):
			component = component + c;
			if (c == ']'):
				state = LEXER_STATE_CLEAN;
				components.append(decode_operand(component));

	if (state == LEXER_STATE_PARSING):
		components.append(decode_operand(component));
	return components;

# precompute memmory operand components
def precompute_mem_operand_components(components):
	summed = 0;
	computed = [];
	for component in components:
		if (component["type"] == "int"):
			summed += component["value"];
		else:
			computed.append(component);

	if (summed == 0):
		return computed;

	summed_int = {"type": "int", "value": summed};
	computed.append(summed_int);
	return computed;


# decode mem operand from string
def decode_mem_operand(operand):
	reference = operand[1:-1].replace(" ", "");

	components = lex_mem_ref(reference);

	decoded_components = [];
	for component in components:
		try:
			component = int_forcecast(component);
			if (type(component) == int):
				component = {"type": "int", "value": component};
		except:
			pass
		if (component["type"] != "sym" and component["type"] != "reg" and component["type"] != "int" and component["type"] != "mem"):
			print_error("error", "decode_mem_operand(): Bad component type in memory ref %s" % operand);
			return {"type": "error", "value": ERR_DECODE_FAILED};

		decoded_components.append(component);
	
	# whar!?!? you cry - this shrimply precomputes [0x0a11 + r0 + 0x0822 + 0x0001] into [0x1234 + r0]
	# it does *not* precompute references like [sym1 + sym2 + sym3 + r0] into [magic_sym1_and_sym2_and_sym3_sum + r0]
	# it can also remove integer components entirely, [r0 + 1 + -1] -> [r0] or [r0 + 0x00] -> [r0]
	decoded_components = precompute_mem_operand_components(decoded_components);

	return {"type": "mem", "value": decoded_components};
"""
	# [0x1234]
	# [X + 0x1234]
	# [Y + 0x1234]
	# [0x34]
	# [X + 0x34]
	# [Y + 0x34]
	# [[0x1234]]
	# [[0x34]]		; 65c02
	# [[X + 0x1234]]	; 65c02
	# [[Y + 0x1234]]	; 65c02
	# [[X + 0x34]]
	# [Y + [0x34]]

	# decode adden if present, adden can be omitted ...
	try:
		adden_index = reference.index("+");
		decodedint = decode_operand(reference[adden_index+1:]);
		if (decodedint["type"] == "sym"):
			decoded["imm"] = 0;
			decoded["sym"] = decodedint["value"];
			regname = reference[:adden_index];
		else:
			try:
				decodedint = int_forcecast(decodedint);
			except Exception as e:
				print_error("error", "decode_mem_operand(): Could not force cast immediate int in memory ref %s" % operand);
				return {"type": "error", "value": ERR_DECODE_FAILED};

			decoded["imm"] = decodedint;
			regname = reference[:adden_index];
	except Exception as e:
		regname = reference;

	# ... ra cannot, error if not present
	if ((regname not in cpu.registers) or (cpu.registers[regname]["type"] != "reg")):
		print_error("error", "decode_mem_operand(): Bad offset-register '%s' during decoding of operand %s" % (regname, operand));
		return {"type": "error", "value": ERR_DECODE_FAILED};

	decoded["value"] = cpu.registers[regname]["value"];
	decoded["regname"] = regname;
	return decoded;
	"""

# decode int operand from string
def decode_int_operand(operand):
	try:
		decoded = decode_int_helper(operand);
		if (type(decoded) != int):
			return {"type": "error", "value": ERR_DECODE_FAILED};

		return {"type": "int", "value": decoded};
	except Exception as e:
		return {"type": "error", "value": ERR_DECODE_FAILED};

# does operand look like decimal?
def looks_like_decimal(operand):
	try: # this is really stupid
		int(operand);
		return False;
	except:
		pass
	try:
		if (operand[-1].lower() == 'f'):
			float(operand[:-1]);
			return True;

		float(operand);
		return True;
	except:
		return False;

# decode operand to type
def decode_operand(operand):
	if (operand in cpu.registers):
		return cpu.registers[operand];

	if (operand[0] == '[' and operand[-1] == ']'):
		return decode_mem_operand(operand);

	if (operand[0] == '"' and operand[-1] == '"'):
		return decode_str_operand(operand);

	intdecode = decode_int_operand(operand); # try as int
	if (intdecode["type"] == "error" and (operand[0] != '\'' and operand[-1] != '\'')):
		decimaldecode = decode_decimal_operand(operand); # try as decimal

		if (decimaldecode["type"] == "error"):
			return {"type": "sym", "value": operand}; # assume ths is some kind of symbol

		return decimaldecode;

	return intdecode;

# resolve an instruction, pass through alias and virtual tables
def resolve_final_instruction(insname, operands, offset):
	if (insname in cpu.aliases):
		insname = cpu.aliases[insname];

	all_virtuals = cpu.virtual | builtin_virtual;
	if (insname in all_virtuals):
		resolver = all_virtuals[insname];
		if (resolver["args"] == False or len(operands) in resolver["args"]):
			try:
				if ("wantsoffset" in resolver):
					return resolver["resolve"](offset, *operands);
				else:
					return resolver["resolve"](*operands);
			except Exception as e:
				print_error("error", "resolve_final_instruction(): Exception during resolution [ %s ]" % format_exception(e));
				return {"type": "error", "value": ERR_RESOLUTION_FAILED};

	return {"type": "instruction", "name": insname};

# decode symbol to relsym or null type
def decode_symbol(symbol):
	if (symbol[-1] == ':'):
		return {"type": "relsym", "value": symbol[:-1]}; # make relative symbol here

	words = symbol.split(' ');
	if (len(words) != 3):
		return {"type": "error", "value": ERR_UNKNOWN_DECODE_ERROR};

	name = words[0];
	value = decode_int_helper(words[2]);
	if (name in symbols):
		print_error("error", "decode_symbol(): Redefined symbol %s" % name);
		return {"type": "error", "value": ERR_SYM_REDEFINED};

	symbols[name] = value;
	symtype[name] = "const";
	return {"type": "null"};

# deduplicate character in string
def strdedup(str, chr):
	dup = chr + chr;
	while (dup in str):
		str = str.replace(dup, chr);
	return str;

# preform fixed symbol decoding prepass
def fixed_sym_decode_prepass(instruction):
	deduped = strdedup(instruction, ' ').strip();
	words = deduped.split(' ');
	if (len(words) != 3):
		return {"type": "null"};

	if (words[1].lower() == "equ"):
		return decode_symbol(deduped);

	return {"type": "null"};

def split_instruction(instruction):
	instruction = instruction.strip();
	whitespace = [' ', '\t', '\v'];
	for i in range(0, len(instruction)):
		c = instruction[i];
		if (c in whitespace):
			return [instruction[:i].strip(), instruction[i + 1:].strip()]

	return [instruction];

def print_operand(operand):
	if (operand["type"] == "reg" or operand["type"] == "vreg"):
		return cpu.registers.keys()[cpu.registers.values().index(operand)];

	if (operand["type"] == "mem"):
		return "[unknown]"; # "[%s+%s]" % (operand["regname"], operand["imm"])

	return str(operand("value"));

# decode instruction to ast object
def decode_instruction(instruction, offset, macro_overrides={}):
	try:
		instruction = instruction.strip();
		if (len(instruction) == 0):
			return {"type": "null"};

		splitinsr = split_instruction(instruction);
		insname = splitinsr[0];
		if len(splitinsr) > 1:
			deduped = strdedup(instruction, ' ').strip();
			words = deduped.split(' ');
			if (len(words) == 3 and words[1].lower() == "equ"):
				return {"type": "null"};

			#splitpoint = instruction.index(' ');
			if (insname[-1] == ':'):
				decodedsym = decode_symbol(insname.replace(' ', ''));
				decodedins = decode_instruction(instruction[instruction.index(":") + 1:], offset, macro_overrides);
				if (type(decodedins) == list):
					return [decodedsym, *decodedins];
				return [decodedsym, decodedins];

			oplist = splitinsr[1];
			operands = oplist.split(',');
		else:
			if (insname[-1] == ':'):
				return decode_symbol(insname.replace(' ', ''));
			operands = [];

		decoded_operands = [];
		for operand in operands:
			operand = operand.strip();
			if (operand in macro_overrides):
				decoded_operands.append(macro_overrides[operand]);
				continue;

			if (operand in macros):
				decoded_operands.append(macros[operand]);
				continue;

			decoded = decode_operand(operand);
			if (decoded["type"] == "error"):
				return decoded;
			decoded_operands.append(decoded);

		resolved = resolve_final_instruction(insname, decoded_operands, offset);
		resolved = [resolved] if type(resolved) != list else resolved;
		resolved_list = [];
		for ins in resolved:
			if (ins["type"] in ["data", "error", "null"]):
				resolved_list.append(ins);
				continue;

			if (ins["type"] != "instruction"):
				print_error("error", "decode_instruction(): Virtual resolver returned unknown type");
				return {"type": "error", "value": ERR_UNKNOWN_DECODE_ERROR};

			local_operands = decoded_operands;
			if ("operands" in ins):
				local_operands = ins["operands"];

			resolved_list.append({"type": "instruction", "name": ins["name"], "operands": local_operands});

		return resolved_list;
	except Exception as e:
		print_error("error", "decode_instruction(): Exception during decoding [ %s ]" % format_exception(e));
		return {"type": "error", "value": ERR_UNKNOWN_DECODE_ERROR};

def format_exception(e):
	return "".join(traceback.format_exception_only(e)).strip();

# serialise instruction at file offset
def serialise_instruction(instruction, offset):
	name = instruction["name"];
	operands = instruction["operands"];
	if (name not in cpu.encodings):
		print_error("error", "serialise_instruction(): Could not find instruction '%s' in encoding table" % name);
		return {"type": "error", "value": ERR_RESOLUTION_FAILED};

	encoding = cpu.encodings[name];
	if (encoding["args"] != len(operands)):
		print_error("error", "serialise_instruction(): Got %s operands, expected [ %s ]" % (encoding["args"], ", ".join(operands)));
		return {"type": "error", "value": ERR_INCORRECT_ARG_COUNT};

	return cpu.encode_instruction(encoding, operands, offset);

def handle_decoded_instruction(decoded, offset, macro_overrides={}):
	if (decoded["type"] == "reprocess"):
		return assemble_instruction(decoded["value"], offset), macro_overrides;

	if (decoded["type"] == "error"):
		return decoded;

	if (decoded["type"] == "data"):
		return decoded;

	if (decoded["type"] == "relsym"):
		symname = decoded["value"];
		if (symname in symbols):
			print_error("error", "assemble_instruction(): Redefined symbol %s" % symname);
			return {"type": "error", "value": ERR_SYM_REDEFINED};

		if (WORD_ADDRESSED):
			symbols[symname] = vmembase + (offset >> ADDRESS_SHIFT);
		else:
			symbols[symname] = vmembase + offset;
		symtype[symname] = "relsym";
		return {"type": "null"};

	if (decoded["type"] == "null"):
		return decoded;

	if (decoded["type"] != "instruction"):
		return {"type": "error", "value": ERR_UNKNOWN_DECODE_ERROR};

	return decoded;

def parse_macrodef(directive):
	function = False;
	nameend = 0;
	whitespace = [' ', '\t', '\v'];
	for i in range(0, len(directive)):
		c = directive[i];
		if (function and c == '('):
			print_error("error", "parse_macrodef(): Bad macro definition [ %s ]" % directive);
			return {"type": "error", "value": ERR_DECODE_FAILED};

		if (not function and c == ')'):
			print_error("error", "parse_macrodef(): Bad macro definition [ %s ]" % directive);
			return {"type": "error", "value": ERR_DECODE_FAILED};

		if (c == '('):
			nameend = i;
			function = True;

		if (function and c == ')'):
			return {"type": "function", "name": directive[:nameend], "operands": directive[nameend + 1:i], "body": directive[i + 1:]};

		if (c in whitespace and not function):
			return {"type": "macro", "name": directive[:i], "body": directive[i + 1:]};

	return {"type": "error", "value": ERR_DECODE_FAILED};

def expand_macro(name, oplist, body, offset, args):
	all_virtuals = cpu.virtual | builtin_virtual;
	if (len(args) not in all_virtuals[name]["args"]):
		return {"type": "error", "value": ERR_INCORRECT_ARG_COUNT};

	macro_overrides = {oplist[i]:args[i] for i in range(len(args))};
	assembly = assemble_lines(body.split('\n'), offset, name, macro_overrides);
	if (type(assembly) != bytes and type(assembly) != bytearray):
		return assembly;

	return {"type": "data", "value": assembly};

def create_macro_function(name, operand, body):
	oplist = [o.strip() for o in operand.split(',')];
	builtin_virtual[name] = {"args": [len(oplist)], "wantsoffset": True, "resolve": lambda offset, *args: expand_macro(name, oplist, body, offset, args)};

def preprocess(instruction, offset):
	global preprocessor_state;
	global preprocessor_accepted;
	global preprocessor_hungry;

	if (instruction.strip().lower() == "#debugger"):
		print(preprocessor_state, preprocessor_hungry, preprocessor_stack, preprocessor_accepted);
		exit();

	instruction = instruction.strip();
	if (instruction[0] == '#'):
		split_directive = split_instruction(instruction[1:]);
		if (split_directive[0] == "endif" and "endif" in preprocessor_accepted):
			if (len(preprocessor_stack) == 0 or preprocessor_state not in preprocessor_controls):
				print_error("error", "preprocess(): Lone ifdef");
				return {"type": "error", "value": ERR_UNIMPLEMENTED};

			controldirective = preprocessor_stack.pop();
			preprocessor_state = controldirective["state"];
			preprocessor_accepted = controldirective["accepted"];
			preprocessor_hungry = controldirective["hunger"];
			if (controldirective["value"] == True):
				return controldirective["body"].strip();
			return "";

		if (split_directive[0] == "else" and "else" in preprocessor_accepted):
			if (len(preprocessor_stack) == 0 or preprocessor_state not in preprocessor_controls):
				print_error("error", "preprocess(): Lone else");
				return {"type": "error", "value": ERR_UNIMPLEMENTED};

			controldirective = preprocessor_stack.pop();
			preprocessor_stack.append({"type": "if", "state": controldirective["state"], "value": not controldirective["value"], "body": "", "hunger": controldirective["hunger"], "accepted": controldirective["accepted"]});
			preprocessor_accepted = ["endif"];
			preprocessor_hungry = True;
			preprocessor_state = PREPROCESSOR_IFDEF;
			if (controldirective["value"] == True):
				return controldirective["body"].strip();
			return "";

		if ((split_directive[0] == "elifdef" and "elifdef" in preprocessor_accepted) or (split_directive[0] == "elifndef" and "elifndef" in preprocessor_accepted)):
			query = split_instruction(split_directive[1]);
			if (len(query) != 1):
				print_error("error", "preprocess(): Trash after elifdef directive");
				return {"type": "error", "value": ERR_UNIMPLEMENTED};

			if (len(preprocessor_stack) == 0 or preprocessor_state not in preprocessor_controls):
				print_error("error", "preprocess(): Lone else");
				return {"type": "error", "value": ERR_UNIMPLEMENTED};

			controldirective = preprocessor_stack.pop();
			symbol = query[0];
			truth = (symbol in macros) or (symbol in symbols);
			if (split_directive[0] == "elifndef"):
				truth = not truth;

			if (controldirective["value"] == True):
				truth = False;

			preprocessor_stack.append({"type": "if", "state": controldirective["state"], "value": False, "body": "", "hunger": controldirective["hunger"], "accepted": controldirective["accepted"]});
			preprocessor_accepted = ["endif", "else", "elifdef", "elifndef"];
			preprocessor_hungry = True;
			preprocessor_state = PREPROCESSOR_IFDEF;
			if (controldirective["value"] == True):
				return controldirective["body"].strip();
			return "";

		if ((split_directive[0] == "ifdef" and "ifdef" in preprocessor_accepted)) or split_directive[0] == "ifndef" and "ifndef" in preprocessor_accepted:
			query = split_instruction(split_directive[1]);
			if (len(query) != 1):
				print_error("error", "preprocess(): Trash after ifdef directive");
				return {"type": "error", "value": ERR_UNIMPLEMENTED};

			symbol = query[0];
			truth = (symbol in macros) or (symbol in symbols);
			if (split_directive[0] == "ifndef"):
				truth = not truth;

			preprocessor_stack.append({"type": "if", "state": preprocessor_state, "value": truth, "body": "", "hunger": preprocessor_hungry, "accepted": preprocessor_accepted});
			preprocessor_accepted = ["endif", "else", "elifdef", "elifndef"];
			preprocessor_hungry = True;
			preprocessor_state = PREPROCESSOR_IFDEF;
			return "";

		if (split_directive[0] == "define" and "define" in preprocessor_accepted):
			macrodef = parse_macrodef(split_directive[1]);
			if (macrodef["type"] == "error"):
				return macrodef;

			if (macrodef["type"] == "function"):
				if (macrodef["body"][-1] == '\\'):
					preprocessor_stack.append({"type": "define", "state": preprocessor_state, "value": macrodef["name"], "body": macrodef["body"][:-1], "hunger": preprocessor_hungry, "operands": macrodef["operands"], "accepted": preprocessor_accepted});
					preprocessor_state = PREPROCESSOR_DEFINE;
					preprocessor_hungry = True;
					preprocessor_accepted = [];
				else:
					create_macro_function(macrodef["name"], macrodef["operands"], macrodef["body"]);
				return "";

			macros[macrodef["name"]] = decode_operand(macrodef["body"]);
			return "";

	if (preprocessor_hungry):
		if (len(preprocessor_stack) == 0):
			print_error("error", "preprocess(): No idea");
			return {"type": "error", "value": ERR_UNIMPLEMENTED};

		if ((preprocessor_state == PREPROCESSOR_DEFINE) and (instruction[-1] == '\\')):
			preprocessor_stack[-1]["body"] += instruction[:-1] + "\n";
		else:
			preprocessor_stack[-1]["body"] += instruction + "\n";

		if ((preprocessor_state == PREPROCESSOR_DEFINE) and (instruction[-1] != '\\')):
			defdirective = preprocessor_stack.pop();
			create_macro_function(defdirective["value"], defdirective["operands"], defdirective["body"]);
			preprocessor_state = defdirective["state"];
			preprocessor_accepted = defdirective["accepted"];
			preprocessor_hungry = defdirective["hunger"];

		return "";


	if (preprocessor_state != PREPROCESSOR_PASSTHROUGH):
		return "";

	return instruction;

# assemble instruction at file offset
def assemble_instruction(instruction, offset, macro_overrides={}):
	instruction = preprocess(instruction, offset);
	if (type(instruction) != str):
		return instruction;

	if ("\n" in instruction or instruction.startswith("#")):
		return assemble_lines(instruction.split('\n'), offset);

	decoded = decode_instruction(instruction, offset, macro_overrides);
	if (type(decoded) == list):
		final_assembly = b"";
		for obj in decoded:
			obj = handle_decoded_instruction(obj, offset, macro_overrides);
			if (obj["type"] == "error"):
				return obj;

			if (obj["type"] == "data"):
				final_assembly += obj["value"];
				continue;

			if (obj["type"] == "null"):
				continue;

			assembled = serialise_instruction(obj, offset + len(final_assembly));
			if (type(assembled) != bytes):
				return assembled;

			final_assembly += assembled;

		return final_assembly;

	decoded = handle_decoded_instruction(decoded, offset, macro_overrides);
	if (decoded["type"] == "error"):
		return decoded;

	if (decoded["type"] == "data"):
		return decoded["value"];

	if (decoded["type"] == "null"):
		return b"";

	return serialise_instruction(decoded, offset);

# strip comments
def strip_comments(line):
	if (";" not in line):
		return line;

	return line[:line.index(";")];

# error to pretty name
def err2name(code):
	if (code == ERR_DECODE_FAILED):
		return "ERR_DECODE_FAILED";
	if (code == ERR_RESOLUTION_FAILED):
		return "ERR_RESOLUTION_FAILED";
	if (code == ERR_UNKNOWN_DECODE_ERROR):
		return "ERR_UNKNOWN_DECODE_ERROR";
	if (code == ERR_INCORRECT_ARG_COUNT):
		return "ERR_INCORRECT_ARG_COUNT";
	if (code == ERR_UNSUPPORTED_ARGS):
		return "ERR_UNSUPPORTED_ARGS";
	if (code == ERR_SERIALISATION_FAILED):
		return "ERR_SERIALISATION_FAILED";
	if (code == ERR_SYM_NOT_FOUND):
		return "ERR_SYM_NOT_FOUND";
	if (code == ERR_GENERIC):
		return "ERR_GENERIC";

def assemble_lines(lines, startoffset=0, macro=None, macro_overrides={}):
	assembly = bytearray();
	linenum = 1;
	offset = startoffset;
	macroerrstr = (" (During expansion of macro %s)" % macro) if macro != None else "";
	for line in lines:
		line = strip_comments(line.strip());
		if (line == ""):
			linenum += 1;
			continue;

		try:
			instruction = assemble_instruction(line, offset, macro_overrides);
		except:
			instruction = {"type": "error", "value": ERR_GENERIC};
		if (type(instruction) != bytes and type(instruction) != bytearray):
			if (instruction["type"] == "error"):
				print_error("error", "assemble_lines(): %s on line %d%s" % (err2name(instruction["value"]), linenum, macroerrstr));
			else:
				print_error("error", "assemble_lines(): Got unknown return value [ %s ] on line %d%s" % (""+instruction, linenum, macroerrstr));
			return instruction;

		offset += len(instruction);
		assembly += instruction;
		linenum += 1;
	return assembly;

# process `source`, assemble, output to `output_file` in `output_format`
def process_file(source):
	global unresolved;
	global compressor;
	macros["__FILE__"] = source;
	file = open(source, "r");
	contents = file.read();
	file.close();

	assembly = bytearray();
	lines = contents.split('\n');
	linenum = 1;
	for line in lines:
		line = strip_comments(line.strip());
		if (line == ""):
			linenum += 1;
			continue;

		stat = fixed_sym_decode_prepass(line);
		if (stat["type"] != "null"):
			if (stat["type"] == "error"):
				print_error("error", "process_file(): %s on line %d" % (err2name(stat["value"]), linenum));
			else:
				print_error("error", "process_file(): Got unknown return value [ %s ] on line %d" % (""+stat, linenum));
			return;
		linenum += 1;

	assembly = assemble_lines(lines, 0);
	if (type(assembly) != bytearray):
		return assembly;

	new_unresolved = [];
	for deferred in unresolved:
		addr = deferred["address"];
		symname = deferred["symname"];
		symtype = deferred["type"];
		symsize = {"abs8": 1, "abs16": 2, "abs32": 4, "abs64": 8, "rel16": 2};
		if ((addr + symsize[symtype] - 1) >= len(assembly)):
			print_error("error", "process_file(): Unresolved symbol extended past assembly");
			return;
	
		if (pie or symname not in symbols):
			if (pie or (symname in exports and output_format == "elf")):
				new_unresolved.append(deferred);
				continue; # this is fine, 

			if (symname not in exports):
				print_error("error", "process_file(): Unresolved symbol '%s'" % symname);
			else:
				print_error("error", "process_file(): Could not export symbol '%s' into %s (Did you forget to output an ELF?)" % (symname, output_format));
			return;
	
		symval = symbols[symname];
		if (symtype == "abs8"):
			if (int_bound_check(symval, 8, 0)):
				print_error("error", "process_file(): Could not encode abs8 symtype, value above 8-bit limit (%s=%s)" % (symname, symval));
				return;

			assembly[addr:addr+1] = struct.pack("B", symbols[symname]);
		elif (symtype == "abs16"):
			if (int_bound_check(symval, 16, 0)):
				print_error("error", "process_file(): Could not encode abs16 symtype, value above 16-bit limit (%s=%s)" % (symname, symval));
				return;

			assembly[addr:addr+2] = struct.pack(">H", symbols[symname]);
		elif (symtype == "abs32"):
			if (int_bound_check(symval, 32, 0)):
				print_error("error", "process_file(): Could not encode abs32 symtype, value above 32-bit limit (%s=%s)" % (symname, symval));
				return;

			assembly[addr:addr+4] = struct.pack(">I", symbols[symname]);
		elif (symtype == "abs64"):
			if (int_bound_check(symval, 64, 0)):
				print_error("error", "process_file(): Could not encode abs64 symtype, value above 64-bit limit (%s=%s)" % (symname, symval));
				return;

			assembly[addr:addr+8] = struct.pack(">Q", symbols[symname]);
		elif (symtype == "rel16"):
			if (int_bound_check(symval, 15, 1)):
				print_error("error", "process_file(): Could not encode rel16 symtype, value above signed 16-bit limit (%s=%s)" % (symname, symval));
				return;

			assembly[addr:addr+2] = struct.pack(">h", symbols[symname] - deferred["relbase"]);

	unresolved = new_unresolved;

	assembly = bytes(assembly);
	if (padded_size != 0):
		if (len(assembly) > padded_size):
			print_error("error", "process_file(): Could not pad binary, assembly larger than pad size");
			return;
		assembly = assembly + (b"\x00" * (padded_size - len(assembly)));

	if (compressor in compressor_aliases):
		compressor = compressor_aliases;
	
	if (output_format not in formats):
		print_error("error", "process_file(): Output format '%s' not supported" % output_format);
		return;
	
	if (compressor not in compressors):
		print_error("error", "process_file(): Compressor '%s' not found" % compressor);
		return;

	assembly = formats[output_format](assembly);

	if (assembly == False):
		print_error("error", "process_file(): Output formatting error.");
		return;

	assembly = compressors[compressor](assembly, compression_level);

	ofile = open(output_filename, "wb");
	ofile.write(assembly);
	ofile.close();

# set vmembase from string
def set_vmembase(origin):
	global vmembase;
	decoded = decode_int_helper(origin);
	if (pie):
		print_error("warning", "set_vmembase(): Setting memory start address for PIE executable, is this a mistake?", "fixed-address-pie");

	if (type(decoded) == bool and decoded == False):
		print_error("error", "set_vmembase(): Could not set origin, bad integer");
		exit();

	vmembase = decoded;

# set pad size from string
def set_pad_size(size):
	global padded_size;
	decoded = decode_int_helper(size);
	if (type(decoded) == bool and decoded == False):
		print_error("error", "set_pad_size(): Could not set pad size, bad integer");
		exit();

	padded_size = decoded;

# set compression level from string
def set_compression_level(level):
	global compression_level;
	decoded = decode_int_helper(level);
	if (type(decoded) == bool and decoded == False):
		print_error("error", "set_compression_level(): Could not set compression level, bad integer");
		exit();

	compression_level = decoded;

class rrasm_resolver():
	def __init__(self):
		pass;

	def __getattribute__(self, name):
		return globals()[name];

# load in appropriate architecture and cpu
def load_architecture():
	global architecture;
	global cpu;
	global target_cpu;
	
	import arch;

	if (target_arch not in dir(arch)):
		print_error("error", "load_architecture(): Could not find architecture '%s'" % target_arch);
		exit();
	
	architecture = getattr(arch, target_arch);
	if (not target_cpu):
		target_cpu = architecture.default_cpu;

	if (target_cpu not in architecture.cpu_table):
		print_error("error", "load_architecture(): Arch does not support CPU '%s'" % target_cpu);
		exit();
	
	rrasm_resolver_obj = rrasm_resolver();
	cpu = architecture.cpu_table[target_cpu](rrasm_resolver_obj);

def init_parser(parser):
	parser.add_argument("source");
	parser.add_argument("-o", "--output", help="output filename");
	parser.add_argument("-f", "--format", help=formats_help);
	parser.add_argument("-a", "--arch", help="set target architecture");
	parser.add_argument("-c", "--cpu", help="set target cpu");

	parser.add_argument("-b", "--origin", help="set origin");
	parser.add_argument("-e", "--entry-point", help="set entrypoint [ -e main ] (not valid for all output formats)");
	parser.add_argument("-w", "--ignore-warning", help="ignore warnings [ -w warningname ]");
	parser.add_argument("-p", "--pad-size", help="pad ASSEMBLY to size (not output file)");
	parser.add_argument("-z", "--compress", help=compressors_help);
	parser.add_argument("-l", "--level", help="set compression level [ 1-9 ]");
	parser.add_argument("-d", "--define", help="define a macro [ -d macro ]");
	parser.add_argument("-E", "--export-all", help="export all symbols by default (set by --pie)", action="store_true");
	parser.add_argument("-s", "--no-pic", help="use jmpabs over jrel code (affects code generation)", action="store_true");
	parser.add_argument("-v", "--no-hlvi", help="disable High-Level Virtual Instructions (affects code generation)", action="store_true");
	parser.add_argument("-r", "--pie", help="generate position independent executable (affects linking)", action="store_true");

def main(args):
	global pic;
	global export_all;
	global pie;
	global target_arch;
	global target_cpu;
	global entrypoint_name;
	global ignored_warnings;
	global output_format;
	global output_filename;
	global compressor;

	pic = not args.no_pic;
	export_all = args.export_all;
	hlvi = not args.no_hlvi;
	if (args.pie):
		export_all = True;
		pie = True;
	if (args.arch):
		target_arch = args.arch;
	if (args.cpu):
		target_cpu = args.cpu;
	if (args.origin != None):
		set_vmembase(args.origin);
	if (args.entry_point != None):
		entrypoint_name = args.entry_point;
	if (args.ignore_warning != None):
		ignored_warnings = [i.strip() for i in args.ignore_warning.split(',')];
	if (args.format != None):
		output_format = args.format;
	if (args.output != None):
		output_filename = args.output;
	if (args.pad_size != None):
		set_pad_size(args.pad_size);
	if (args.level != None):
		set_compression_level(args.level);
	if (args.compress != None):
		compressor = args.compress;
	if (args.define != None):
		for macro in args.define.split(","):
			macros[macro] = "1";

	load_architecture();
	process_file(args.source);

if (__name__ == "__main__"):
	parser = argparse.ArgumentParser(prog="rrasm", description="Roadrunner Assembler");
	init_parser(parser);

	args = parser.parse_args();
	main(args);