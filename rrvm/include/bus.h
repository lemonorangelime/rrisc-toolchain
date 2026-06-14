#pragma once

#include <stdint.h>
#include <stddef.h>


typedef struct bus bus_t;
typedef void (* bus_op_t)(bus_t * bus, uint32_t address, void * buffer, size_t bytes);
typedef void (* bus_free_t)(bus_t * bus);

typedef enum {
	BUS_MEMORY,
	BUS_IO,
} bus_type_t;

typedef struct bus {
	bus_type_t type;
	bus_op_t read;
	bus_op_t write;
	bus_free_t free;
} bus_t;