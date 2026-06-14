#include <3rdparty/lemonos/argv.h>
#include <3rdparty/lemonos/argvtypes.h>
#include <systemdefs.h>
#include <options.h>
#include <stdio.h>

void args_machine_callback(void * priv, args_option_t * option, char * arg) {
	if (!arg) return;
	option_machine_name = arg;
}

void args_sysdef_callback(void * priv, args_option_t * option, char * arg) {
	if (!arg) return;
	printf("%s\n", arg); // unimplemented
}

void args_option_callback(void * priv, args_option_t * option, char * arg) {
	if (!arg) return;
	printf("%s\n", arg); // unimplemented
}

// TODO: allow multiple memory loads...
void args_load_callback(void * priv, args_option_t * option, char * arg) {
	if (!arg) return;
	option_memory_load = arg;
}

void args_trace_callback(void * priv, args_option_t * option, int present) {
	if (!present) return;
	option_trace = 1;
}

int parse_args(int argc, char * argv[]) {
	args_progspec_t spec[] = {
		"rrasm", "1.0", "Lemon", NULL,
		"unlicense: <https://unlicense.org>",
		"This is public domain software: you are free to change and redistribute it.",
		"There is NO WARRANTY, to the extent permitted by law.",
		"rrisc virtual machine."
	};

	args_option_t options[] = {
		{'m', "machine", 1, TYPE_STRING, 0, args_machine_callback, .help="system defintion preset"},
		{'s', "sysdef",  1, TYPE_STRING, 0, args_sysdef_callback,  .help="read system definition from file"},
		{'o', "option",  1, TYPE_STRING, 0, args_option_callback,  .help="system defintion override [-o option=value ]"},
		{'e', "load",    1, TYPE_STRING, 0, args_load_callback,    .help="load binary file to memory [ -e file || -e 1,file ]"},
		{0,   NULL,      0, TYPE_NULL,   0, NULL,                  .help=""},
		{'t', "trace",   0, TYPE_BOOL,   0, args_trace_callback,   .help="enable tracing"},
	};

	int options_count = sizeof(options) / sizeof(options[0]);
        args_setup(ALLOW_DUPLICATES | ARG_DEFAULT_TO_HELP);
        args_load_spec(spec);

        if (args_parse(argc, argv, options_count, options, NULL)) {
                return -1;
        }

        args_unsetup();
}