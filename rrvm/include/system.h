#pragma once

#include <cpu.h>
#include <bus.h>
#include <device.h>

typedef struct {
	cpu_t * cpu;
	bus_t ** buses;
	device_t ** devices;
	int bus_count;
	int device_count;
	int running;
} system_t;

typedef struct {
	uint32_t magic;
	uint32_t local_bus_address;
} vmd_header_t;



system_t * system_create();
void system_free(system_t * system);
void system_clock(system_t * system);