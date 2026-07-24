#!/usr/bin/env python3

import struct;

class RRISC32():
	def __init__(self, rrasm):
		# vmemdepth - bit width of the address space
		self.vmemdepth = 32;

		# constants
		self.ELF_MACHINE_TYPE = 0x23; # our (unrecognised, non-standard) machine type

		# instruction encoding table

		## "encoding letter" options
		#
		## Name - Letter = Meaning
		#
		# Destination     - d = Rd
		# Register A      - a = Ra
		# Register B      - b = Rb
		# Immediate       - i = #imm
		# Memory (A)      - m = mem[Ra + #imm]
		# Memory (D)      - M = mem[Rd + #imm]
		# Relative (#imm) - r = signed(#imm) - pc
		#
		## Obselete encodings
		#
		# Control (A)     - c = ctrlregs[Ra]
		# Control (D)     - C = ctrlregs[Rd]
		self.encodings = {
			"nop":	{"opcode": 0x00, "args": 0, "encoding": ""},
			"add":	{"opcode": 0x02, "args": 3, "encoding": "dab"},
			"addi":	{"opcode": 0x03, "args": 3, "encoding": "dai"},
			"sub":	{"opcode": 0x04, "args": 3, "encoding": "dab"},
			"subi":	{"opcode": 0x05, "args": 3, "encoding": "dai"},
			"and":	{"opcode": 0x06, "args": 3, "encoding": "dab"},
			"andi":	{"opcode": 0x07, "args": 3, "encoding": "dai"},
			"or":	{"opcode": 0x08, "args": 3, "encoding": "dab"},
			"ori":	{"opcode": 0x09, "args": 3, "encoding": "dai"},
			"xor":	{"opcode": 0x0a, "args": 3, "encoding": "dab"},
			"xori":	{"opcode": 0x0b, "args": 3, "encoding": "dai"},
			"shl":	{"opcode": 0x0c, "args": 3, "encoding": "dab"},
			"shli": {"opcode": 0x0d, "args": 3, "encoding": "dai"},
			"shr":	{"opcode": 0x0e, "args": 3, "encoding": "dab"},
			"shri":	{"opcode": 0x0f, "args": 3, "encoding": "dai"},
			"ld":	{"opcode": 0x10, "args": 2, "encoding": "dm"},
			"st":	{"opcode": 0x11, "args": 2, "encoding": "Ma"},
			"beq":	{"opcode": 0x12, "args": 3, "encoding": "dar"},
			"blt":	{"opcode": 0x13, "args": 3, "encoding": "dar"},
			"jmp":	{"opcode": 0x14, "args": 1, "encoding": "i"},
			"jal":	{"opcode": 0x15, "args": 2, "encoding": "di"},
			"jalr":	{"opcode": 0x15, "args": 2, "encoding": "dr"},
			"jrel":	{"opcode": 0x16, "args": 1, "encoding": "r"},
			"ldi":	{"opcode": 0x17, "args": 2, "encoding": "di"},
			"bneq":	{"opcode": 0x18, "args": 3, "encoding": "dar"},
		#	"bgt":	{"opcode": 0x19, "args": 3, "encoding": "dar"},
			"lds":	{"opcode": 0x1a, "args": 1, "encoding": "d"},
			"sts":	{"opcode": 0x1b, "args": 1, "encoding": "a"},
			"push":	{"opcode": 0x1c, "args": 1, "encoding": "a"},
			"pop":	{"opcode": 0x1d, "args": 1, "encoding": "d"},
			"call":	{"opcode": 0x1e, "args": 1, "encoding": "r"},
			"ret":	{"opcode": 0x1f, "args": 0, "encoding": ""},

			"mulu":	{"opcode": 0x43, "args": 3, "encoding": "dab"},
			"mull":	{"opcode": 0x44, "args": 3, "encoding": "dab"},

			# ...
			"iret":	{"opcode": 0x22, "args": 0, "encoding": ""},
			"ldio":	{"opcode": 0x23, "args": 2, "encoding": "da"},
			"stio":	{"opcode": 0x24, "args": 2, "encoding": "da"},
			"jreg": {"opcode": 0x25, "args": 1, "encoding": "d"},
			"halt":	{"opcode": 0xff, "args": 0, "encoding": ""},
		};

		# register table
		self.registers = {
			"r0": {"type": "reg", "value": 0},
			"r1": {"type": "reg", "value": 1},
			"r2": {"type": "reg", "value": 2},
			"r3": {"type": "reg", "value": 3},
			"r4": {"type": "reg", "value": 4},
			"r5": {"type": "reg", "value": 5},
			"r6": {"type": "reg", "value": 6},
			"r7": {"type": "reg", "value": 7},
			"r8": {"type": "reg", "value": 8},
			"r9": {"type": "reg", "value": 9},
			"r10": {"type": "reg", "value": 10},
			"r11": {"type": "reg", "value": 11},
			"r12": {"type": "reg", "value": 12},
			"r13": {"type": "reg", "value": 13},
			"r14": {"type": "reg", "value": 14},
			"r15": {"type": "reg", "value": 15},

			# controls
			#"iva": {"type": "ctrl", "value": 16},
			#"r16": {"type": "ctrl", "value": 16},

			# virtual
			"sp": {"type": "vreg", "value": "sp"},
			"pc": {"type": "vreg", "value": "pc"},
		};

		# aliases
		self.aliases = {
			"hlt": "halt",
		};
	
		# virtual instruction list
		self.virtual = {
			"mov": {"args": [2], "resolve": self.mov_virtual_instruction},
			"add": {"args": [2, 3], "resolve": lambda a, b, c=False: self.rrasm.rr_imm_virtual_resolver(a, b, c, "add", "addi")},
			"sub": {"args": [2, 3], "resolve": lambda a, b, c=False: self.rrasm.rr_imm_virtual_resolver(a, b, c, "sub", "subi")},
			"and": {"args": [2, 3], "resolve": lambda a, b, c=False: self.rrasm.rr_imm_virtual_resolver(a, b, c, "and", "andi")},
			"or": {"args": [2, 3], "resolve": lambda a, b, c=False: self.rrasm.rr_imm_virtual_resolver(a, b, c, "or", "ori")},
			"xor": {"args": [2, 3], "resolve": lambda a, b, c=False: self.rrasm.rr_imm_virtual_resolver(a, b, c, "xor", "xori")},
			"shl": {"args": [2, 3], "resolve": lambda a, b, c=False: self.rrasm.rr_imm_virtual_resolver(a, b, c, "shl", "shli")},
			"shr": {"args": [2, 3], "resolve": lambda a, b, c=False: self.rrasm.rr_imm_virtual_resolver(a, b, c, "shr", "shri")},

			# support x86-like syntax support for immediate forms
			"addi": {"args": [2, 3], "resolve": lambda a, b, c=False: self.rrasm.r_imm_virtual_resolver(a, b, c, "addi")},
			"subi": {"args": [2, 3], "resolve": lambda a, b, c=False: self.rrasm.r_imm_virtual_resolver(a, b, c, "subi")},
			"andi": {"args": [2, 3], "resolve": lambda a, b, c=False: self.rrasm.r_imm_virtual_resolver(a, b, c, "andi")},
			"ori": {"args": [2, 3], "resolve": lambda a, b, c=False: self.rrasm.r_imm_virtual_resolver(a, b, c, "ori")},
			"xori": {"args": [2, 3], "resolve": lambda a, b, c=False: self.rrasm.r_imm_virtual_resolver(a, b, c, "xori")},
			"shli": {"args": [2, 3], "resolve": lambda a, b, c=False: self.rrasm.r_imm_virtual_resolver(a, b, c, "shli")},
			"shri": {"args": [2, 3], "resolve": lambda a, b, c=False: self.rrasm.r_imm_virtual_resolver(a, b, c, "shri")},

			"mul": {"args": [2, 3, 4], "resolve": lambda a, b, c=False, d=False: self.mul_virtual_resolver(a, b, c, d, "mull")},
			"mull": {"args": [2, 3], "resolve": lambda a, b, c=False: self.mul_virtual_resolver(a, b, c, False, "mull")},
			"mulh": {"args": [2, 3], "resolve": lambda a, b, c=False: self.mul_virtual_resolver(a, b, c, False, "mulh")},

			"jmp": {"args": [1], "resolve": self.jmp_virtual_instruction},
			"jmpabs": {"args": [1], "resolve": self.jmpabs_virtual_instruction},
			"out": {"args": [2], "resolve": self.out_virtual_instruction},
			"in": {"args": [2], "resolve": self.in_virtual_instruction},
			"bgt": {"args": [3], "resolve": self.bgt_virtual_instruction},
		};
	
		self.rrasm = rrasm;


	# `mov` instruction virtual overload resolver, resolve to various load instructions depending on operand types
	def mov_virtual_instruction(self, a, b):
		b = self.rrasm.int_coerce(b);
		if (a["type"] == "reg" and b["type"] == "reg"):
			return {"type": "instruction", "name": "ori", "operands": [a, b, self.rrasm.decode_operand("0")]};

		if (a["type"] == "mem" and b["type"] == "reg"):
			return {"type": "instruction", "name": "st"};

		if (a["type"] == "reg" and b["type"] == "mem"):
			return {"type": "instruction", "name": "ld"};

		if (a["type"] == "reg" and (b["type"] == "int" or b["type"] == "sym")):
			return {"type": "instruction", "name": "ldi"};

		if (a["type"] == "reg" and b["type"] == "ctrl"):
			return {"type": "instruction", "name": "ldc"};

		if (a["type"] == "ctrl" and b["type"] == "reg"):
			return {"type": "instruction", "name": "stc"};

		if ((a["type"] == "vreg" and a["value"] == "sp") and b["type"] == "reg"):
			return {"type": "instruction", "name": "sts", "operands": [b]};

		if (a["type"] == "reg" and (b["type"] == "vreg" and b["value"] == "sp")):
			return {"type": "instruction", "name": "lds", "operands": [a]};

		return {"type": "error", "value": self.rrasm.ERR_UNSUPPORTED_ARGS};

	# `jmp` instruction virtual resolver, resolve to jrel or jmp depending on pic settings
	def jmp_virtual_instruction(self, a):
		if (a["type"] == "reg"):
			return {"type": "instruction", "name": "jreg"};

		if (self.rrasm.pic):
			return {"type": "instruction", "name": "jrel"};

		return {"type": "instruction", "name": "jmp"};

	# `jmpabs` instruction virtual resolver, resolve to jmp always regardless of pic
	def jmpabs_virtual_instruction(self, a):
		return {"type": "instruction", "name": "jmp"};

	# `out` instruction virtual resolver, resolve to stio always
	def out_virtual_instruction(self, a, b):
		return {"type": "instruction", "name": "stio"};

	# `out` instruction virtual resolver, resolve to ldio
	def in_virtual_instruction(self, a, b):
		return {"type": "instruction", "name": "ldio"};

	# `mul` instruction virtual resolver, resolve to mull, mulu, or both depending on operand count
	def mul_virtual_resolver(self, a, b, c, d, which="mull"):
		if (not c):
			c = b;
			b = a;

		if (d):
			if (not self.rrasm.hlvi): # mul low, high, X, Y is a high-level virtual, only allow if enabled
				return {"type": "error", "value": self.rrasm.ERR_UNSUPPORTED_ARGS};

			return [
				{"type": "instruction", "name": "mull", "operands": [a, c, d]},
				{"type": "instruction", "name": "mulu", "operands": [b, c, d]},
			];

		return {"type": "instruction", "name": which, "operands": [a, b, c]};

	# 'bgt' instruction virtual resolver, resolve to operand swapped blt
	def bgt_virtual_instruction(self, a, b, c):
		return {"type": "instruction", "name": "blt", "operands": [b, a, c]};

	# map memory reference to simplified rrisc format memory reference
	def map_mem_to_rrisc_offset(self, operand):
		if (operand["type"] != "mem"):
			return operand;
	
		components = operand["value"];
		if (len(components) > 2):
			self.rrasm.print_error("error", "map_mem_to_rrisc_offset(): Too many components to map mem ref to rrisc register offset");
			return {"type": "error", "value": self.rrasm.ERR_UNSUPPORTED_ARGS};

		# rrisc memory references are always register offset (at least currently)
		register = None;
		offset = None;
		for component in components:
			if (component["type"] == "reg" and register == None):
				register = component;
			elif ((component["type"] == "sym" or component["type"] == "int") and offset == None):
				offset = component;
			else:
				pretty_names = {
					"mem": "Memory Reference",
					"float": "Float",
					"decimal": "Fixed-Point",
					"str": "String",
				};
				optype = operand["type"];
				self.rrasm.print_error("error", "map_mem_to_rrisc_offset(): Could not map mem ref to rrisc register offset, bad component type '%s', dunno what you're trying to accomplish 🤷" % (pretty_names[optype] if optype in pretty_names else optype));
				return {"type": "error", "value": self.rrasm.ERR_UNSUPPORTED_ARGS};

		if (register == None):
			self.rrasm.print_error("error", "map_mem_to_rrisc_offset(): Missing register to offset, could not map mem ref");
			return {"type": "error", "value": self.rrasm.ERR_UNSUPPORTED_ARGS};

		reg_obj = {"type": "mem", "value": register["value"], "imm": 0};

		if (offset != None):
			if (offset["type"] == "sym"):
				reg_obj["sym"] = offset["value"];
			else:
				reg_obj["imm"] = offset["value"];

		return reg_obj;

	# encode instruction at file offset
	def encode_instruction(self, encoding, operands, offset):
		serialised = bytearray([encoding["opcode"], 0, 0, 0]);
		relative_unresolved = False;
		optypes = encoding["encoding"];
		i = 0;
		for operand in operands:
			operand = self.map_mem_to_rrisc_offset(operand);
			if (operand["type"] == "err"):
				return operand;

			optype = optypes[i];
			if (operand["type"] == "str"):
				operand = self.rrasm.int_coerce(operand);
			if (operand["type"] == "sym"):
				symname = operand["value"];
				if (symname in self.rrasm.symbols and (self.rrasm.symtype[symname] == "const" or self.rrasm.pie == False)):
					operand = {"type": "int", "value": self.rrasm.symbols[symname]};
				else:
					operand = {"type": "int", "value": self.rrasm.vmembase};
					if (optype == 'r'):
						relative_unresolved = True;
						if (self.rrasm.WORD_ADDRESSED):
							self.rrasm.unresolved.append({"type": "rel16", "symname": symname, "address": offset + 2, "relbase": self.rrasm.vmembase + (offset >> self.rrasm.ADDRESS_SHIFT)});
						else:
							self.rrasm.unresolved.append({"type": "rel16", "symname": symname, "address": offset + 2, "relbase": self.rrasm.vmembase + offset});
					else:
						self.rrasm.unresolved.append({"type": "abs16", "symname": symname, "address": offset + 2});

			if (operand["type"] == "mem" and "sym" in operand):
				symname = operand["sym"];
				value = operand["value"];
				if (symname in self.rrasm.symbols and (self.rrasm.symtype[symname] == "const" or self.rrasm.pie == False)):
					operand = {"type": "mem", "value": value, "imm": self.rrasm.symbols[symname]};
				else:
					operand = {"type": "mem", "value": value, "imm": self.rrasm.vmembase};
					self.rrasm.unresolved.append({"type": "abs16", "symname": symname, "address": offset + 2});

			# this needs to be simplified, very messy due to frequent changes
			if ((optype == 'd' or optype == 'a' or optype == 'b') and operand["type"] != "reg"):
				return {"type": "error", "value": self.rrasm.ERR_UNSUPPORTED_ARGS};

			if ((optype == 'm' or optype == 'M') and operand["type"] != "mem"):
				return {"type": "error", "value": self.rrasm.ERR_UNSUPPORTED_ARGS};

			if ((optype == 'i' or optype == 'r') and operand["type"] != "int"):
				return {"type": "error", "value": self.rrasm.ERR_UNSUPPORTED_ARGS};

			if ((optype == 'c' or optype == 'C') and operand["type"] != "ctrl"):
				return {"type": "error", "value": self.rrasm.ERR_UNSUPPORTED_ARGS};

			if (optype == 'd' or optype == 'M' or optype == "C"):
				is_control = optype == "C";
				serialised[1] = serialised[1] | ((operand["value"] - (16 if is_control else 0)) << 4);

			if (optype == 'a' or optype == 'm' or optype == "c"):
				is_control = optype == "c";
				serialised[1] = serialised[1] | operand["value"] - (16 if is_control else 0);

			if (optype == 'b'):
				serialised[2] = serialised[2] | (operand["value"] << 4);

			if (optype == 'i'):
				value = operand["value"];
				if (self.rrasm.int_bound_check(value, 16, 0)):
					return {"type": "error", "value": self.rrasm.ERR_UNSUPPORTED_ARGS};

				serialised[3] = value & 0xff;
				serialised[2] = (value >> 8) & 0xff;

			if (optype == 'r' and relative_unresolved == False):
				if (self.rrasm.WORD_ADDRESSED):
					value = operand["value"] - (self.rrasm.vmembase + (offset >> self.rrasm.ADDRESS_SHIFT));
				else:
					value = operand["value"] - (self.rrasm.vmembase + offset);

				if (self.rrasm.int_bound_check(value, 15, 1)):
					self.rrasm.print_error("error", "serialise_instruction(): Could not encode relative reference, %s outside signed 16-bit int limits" % value);
					return {"type": "error", "value": self.rrasm.ERR_UNSUPPORTED_ARGS};

				packed = struct.pack(">h", value);
				serialised[2] = packed[0];
				serialised[3] = packed[1];

			if ((optype == 'm' or optype == 'M') and operand["type"] == "mem"):
				if (operand["imm"] < 0 or operand["imm"] >= 0x10000):
					return {"type": "error", "value": self.rrasm.ERR_UNSUPPORTED_ARGS};

				serialised[3] = operand["imm"] & 0xff;
				serialised[2] = (operand["imm"] >> 8) & 0xff;
			i = i + 1;

		return bytes(serialised);


# cpu table
cpu_table = {
	"rrisc32": RRISC32,
};

# default option
default_cpu = "rrisc32";