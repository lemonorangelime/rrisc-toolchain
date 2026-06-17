#include <cpu.h>
#include <bus.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <args.h>
#include <options.h>
#include <systemdefs.h>
#include <system.h>
#include <sys/time.h>

uint64_t get_time_ns() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000000ull) + (tv.tv_usec);
}

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
	system->running = 1;

	if (clock_speed < 0) {
		while (system->running) {
			system_clock(system);
		}
	} else {
		uint32_t usec_delay = 1000000llu / clock_speed;
		uint64_t deadline = get_time_ns() + usec_delay;
		while (system->running) {
			system_clock(system);
			while (get_time_ns() < deadline) {}
			deadline = get_time_ns() + usec_delay;
		}
	}
}