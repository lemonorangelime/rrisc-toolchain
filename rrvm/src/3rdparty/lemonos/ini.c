#include <3rdparty/lemonos/ini.h>
#include <3rdparty/lemonos/string.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

ini_table_t * ini_get_tablel(ini_table_t * table, char * name, int size) {
	ini_table_t * node = table;
	while (node) {
		if (memcmp(node->table_name, name, size) == 0) {
			return node;
		}
		node = node->next_table;
	}
	return NULL;
}

ini_table_t * ini_get_table(ini_table_t * table, char * name) {
	return ini_get_tablel(table, name, strlen(name));
}

ini_key_t * ini_get_key(ini_table_t * table, char * name) {
	ini_key_t * key = table->keys;
	while (key) {
		if (strcmp(key->key_name, name) == 0) {
			return key;
		}
		key = key->next_key;
	}
	return NULL;
}

int ini_malformed_name(char * string, int size) {
	char * ini_bad_chars = "[](){}~`!@#$%^&*+=\\|\"';:.,/<>?";
	while (size--) {
		if (char_search(*string++, ini_bad_chars)) {
			return 1;
		}
	}
	return 0;
}

int ini_name_length(char * name) {
	int i = 0;
	while (*name && *name != '\n' && *name != '=' && *name != ']') {
		i++;
		name++;
	}
	return i;
}

char * ini_get_name(char * line) {
	int length = ini_name_length(line);
	char * name = malloc(length + 1);
	memcpy(name, line, length);
	name[length] = '\0';
	return name;
}

char * ini_get_value(char * value) {
	int size = line_strlen(value);
	char * v = malloc(size + 1);
	memcpy(v, value, size);
	v[size] = '\0';
	return v;
}

int ini_identify_line(char * line) {
	int size = line_strlen(line); // length for just this line (stop at \n) instead of the whole string
	switch (*line) { // this
		case '\0':
			return INI_END;
		case '\n':
			return INI_EMPTY;
		case '=':
			return INI_MALFORMED;
	}
	if (*line == '[') {
		// check if the string is too small, lacks a closing bracket, or has a malformed name
		if ((size < 3) || (line[size - 1] != ']') || ini_malformed_name(line + 1, size - 2)) {
			return INI_MALFORMED;
		}
		return INI_TABLE;
	}
	char * key = line; // make sure the key part of the KV pair isnt malformed
	char * ini_bad_chars = "[](){}~`!@#$%^&*+=\\|\"';:.,/<>?";
	while (*key && *key != '=' && *key != '\n') {
		if (char_search(*key, ini_bad_chars)) { // see if this character is an evil one
			return INI_MALFORMED;
		}
		key++;
	}
	if (*key != '=') { // check if the = is actually there (q: should this be INI_TABLE?)
		return INI_MALFORMED;
	}
	// we dont care about if the value is malformed, or even there
	return INI_KV;
}

void ini_append_table(ini_table_t * tables, ini_table_t * table) {
	while (tables->next_table) {
		tables = tables->next_table;
	}
	tables->next_table = table;
}

void ini_append_key(ini_key_t * keys, ini_key_t * key) {
	while (keys->next_key) {
		keys = keys->next_key;
	}
	keys->next_key = key;
}

ini_table_t * ini_add_table(ini_table_t * tables, char * name) {
	ini_table_t * table = malloc(sizeof(ini_table_t));
	table->table_name = name;
	table->keys = NULL;
	table->next_table = NULL;
	ini_append_table(tables, table);
	return table;
}

ini_key_t * ini_add_key(ini_table_t * table, char * key, char * value) {
	ini_key_t * kv = malloc(sizeof(ini_key_t));
	kv->key_name = key;
	kv->value = value;
	kv->next_key = NULL;
	if (!table->keys) {
		table->keys = kv;
		table->top_key = kv;
		return kv;
	}
	table->top_key->next_key = kv;
	table->top_key = kv;
	return kv;
}

void * ini_resolve(ini_table_t * tables, char * symbol) {
	int table_size = 0;
	char * table_name = symbol;
	while (*symbol && *symbol != '.') {
		table_size++;
		symbol++;
	}
	if (*symbol == '\0') {
		return ini_get_table(tables, table_name);
	}
	char * key_name = ++symbol;
	ini_table_t * table = ini_get_tablel(tables, table_name, table_size);
	ini_key_t * key = ini_get_key(table, key_name);
	return key;
}

void ini_dump(ini_table_t * tables, void * callback, void * priv) {
	if (!callback || !tables) {
		return;
	}
	ini_callback_t write = callback;
	ini_table_t * table = tables;
	while (table) {
		write("[", 1, 1, priv);
		write(table->table_name, strlen(table->table_name), 1, priv);
		write("]\n", 2, 1, priv);

		ini_key_t * key = table->keys;
		while (key) {
			write(key->key_name, strlen(key->key_name), 1, priv);
			write("=", 1, 1, priv);
			write(key->value, strlen(key->value), 1, priv);
			write("\n", 1, 1, priv);
			key = key->next_key;
		}
		write("\n", 1, 1, priv);

		table = table->next_table;
	}
	write("\0", 1, 1, priv);
}

void ini_free(ini_table_t * tables) {
	ini_table_t * table = tables;
	ini_table_t * next_table = table;
	while (table) {
		ini_key_t * key = table->keys;
		ini_key_t * next_key = key;
		while (key) {
			free(key->key_name);
			free(key->value);
			next_key = key->next_key;
			free(key);
			key = next_key;
		}
		free(table->table_name);
		next_table = table->next_table;
		free(table);
		table = next_table;
	}
}

ini_table_t * ini_parse(char * ini) {
	ini_table_t * table_root = malloc(sizeof(ini_table_t));
	ini_table_t * table_branch = table_root;
	table_root->table_name = strdup("default");
	table_root->keys = NULL;
	table_root->next_table = NULL;

	char * line = ini;
	while (*line) {
		int identity = ini_identify_line(line);
		if (identity == INI_END) { /* we cant use a switch here */
			break;
		} else if (identity == INI_TABLE) {
			if (*line == '[') {
				line++;
			}
			char * name = ini_get_name(line);
			ini_table_t * table = NULL;
			if (table = ini_get_table(table_root, name)) {
				table_branch = table;
			} else {
				table_branch = ini_add_table(table_branch, name);
			}
		} else if (identity == INI_KV) {
			int length = ini_name_length(line);
			char * key = ini_get_name(line);
			char * value = ini_get_value(line + length + 1);
			ini_add_key(table_branch, key, value);
		}
		line = step_line(line);
	}
	return table_root;
}


size_t ini_generic_write(void * data, size_t size, size_t unit, void * pass) {
	ini_output_t * priv = pass;
	size_t rsize = size * unit;
	if (!priv->data) {
		priv->data = malloc(rsize);
		priv->size = rsize;
		memcpy(priv->data, data, rsize);
		return rsize;
	}
	void * p = malloc(priv->size + rsize);
	memcpy(p, priv->data, priv->size);
	memcpy(p + priv->size, data, rsize);
	free(priv->data);
	priv->size += rsize;
	priv->data = p;
	return rsize;
}