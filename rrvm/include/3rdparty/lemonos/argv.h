#pragma once

#include <stdint.h>
#include <3rdparty/lemonos/dynarray.h>

typedef struct args_option args_option_t;

typedef void (* args_int_callback_t)(void * priv, args_option_t * option, int arg, int has_arg);
typedef void (* args_address_callback_t)(void * priv, args_option_t * option, uintptr_t arg, int has_arg);
typedef void (* args_float_callback_t)(void * priv, args_option_t * option, float arg, int has_arg);
typedef void (* args_string_callback_t)(void * priv, args_option_t * option, char * arg);
typedef void (* args_bool_callback_t)(void * priv, args_option_t * option, int present);

typedef struct args_option {
	char short_name;
	char * long_name;
	int wants_argument;
	int type;
	uint32_t flags;
	void * callback;
	void * priv;
	char * help;
} args_option_t;

typedef struct {
	args_option_t * option;
	char * arg;
} args_option_state_t;

typedef struct {
	uint32_t stack_positionals : 1;
	uint32_t allow_option_arguments : 1;
	uint32_t allow_bad_types : 1;
	uint32_t allow_dups : 1;
	uint32_t default_to_help : 1;
	uint32_t no_casts : 1;
	uint32_t silent : 1;
	uint32_t muts_cause_error : 1;
	uint32_t none_required : 1;
	int tab_length;
	char * description; // !
	char * copyright; // !
	char * license; // ! License ${license}
	char * author; // ! Written by ${author}
	char * version; // ! progname (${package}) ${version}
	char * package; // ! above
	char * disclaimer; // !
	char * warranty; // !

	char help_char;
	char version_char;
} args_settings_t;

typedef struct {
	char * package; //
	char * version;
	char * author;
	char * copyright;
	char * license;
	char * disclaimer;
	char * warranty;
	char * description;
} args_progspec_t;

// ./progname --help
// Usage: progname [OPTION]...
// ${description}
//
//  -e, --example-arg       example argument
//
// License ${license}
// LemonOS stdlib license: <https://unlicense.org>

// -- and --

// ./progname --version
// progname (${package}) ${version}
// ${copyright}
// License ${license}
// ${disclaimer}
//
// Written by ${author}

typedef struct {
	dynarray_t * array; // todo: this
} args_priv_t;

enum {
	ALLOW_OPTIONS_AS_ARGS       = 0b000000001,
	ALLOW_INCORRECT_TYPES       = 0b000000010,
	ALLOW_DUPLICATES            = 0b000000100,
	ARG_NO_CASTS                = 0b000001000, // shared with args_option_t->flags
	ARG_SILENT                  = 0b000010000,
	ARG_DEFAULT_TO_HELP         = 0b000100000,
	ARG_STACK_POSITIONALS       = 0b001000000,
	ARG_MUTUAL_EXCLUSION_ERRORS = 0b010000000,
	ARG_NONE_REQUIRED           = 0b100000000,
};

enum {
	ARG_REQUIRED          = 0b00000001,
	ARG_DEFAULT           = 0b00000010,
	ARG_DISABLE_CHECKS    = 0b00000100,
	// ARG_NO_CASTS       = 0b00001000, // shared with args_setup()
	ARG_ARGUMENT_REQUIRED = 0b00010000,
	ARG_FOUND             = 0b10000000,
};

enum {
	ARG_XOR_MASK = 0xffff0000,
};

int args_contains(int argc, char * argv[], char * argument);
int args_contains_long(int argc, char * argv[], char * shortname, char * longname);
int args_parse(int argc, char * argv[], int optionc, args_option_t * options, void * priv);
void args_print_help(int argc, char * argv[], int optionc, args_option_t * options);
void args_setup(uint32_t flags);
void args_unsetup();

void args_set_license(char * license);
void args_set_copyright(char * copyright);
void args_set_description(char * description);
void args_set_disclaimers(char * disclaimer, char * warranty);
void args_set_author(char * author);
void args_set_package(char * package, char * version);
void args_set_version_character(char chr);
void args_set_help_character(char chr);
void args_set_tab_length(int length);
void args_load_spec(args_progspec_t * spec);