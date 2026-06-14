#pragma once

#include <stddef.h>

typedef size_t (* ini_callback_t)(void * data, size_t size, size_t unit, void * pass);

typedef struct ini_key {
	char * key_name;
	char * value;
	struct ini_key * next_key;
} ini_key_t;

typedef struct ini_table {
	char * table_name;
	ini_key_t * keys;
	ini_key_t * top_key;
	struct ini_table * next_table;
} ini_table_t;

typedef struct {
	void * data;
	size_t size;
} ini_output_t;

enum {
	INI_END,
	INI_EMPTY,
	INI_TABLE,
	INI_KV,
	INI_MALFORMED,
};

ini_table_t * ini_get_tablel(ini_table_t * table, char * name, int size);
ini_table_t * ini_get_table(ini_table_t * table, char * name);
ini_key_t * ini_get_key(ini_table_t * table, char * name);
int ini_malformed_name(char * string, int size);
int ini_name_length(char * name);
char * ini_get_name(char * line);
char * ini_get_value(char * value);
int ini_identify_line(char * line);
void ini_append_table(ini_table_t * tables, ini_table_t * table);
void ini_append_key(ini_key_t * keys, ini_key_t * key);
ini_table_t * ini_add_table(ini_table_t * tables, char * name);
ini_key_t * ini_add_key(ini_table_t * table, char * key, char * value);
void * ini_resolve(ini_table_t * tables, char * symbol);
void ini_dump(ini_table_t * tables, void * callback, void * priv);
void ini_free(ini_table_t * table);
ini_table_t * ini_parse(char * ini);

size_t ini_generic_write(void * data, size_t size, size_t unit, void * pass);