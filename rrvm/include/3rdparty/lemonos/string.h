#pragma once

#include <stdint.h>

char * step_line(char * line);
int line_strlen(char * line);
int char_search(char chr, char * hay);
uint64_t strtolh(const char * nptr, char ** endptr, int base);
uint64_t strtolhauto(char * string);