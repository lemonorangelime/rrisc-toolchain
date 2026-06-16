#include <bus.h>
#include <bus/mem.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int mem_bus_child_read(bus_t * bus, uint32_t address, void * buffer, size_t bytes) {
	mem_bus_t * mem_bus = (mem_bus_t *) bus;
	for (int i = 0; i < mem_bus->read_children; i++) {
		int r = mem_bus->child_read[i](mem_bus->child_read_privates[i], bus, address, buffer, bytes);
		if (r == bytes) {
			return r;
		}
	}
	return -1;
}

int mem_bus_child_write(bus_t * bus, uint32_t address, void * buffer, size_t bytes) {
	mem_bus_t * mem_bus = (mem_bus_t *) bus;
	for (int i = 0; i < mem_bus->write_children; i++) {
		int r = mem_bus->child_write[i](mem_bus->child_write_privates[i], bus, address, buffer, bytes);
		if (r == bytes) {
			return r;
		}
	}
	return -1;
}

int mem_bus_read_handler(bus_t * bus, uint32_t address, void * bufferp, size_t bytes) {
	mem_bus_t * mem_bus = (mem_bus_t *) bus;
	uint8_t * ram = mem_bus->ram;
	uint8_t * buffer = bufferp;
	for (int b = 0; b < bytes; b++) {
		uint8_t data = 0;
		if (mem_bus_child_read(bus, address + b, &buffer[b], 1) == 1) {
			continue;
		}
		if ((address + b) <= mem_bus->size) {
			data = ram[address + b];
		}
		buffer[b] = ram[address + b];
	}
	return bytes;
}

int mem_bus_write_handler(bus_t * bus, uint32_t address, void * bufferp, size_t bytes) {
	mem_bus_t * mem_bus = (mem_bus_t *) bus;
	uint8_t * ram = mem_bus->ram;
	uint8_t * buffer = bufferp;
	for (int b = 0; b < bytes; b++) {
		uint8_t data = buffer[b];
		if (mem_bus_child_write(bus, address + b, &buffer[b], 1) == 1) {
			continue;
		}
		if ((address + b) < mem_bus->size) {
			ram[address + b] = data;
		}
	}
	return bytes;
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

	memset(bus->child_read, 0, sizeof(bus->child_read));
	memset(bus->child_write, 0, sizeof(bus->child_write));
	bus->read_children = 0;
	bus->write_children = 0;
	return (bus_t *) bus;
}

int mem_bus_attach_read(bus_t * bus, bus_child_op_t read, void * priv) {
	mem_bus_t * mem_bus = (mem_bus_t *) bus;
	if (mem_bus->read_children == MEM_BUS_MAX_DEVICES) {
		return -1;
	}
	mem_bus->child_read_privates[mem_bus->read_children] = priv;
	mem_bus->child_read[mem_bus->read_children++] = read;
	return 0;
}

int mem_bus_attach_write(bus_t * bus, bus_child_op_t write, void * priv) {
	mem_bus_t * mem_bus = (mem_bus_t *) bus;
	if (mem_bus->write_children == MEM_BUS_MAX_DEVICES) {
		return -1;
	}
	mem_bus->child_write_privates[mem_bus->write_children] = priv;
	mem_bus->child_write[mem_bus->write_children++] = write;
	return 0;
}