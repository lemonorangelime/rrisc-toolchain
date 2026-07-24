import argparse;
import struct;
try:
	import rrasm;
except:
	import sys;
	sys.path.append("/usr/local/share/rrasm/");
	import rrasm;

# default variables
vmembase = 0x0000; # executable base (-b)
input_format = None; # input bianry format (-f)
target_arch="rrisc" # target architecture (-a)
target_cpu=None # target cpu (-c)
offset = 0x0000; # offset (into .bin) to start disassembling
literal_mode = False;

# misc
architecture = None;
cpu = None;

def raw_disassembler(binary):
	binary = binary[offset:];
	while (binary):
		read = cpu.disassemble(binary, literal_mode);
		binary = binary[read:]

def roadrun_format_disassembler(binary):
	global vmembase;
	file_vmembase = struct.unpack(">I", binary[8:12]);
	if (vmembase == 0x0000):
		vmembase = file_vmembase;

	raw_disassembler(binary[16:]);

def vmd_format_disassembler(binary):
	global vmembase;
	file_vmembase = struct.unpack(">I", binary[4:8]);
	if (vmembase == 0x0000):
		vmembase = file_vmembase;

	raw_disassembler(binary[8:]);

# format table
formats = {
	"bin": raw_disassembler, # already binary, no transform
#	"fpgasynth": fpgasynth_format_disassembler,
	"roadrun": roadrun_format_disassembler,
#	"mi": mi_format_disassembler,
	"vmd": vmd_format_disassembler,
#	"elf": elf_format_disassembler,
};

# format option help text
formats_help = "set output format [ %s ]" % (", ".join(formats.keys()));

# detect binary format
def detect_format(binary):
	if (binary[0:8] == b"\x7fROADRUN"):
		return "roadrun";

	if (binary[0:4] == b"\xfeVmD"):
		return "vmd";

	return "bin";

# set vmembase from string
def set_vmembase_override(origin):
	global vmembase;
	decoded = rrasm.decode_int_helper(origin);

	if (type(decoded) == bool and decoded == False):
		rrasm.print_error("error", "set_vmembase_override(): Could not set origin, bad integer");
		exit();

	vmembase = decoded;

# set file offset from string
def set_offset(off):
	global offset;
	decoded = rrasm.decode_int_helper(off);

	if (type(decoded) == bool and decoded == False):
		rrasm.print_error("error", "set_offset(): Could not set offset, bad integer");
		exit();

	offset = decoded;

# load in appropriate architecture and cpu
def load_architecture():
	global architecture;
	global cpu;
	global target_cpu;
	
	import arch;

	if (target_arch not in dir(arch)):
		rrasm.print_error("error", "load_architecture(): Could not find architecture '%s'" % target_arch);
		exit();
	
	architecture = getattr(arch, target_arch);
	if (not target_cpu):
		target_cpu = architecture.default_cpu;

	if (target_cpu not in architecture.cpu_table):
		rrasm.print_error("error", "load_architecture(): Arch does not support CPU '%s'" % target_cpu);
		exit();
	
	rrasm_resolver_obj = rrasm.rrasm_resolver();
	cpu = architecture.cpu_table[target_cpu](rrasm_resolver_obj);


def init_parser(parser):
	parser.add_argument("binary");
	parser.add_argument("-f", "--format", help=formats_help);
	parser.add_argument("-a", "--arch", help="set target architecture");
	parser.add_argument("-c", "--cpu", help="set target cpu");

	parser.add_argument("-b", "--origin", help="set origin");
	parser.add_argument("-o", "--offset", help="set file offset");
	parser.add_argument("-w", "--ignore-warning", help="ignore warnings [ -w warningname ]");
	parser.add_argument("-l", "--literal", help="disassemble literally (generate `jrel 1` over `jrel pc + 1`, etc...)", action="store_true");

def process_file(binary):
	f = open(binary, "rb");
	blob = f.read();
	f.close();

	format = input_format;
	if (format == None):
		format = detect_format(blob);

	formats[format](blob);

def main(args):
	global target_arch;
	global target_cpu;
	global ignored_warnings;
	global input_format;
	global literal_mode;

	if (args.arch):
		target_arch = args.arch;
	if (args.cpu):
		target_cpu = args.cpu;
	if (args.format != None):
		input_format = args.format;
	if (args.origin != None):
		set_vmembase_override(args.origin);
	if (args.offset != None):
		set_offset(args.offset);
	if (args.ignore_warning != None):
		ignored_warnings = [i.strip() for i in args.ignore_warning.split(',')];
	if (args.literal):
		literal_mode = True;

	load_architecture();
	process_file(args.binary);

if (__name__ == "__main__"):
	parser = argparse.ArgumentParser(prog="rrasm", description="Roadrunner Assembler");
	init_parser(parser);

	args = parser.parse_args();
	main(args);