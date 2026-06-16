#include <device.h>
#include <stdlib.h>
#include <bus/io.h>
#include <stdio.h>
#include <device/iotrap.h>

int iotrap_read_handler(void * priv, bus_t * bus, uint32_t address, void * buffer, size_t bytes) {
	printf("reading %d bytes from %p to %p on bus %p\n", bytes, address, buffer, bus);
	return 0;
}

int iotrap_write_handler(void * priv, bus_t * bus, uint32_t address, void * buffer, size_t bytes) {
	printf("writing %d bytes to %p from %p on bus %p\n", bytes, address, buffer, bus);
	return 0;
}

void iotrap_clock(device_t * device) {}
void iotrap_init(device_t * device) {
	iotrap_device_t * iotrap = (iotrap_device_t *) device;
	if (!iotrap->iobus) {
		return; // erhm
	}
	io_bus_attach_read(iotrap->iobus, iotrap_read_handler, NULL);
	io_bus_attach_write(iotrap->iobus, iotrap_write_handler, NULL);
}

void iotrap_free(device_t * device) {
	free(device->ports);
	free(device);
}

device_t * iotrap_create_device() {
	iotrap_device_t * iotrap = malloc(sizeof(iotrap_device_t));
	iotrap->device.clock = iotrap_clock;
	iotrap->device.free = iotrap_free;
	iotrap->device.init = iotrap_init;
	iotrap->device.port_count = 1;

	bus_port_t * port_list = malloc(sizeof(bus_port_t) * 2);
	port_list[0].type = BUS_IO;
	port_list[0].bus = &iotrap->iobus;
	iotrap->device.ports = port_list;

	return &iotrap->device;
}

device_interface_t DEVICE_INTERFACE iotrap_device_interface = {
	.name = "iotrap",
	.create = iotrap_create_device,
};