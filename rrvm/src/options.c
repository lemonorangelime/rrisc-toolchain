#include <3rdparty/lemonos/ini.h>
#include <stdio.h>

char * option_machine_name = "rrisc";
char * option_memory_load = NULL;
int option_trace = 0;
ini_table_t * option_sysdef_ini = NULL;

char * option_lookup_sysdef_key(char * keyname) {
	ini_key_t * key = ini_resolve(option_sysdef_ini, keyname);
	if (!key) {
		return NULL;
	}
	return key->value;
}