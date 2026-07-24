#!/usr/bin/env python3

import struct;

class FAKE_ENCODER_TABLE():
	def __init__(self, global_encoding):
		self.encoding = global_encoding;

	def __contains__(self, key):
		return True;

	def __getattribute__(self, name):
		return self.encoding;

class AST_DEBUGGER():
	def __init__(self, rrasm):
		# vmemdepth - bit width of the address space
		self.vmemdepth = 64;

		# constants
		self.ELF_MACHINE_TYPE = 0x23; # our (unrecognised, non-standard) machine type
		self.encodings = FAKE_ENCODER_TABLE({"opcode": 0x00, "args": -1, "encoding": ""});

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

			"sp": {"type": "vreg", "value": "sp"},
			"pc": {"type": "vreg", "value": "pc"},
		};

		# aliases
		self.aliases = {};
	
		# virtual instruction list
		self.virtual = {};
	
		self.rrasm = rrasm;

	def serialise_instruction(self, instruction, offset):
		return bytes(str(instruction) + "\n", "utf-8");


# cpu table
cpu_table = {
	"ast": AST_DEBUGGER,
};

# default option
default_cpu = "ast";