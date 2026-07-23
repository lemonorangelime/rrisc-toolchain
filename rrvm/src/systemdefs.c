#include <systemdefs.h>
#include <string.h>

sysdef_t machines[] = (sysdef_t[]) {
	{	.name = "rrisc",
		.ini =	"[cpu]\n"
			"name=rrisc\n" // 50000000
			"clock-speed=50000000\n" // hz

			"[buses]\n"
			"io-count=1\n"
			"mem-count=1\n"
			"mem-size=131072\n"
		
			"[devices]\n"
			"attached=iotrap,rrisc-vga\n"
		
			"[rrisc-vga]\n"
			"palette-base=0x1a70\n"
			"fb-base=0x1a80\n"		},
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
