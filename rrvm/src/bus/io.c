#include <bus.h>
#include <bus/io.h>
#include <stdlib.h>
#include <string.h>

void io_bus_read_handler(bus_t * bus, uint32_t address, void * buffer, size_t bytes) {
	io_bus_t * io_bus = (io_bus_t *) bus;
	for (int i = 0; i < io_bus->read_children; i++) {
		io_bus->child_read[i](bus, address, buffer, bytes);
	}
}

void io_bus_write_handler(bus_t * bus, uint32_t address, void * buffer, size_t bytes) {
	io_bus_t * io_bus = (io_bus_t *) bus;
	for (int i = 0; i < io_bus->write_children; i++) {
		io_bus->child_write[i](bus, address, buffer, bytes);
	}
}

void io_bus_free(bus_t * bus) {
	free(bus);
}

bus_t * io_bus_create() {
	io_bus_t * bus = malloc(sizeof(io_bus_t));
	bus->type = BUS_IO;
	bus->read = io_bus_read_handler;
	bus->write = io_bus_write_handler;
	bus->free = io_bus_free;

	memset(bus->child_read, 0, sizeof(bus->child_read));
	memset(bus->child_write, 0, sizeof(bus->child_write));
	bus->read_children = 0;
	bus->write_children = 0;
	return (bus_t *) bus;
}

int io_bus_attach_read(bus_t * bus, bus_op_t read) {
	io_bus_t * io_bus = (io_bus_t *) bus;
	if (io_bus->read_children == IO_BUS_MAX_DEVICES) {
		return -1;
	}
	io_bus->child_read[io_bus->read_children++] = read;
	return 0;
}

int io_bus_attach_write(bus_t * bus, bus_op_t write) {
	io_bus_t * io_bus = (io_bus_t *) bus;
	if (io_bus->write_children == IO_BUS_MAX_DEVICES) {
		return -1;
	}
	io_bus->child_write[io_bus->write_children++] = write;
	return 0;
}