#pragma once

#include <3rdparty/lemonos/ini.h>

extern char * option_machine_name;
extern char * option_memory_load;
extern int option_trace;
extern ini_table_t * option_sysdef_ini;

char * option_lookup_sysdef_key(char * keyname);