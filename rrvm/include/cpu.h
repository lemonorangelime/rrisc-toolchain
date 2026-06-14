#pragma once

#include <bus.h>

typedef struct cpu cpu_t;
typedef void (* cpu_clock_t)(cpu_t * cpu);
typedef void (* cpu_free_t)(cpu_t * cpu);
typedef cpu_t * (* cpu_create_t)();

typedef enum {
	CPU_PORT_MEMORY,
	CPU_PORT_IO,

	CPU_PORT_LIST_END = 0xffff,
} cpu_port_type_t;

typedef struct cpu_port {
	cpu_port_type_t type;
	union {
		bus_t ** bus;
	};
} cpu_port_t;

typedef struct cpu {
	cpu_clock_t clock;
	cpu_free_t free;
	cpu_port_t * ports;
	int interrupt_pin;
	int port_count;
} cpu_t;

typedef struct {
	char * name;
	cpu_create_t create;
} cpu_interface_t;

#define CPU_INTERFACE __attribute__((section(".cpuinterfaces")))



cpu_interface_t * lookup_cpu_interface(char * name);