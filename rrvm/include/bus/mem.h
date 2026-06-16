#pragma once

#include <bus.h>

#define MEM_BUS_MAX_DEVICES 32

typedef struct {
	bus_type_t type;
	bus_op_t read;
	bus_op_t write;
	bus_free_t free;

	void * ram;
	size_t size;

	void * child_read_privates[MEM_BUS_MAX_DEVICES];
	void * child_write_privates[MEM_BUS_MAX_DEVICES];
	bus_child_op_t child_read[MEM_BUS_MAX_DEVICES];
	bus_child_op_t child_write[MEM_BUS_MAX_DEVICES];
	int read_children;
	int write_children;
} mem_bus_t;

bus_t * mem_bus_create();
int mem_bus_attach_read(bus_t * bus, bus_child_op_t read, void * priv);
int mem_bus_attach_write(bus_t * bus, bus_child_op_t write, void * priv);