#include <cpu.h>
#include <bus.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <cpu/rrisc.h>
#include <options.h>
#include <arpa/inet.h>

void rrisc_cpu_write_wrapper(bus_t * bus, uint32_t address, void * buffer, size_t bytes) {
	if (option_trace) {
		printf("%s write %p, %p, %d\n", bus->type == BUS_IO ? "io" : "mem", address, buffer, bytes);
	}
	if (!bus) {
		return;
	}
	bus->write(bus, address << 2, buffer, bytes);
}
void rrisc_cpu_read_wrapper(bus_t * bus, uint32_t address, void * buffer, size_t bytes) {
	if (option_trace) {
		printf("%s read %p, %p, %d\n", bus->type == BUS_IO ? "io" : "mem", address, buffer, bytes);
	}
	if (!bus) {
		memset(buffer, 0, bytes);
		return;
	}
	bus->read(bus, address << 2, buffer, bytes);
}

void rrisc_cpu_fetch(rrisc_cpu_t * rrisc_cpu, rrisc_instruction_t * instruction) {
	rrisc_cpu_read_wrapper(rrisc_cpu->mem_bus, rrisc_cpu->registers.pc, instruction, sizeof(rrisc_instruction_t));
}

#define REGISTER(x) rrisc_cpu->registers.regs[x]
#define RREGISTER(x) ntohl(rrisc_cpu->registers.regs[x])
#define CTRLREGISTER(x) rrisc_cpu->registers.ctrl[x]
#define RCTRLREGISTER(x) ntohl(rrisc_cpu->registers.ctrl[x])
#define IMM(x) ntohs(x)
#define BUS_READ(bus, dest, src, offset) rrisc_cpu_read_wrapper(bus, src + offset, &dest, sizeof(src))
#define BUS_WRITE(bus, dest, src, offset) rrisc_cpu_write_wrapper(bus, dest + offset, &src, sizeof(dest))

void dump_registers(rrisc_cpu_t * rrisc_cpu) {
	for (int i = 0; i < 8; i++) {
		printf(" r%d=0x%.8x ", i, RREGISTER(i));
	}
	printf("\n");
	for (int i = 0; i < 8; i++) {
		printf("%sr%d=0x%.8x ", (i < 2) ? " " : "", i + 8, RREGISTER(i + 8));
	}
	printf("\n pc=0x%.8x  sp=0x%.8x tsc=0x%.8x\n", rrisc_cpu->registers.pc, rrisc_cpu->registers.sp, rrisc_cpu->registers.tsc);
	printf("\n\n");
}

void rrisc_cpu_clock(cpu_t * cpu) {
	rrisc_cpu_t * rrisc_cpu = (rrisc_cpu_t *) cpu;
	rrisc_instruction_t instruction;
	rrisc_cpu_fetch(rrisc_cpu, &instruction);

	if (option_trace) {
		printf("FETCHED: 0x%.8x %d %d %d 0x%.4x\n", ntohl(*(uint32_t*)&instruction), instruction.dest, instruction.reg_a, instruction.reg_b, instruction.imm);
		dump_registers(rrisc_cpu);
	}

	uint32_t pc_next = rrisc_cpu->registers.pc + 1;
	uint32_t sp_next = rrisc_cpu->registers.sp;

	// lazy decoding, quite repetitive but also very flexible
	switch (instruction.opcode) {
		case OP_NOP: break;
		case OP_ADD: REGISTER(instruction.dest) = htonl(RREGISTER(instruction.reg_a) + RREGISTER(instruction.reg_b)); break;
		case OP_SUB: REGISTER(instruction.dest) = htonl(RREGISTER(instruction.reg_a) - RREGISTER(instruction.reg_b)); break;
		case OP_AND: REGISTER(instruction.dest) = htonl(RREGISTER(instruction.reg_a) & RREGISTER(instruction.reg_b)); break;
		case OP_OR: REGISTER(instruction.dest) = htonl(RREGISTER(instruction.reg_a) | RREGISTER(instruction.reg_b)); break;
		case OP_XOR: REGISTER(instruction.dest) = htonl(RREGISTER(instruction.reg_a) ^ RREGISTER(instruction.reg_b)); break;
		case OP_SHL: REGISTER(instruction.dest) = htonl(RREGISTER(instruction.reg_a) << RREGISTER(instruction.reg_b)); break;
		case OP_SHR: REGISTER(instruction.dest) = htonl(RREGISTER(instruction.reg_a) >> RREGISTER(instruction.reg_b)); break;

		case OP_ADDI: REGISTER(instruction.dest) = htonl(RREGISTER(instruction.reg_a) + IMM(instruction.imm)); break;
		case OP_SUBI: REGISTER(instruction.dest) = htonl(RREGISTER(instruction.reg_a) - IMM(instruction.imm)); break;
		case OP_ANDI: REGISTER(instruction.dest) = htonl(RREGISTER(instruction.reg_a) & IMM(instruction.imm)); break;
		case OP_ORI: REGISTER(instruction.dest) = htonl(RREGISTER(instruction.reg_a) | IMM(instruction.imm)); break;
		case OP_XORI: REGISTER(instruction.dest) = htonl(RREGISTER(instruction.reg_a) ^ IMM(instruction.imm)); break;
		case OP_SHLI: REGISTER(instruction.dest) = htonl(RREGISTER(instruction.reg_a) << IMM(instruction.imm)); break;
		case OP_SHRI: REGISTER(instruction.dest) = htonl(RREGISTER(instruction.reg_a) >> IMM(instruction.imm)); break;

		case OP_LD:
			if ((RREGISTER(instruction.reg_a) + IMM(instruction.imm)) == RRISC_VERSION_ADDR) {
				REGISTER(instruction.dest) = htonl(RRISC_VERSION);
				break;
			}
			if ((RREGISTER(instruction.reg_a) + IMM(instruction.imm)) == RRISC_IDENTIFIER_ADDR) {
				REGISTER(instruction.dest) = htonl(RRISC_IDENTIFIER);
				break;
			}
			BUS_READ(rrisc_cpu->mem_bus, REGISTER(instruction.dest), RREGISTER(instruction.reg_a), IMM(instruction.imm));
			break;
		case OP_ST: BUS_WRITE(rrisc_cpu->mem_bus, RREGISTER(instruction.dest), REGISTER(instruction.reg_a), IMM(instruction.imm)); break;

		case OP_BEQ:
			if (RREGISTER(instruction.dest) == RREGISTER(instruction.reg_a)) {
				pc_next = rrisc_cpu->registers.pc + ((int16_t) IMM(instruction.imm));
			}
			break;
		case OP_BLT:
			if (RREGISTER(instruction.dest) > RREGISTER(instruction.reg_a)) { // endian problem?
				pc_next = rrisc_cpu->registers.pc + ((int16_t) IMM(instruction.imm));
			}
			break;

		case OP_JAL:
			REGISTER(instruction.dest) = htonl(pc_next);
		case OP_JMP: pc_next = IMM(instruction.imm); break;
		case OP_JREL: pc_next = rrisc_cpu->registers.pc + ((int16_t) IMM(instruction.imm)); break;

		case OP_LDI: REGISTER(instruction.dest) = htonl(ntohs(instruction.imm)); break;

		case OP_BNEQ:
			if (RREGISTER(instruction.dest) != RREGISTER(instruction.reg_a)) {
				pc_next = rrisc_cpu->registers.pc + ((int16_t) IMM(instruction.imm));
			}
			break;
		case OP_BGT:
			if (RREGISTER(instruction.dest) < RREGISTER(instruction.reg_a)) {
				pc_next = rrisc_cpu->registers.pc + ((int16_t) IMM(instruction.imm));
			}
			break;

		case OP_LDS: REGISTER(instruction.dest) = htonl(rrisc_cpu->registers.sp); break;
		case OP_STS: sp_next = RREGISTER(instruction.reg_a); break;

		case OP_PUSH:
			sp_next = rrisc_cpu->registers.sp - 1;
			BUS_WRITE(rrisc_cpu->mem_bus, sp_next, REGISTER(instruction.reg_a), 0);
			break;
		case OP_POP:
			BUS_READ(rrisc_cpu->mem_bus, REGISTER(instruction.dest), rrisc_cpu->registers.sp, 0);
			sp_next = rrisc_cpu->registers.sp + 1;
			break;

		case OP_CALL: {
			uint32_t big_endian_pc = htonl(pc_next);
			sp_next = rrisc_cpu->registers.sp - 1;
			BUS_WRITE(rrisc_cpu->mem_bus, sp_next, big_endian_pc, 0);
			pc_next = rrisc_cpu->registers.pc + ((int16_t) IMM(instruction.imm));
			break;
		}

		case OP_RET:
			BUS_READ(rrisc_cpu->mem_bus, pc_next, rrisc_cpu->registers.sp, 0);
			pc_next = ntohl(pc_next);
			sp_next = rrisc_cpu->registers.sp + 1;
			break;

		case OP_MULL: REGISTER(instruction.dest) = htonl(RREGISTER(instruction.reg_a) * RREGISTER(instruction.reg_b)); break;
			case OP_MULH: {
			uint64_t result64 = RREGISTER(instruction.reg_a) * RREGISTER(instruction.reg_b);
			uint32_t result = (uint32_t) (result64 >> 32llu);
			REGISTER(instruction.dest) = htonl(result); break;
		}

		case OP_IRET:
			BUS_READ(rrisc_cpu->mem_bus, pc_next, rrisc_cpu->registers.sp, 0);
			sp_next = rrisc_cpu->registers.sp + 2;
			break;

		case OP_LDIO: BUS_READ(rrisc_cpu->io_bus, REGISTER(instruction.dest), RREGISTER(instruction.reg_a), 0); break;
		case OP_STIO: BUS_WRITE(rrisc_cpu->io_bus, RREGISTER(instruction.dest), REGISTER(instruction.reg_a), 0); break;

		case OP_JREG: pc_next = RREGISTER(instruction.dest);

		case OP_HALT: pc_next = rrisc_cpu->registers.pc; break;

		default:
			printf("unimplemented or invalid opcode 0x%.8x\n", ntohl(*(uint32_t*)&instruction));
			break;
	}

	rrisc_cpu->registers.tsc += 1;
	rrisc_cpu->registers.pc = pc_next;
	rrisc_cpu->registers.sp = sp_next;
}

void rrisc_cpu_free(cpu_t * cpu) {
	free(cpu->ports);
	free(cpu);
}

cpu_t * rrisc_cpu_create() {
	rrisc_cpu_t * cpu = malloc(sizeof(rrisc_cpu_t));
	cpu->cpu.clock = rrisc_cpu_clock;
	cpu->cpu.free = rrisc_cpu_free;
	cpu->cpu.port_count = 2;
	cpu->mem_bus = NULL;
	cpu->io_bus = NULL;

	memset(&cpu->registers, 0, sizeof(cpu->registers));

	bus_port_t * port_list = malloc(sizeof(bus_port_t) * 2);
	port_list[0].type = BUS_MEMORY;
	port_list[0].bus = &cpu->mem_bus;
	port_list[1].type = BUS_IO;
	port_list[1].bus = &cpu->io_bus;
	cpu->cpu.ports = port_list;

	return &cpu->cpu;
}

cpu_interface_t CPU_INTERFACE rrisc_cpu_interface = {
	.name = "rrisc",
	.create = rrisc_cpu_create,
};