#include <systemdefs.h>
#include <string.h>

sysdef_t machines[] = (sysdef_t[]) {
	{	.name = "rrisc",
		.ini =	"[cpu]\n"
			"name=rrisc\n"
			"clock-speed=2\n" // 2hz

			"[buses]\n"
			"io-count=1\n"
			"mem-count=1\n"
			"mem-size=65536\n"	},

	{	.name = "rrisc-tiny",
		.ini =	"[cpu]\n"
			"name=rrisc\n"
			"clock-speed=2\n" // 2hz

			"[buses]\n"
			"io-count=1\n"
			"mem-count=1\n"
			"mem-size=1024\n"	}
};

sysdef_t * lookup_system_def(char * search_name) {
	sysdef_t * machine = &machines[0];
	sysdef_t * end = machines + (sizeof(machines) / sizeof(machines[0]));
	while (machine < end) {
		if (strcmp(machine->name, search_name) == 0) {
			return machine;
		}
		machine++;
	}
	return NULL;
}