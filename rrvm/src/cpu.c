#include <cpu.h>
#include <string.h>
#include <stdio.h>

extern cpu_interface_t cpu_interfaces_start;
extern cpu_interface_t cpu_interfaces_end;

cpu_interface_t * lookup_cpu_interface(char * name) {
	cpu_interface_t * interface = &cpu_interfaces_start;
	cpu_interface_t * end = &cpu_interfaces_end;
	while (interface < end) {
		if (strcmp(interface->name, name) == 0) {
			return interface;
		}
		interface++;
	}
	return NULL;
}