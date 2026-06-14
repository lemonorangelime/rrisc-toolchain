#pragma once

#include <bus.h>

typedef struct {
	bus_type_t type;
	bus_op_t read;
	bus_op_t write;
	bus_free_t free;

	void * ram;
	size_t size;
} mem_bus_t;

bus_t * mem_bus_create();