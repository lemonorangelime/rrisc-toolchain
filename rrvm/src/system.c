#include <cpu.h>
#include <systemdefs.h>
#include <system.h>
#include <options.h>
#include <stdlib.h>
#include <bus.h>
#include <bus/io.h>
#include <bus/mem.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>



int system_attach_cpu_bus(cpu_t * cpu, bus_t * bus) {
	for (int p = 0; p < cpu->port_count; p++) {
		cpu_port_t * port = &cpu->ports[p];
		if (port->type == CPU_PORT_IO && bus->type == BUS_IO && *port->bus == NULL) {
			*port->bus = bus;
			return 0;
		}
		if (port->type == CPU_PORT_MEMORY && bus->type == BUS_MEMORY && *port->bus == NULL) {
			*port->bus = bus;
			return 0;
		}
	}
	return -1;
}

int system_attach_buses(system_t * system) {
	for (int i = 0; i < system->bus_count; i++) {
		if (system_attach_cpu_bus(system->cpu, system->buses[i]) != 0) {
			return -1;
		}
	}
	return 0;
}

int system_load_vmd(bus_t * bus, char * filename) {
	vmd_header_t vmd_header;
	int fd = open(filename, O_RDONLY);
	if (!fd) return -1;

	ssize_t r = read(fd, &vmd_header, sizeof(vmd_header_t));
	if (r != sizeof(vmd_header_t)) {
		// failed or too small, either way, same response
		close(fd);
		return -2;
	}

	if (ntohl(vmd_header.magic) != 0xfe566d44) {
		close(fd);
		return -3;
	}

	uint32_t bus_address = ntohl(vmd_header.local_bus_address);
	while (r > 0) {
		uint32_t dword = 0;
		r = read(fd, &dword, 4);
		if (r < 0) {
			close(fd);
			return -4;
		}
		bus->write(bus, bus_address, &dword, r);
		bus_address += 4;
	}

	close(fd);
	return 0;
}

void system_free(system_t * system) {
	system->cpu->free(system->cpu);
	for (int i = 0; i < system->bus_count; i++) {
		system->buses[i]->free(system->buses[i]);
	}
	free(system);
}

void system_clock(system_t * system) {
	system->cpu->clock(system->cpu);
}

system_t * system_create() {
	// try to find a CPU first
	cpu_interface_t * cpu_interface = lookup_cpu_interface(option_lookup_sysdef_key("cpu.name"));
	if (cpu_interface == NULL) {
		return NULL;
	}
	int io_buses = atoi(option_lookup_sysdef_key("buses.io-count"));
	int mem_buses = atoi(option_lookup_sysdef_key("buses.mem-count"));
	int total_buses = io_buses + mem_buses;

	system_t * system = malloc(sizeof(system_t));
	system->cpu = cpu_interface->create();
	system->buses = malloc(sizeof(bus_t *) * total_buses);
	system->bus_count = total_buses;
	system->running = 0;

	int busnum = 0;
	for (int i = 0; i < io_buses; i++) {
		bus_t * bus = io_bus_create();
		system->buses[busnum++] = bus;
	}
	for (int i = 0; i < io_buses; i++) {
		int size = atoi(option_lookup_sysdef_key("buses.mem-size")); // this should be an array
		bus_t * bus = mem_bus_create(size);
		system_load_vmd(bus, option_memory_load);
		system->buses[busnum++] = bus;
	}
	if (system_attach_buses(system) != 0) {
		printf("Could not attach buses.\n");
		system_free(system);
		return NULL;
	}
	return system;
}