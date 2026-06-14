#pragma once

#include <bus.h>

#define IO_BUS_MAX_DEVICES 32

typedef struct {
	bus_type_t type;
	bus_op_t read;
	bus_op_t write;
	bus_free_t free;

	bus_op_t child_read[IO_BUS_MAX_DEVICES];
	bus_op_t child_write[IO_BUS_MAX_DEVICES];
	int read_children;
	int write_children;
} io_bus_t;

bus_t * io_bus_create();
int io_bus_attach_read(bus_t * bus, bus_op_t read);
int io_bus_attach_write(bus_t * bus, bus_op_t write);