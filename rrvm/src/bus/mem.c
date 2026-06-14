#include <bus.h>
#include <bus/mem.h>
#include <stdlib.h>

void mem_bus_read_handler(bus_t * bus, uint32_t address, void * bufferp, size_t bytes) {
	mem_bus_t * mem_bus = (mem_bus_t *) bus;
	uint8_t * ram = mem_bus->ram;
	uint8_t * buffer = bufferp;
	for (int b = 0; b < bytes; b++) {
		uint8_t data = 0;
		if ((address + b) < mem_bus->size) {
			data = ram[address + b];
		}
		buffer[b] = ram[address + b];
	}
}

void mem_bus_write_handler(bus_t * bus, uint32_t address, void * bufferp, size_t bytes) {
	mem_bus_t * mem_bus = (mem_bus_t *) bus;
	uint8_t * ram = mem_bus->ram;
	uint8_t * buffer = bufferp;
	for (int b = 0; b < bytes; b++) {
		uint8_t data = buffer[b];
		if ((address + b) < mem_bus->size) {
			ram[address + b] = data;
		}
	}
}

void mem_bus_free(bus_t * bus) {
	mem_bus_t * mem_bus = (mem_bus_t *) bus;
	free(mem_bus->ram);
	free(mem_bus);
}

bus_t * mem_bus_create(size_t size) {
	mem_bus_t * bus = malloc(sizeof(mem_bus_t));
	bus->type = BUS_MEMORY;
	bus->read = mem_bus_read_handler;
	bus->write = mem_bus_write_handler;
	bus->free = mem_bus_free;

	bus->ram = malloc(size);
	bus->size = size;
	return (bus_t *) bus;
}