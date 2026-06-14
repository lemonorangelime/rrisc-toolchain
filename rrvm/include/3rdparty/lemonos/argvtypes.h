#pragma once

enum {
	TYPE_INT,
	TYPE_STRING,
	TYPE_FLOAT,
	TYPE_NULL, // various purposes, anytypes use this as NULL (obviously), argv will use this as `dont validate` marker
	TYPE_TYPE,
	TYPE_ADDRESS,
	TYPE_REGISTER,
	TYPE_BOOL,
};

int string2bool(char * string);
int guess_string_type(char *);