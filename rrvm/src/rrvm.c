#include <cpu.h>
#include <bus.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <args.h>
#include <options.h>
#include <systemdefs.h>
#include <system.h>

int main(int argc, char * argv[]) {
	parse_args(argc, argv);

	sysdef_t * sysdef = lookup_system_def(option_machine_name);
	if (!sysdef) {
		printf("ERROR: could not find system definition in table.\n");
		return -1;
	}
	option_sysdef_ini = ini_parse(sysdef->ini);

	system_t * system = system_create();
	if (!system) {
		return -2;
	}
	int clock_speed = atoi(option_lookup_sysdef_key("cpu.clock-speed"));
	useconds_t delay = 1000000 / clock_speed;

	system->running = 1;
	while (system->running) {
		system_clock(system);
		usleep(delay);
	}
}