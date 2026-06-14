#pragma once

typedef struct {
	char * name;
	char * ini;
} sysdef_t;



sysdef_t * lookup_system_def(char * search_name);