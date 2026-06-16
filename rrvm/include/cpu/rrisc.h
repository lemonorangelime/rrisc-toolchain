#pragma once

#include <cpu.h>
#include <bus.h>

typedef struct {
	uint32_t regs[16];
	//uint32_t ctrl[16];
	uint32_t tsc;
	uint32_t pc;
	uint32_t sp;
} rrisc_registers_t;

typedef struct {
	cpu_t cpu;
	bus_t * io_bus;
	bus_t * mem_bus;
	rrisc_registers_t registers;
	int state;
} rrisc_cpu_t;


typedef struct {
	uint8_t opcode;
	uint8_t reg_a : 4;
	uint8_t dest : 4;
	union {
		struct {
			uint8_t padding : 4;
			uint8_t reg_b : 4;
		} __attribute__((packed));
		uint16_t imm;
	};
} __attribute__((packed)) rrisc_instruction_t;

enum {
	RRISC_REG_CTRL_IVA = 16,
};

enum {
	OP_NOP  = 0x00,
	OP_ADD  = 0x02,
	OP_ADDI = 0x03,
	OP_SUB  = 0x04,
	OP_SUBI = 0x05,
	OP_AND  = 0x06,
	OP_ANDI = 0x07,
	OP_OR   = 0x08,
	OP_ORI  = 0x09,
	OP_XOR  = 0x0A,
	OP_XORI = 0x0B,
	OP_SHL  = 0x0C,
	OP_SHLI = 0x0D,
	OP_SHR  = 0x0E,
	OP_SHRI = 0x0F,
	OP_LD   = 0x10,
	OP_ST   = 0x11,
	OP_BEQ  = 0x12,
	OP_BLT  = 0x13,
	OP_JMP  = 0x14,
	OP_JAL  = 0x15,
	OP_JREL = 0x16,
	OP_LDI  = 0x17,
	OP_BNEQ = 0x18,
	OP_BGT  = 0x19,
	OP_LDS  = 0x1a, // load stack pointer
	OP_STS  = 0x1b, // store stack pointer
	OP_PUSH = 0x1c, // push onto stack
	OP_POP  = 0x1d, // pop from stack
	OP_CALL = 0x1e,
	OP_RET  = 0x1f,

	OP_MULH = 0x43,
	OP_MULL = 0x44,

	OP_IRET = 0x22, // retrun from interrupt
	OP_LDIO = 0x23, // load from I/O bus
	OP_STIO = 0x24, // store to I/O bus
	OP_JREG = 0x25,
	OP_HALT = 0xFF
};