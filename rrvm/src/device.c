#include <device.h>
#include <string.h>

extern device_interface_t dev_interfaces_start;
extern device_interface_t dev_interfaces_end;

device_interface_t * lookup_dev_interface(char * name) {
	device_interface_t * interface = &dev_interfaces_start;
	device_interface_t * end = &dev_interfaces_end;
	while (interface < end) {
		if (strcmp(interface->name, name) == 0) {
			return interface;
		}
		interface++;
	}
	return NULL;
}