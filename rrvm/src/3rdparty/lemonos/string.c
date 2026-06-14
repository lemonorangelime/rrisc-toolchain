#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>

char * step_line(char * line) {
        while (*line && *line != '\n') {
                line++;
        }
        if (*line == '\n') {
                line++;
        }
        return line;
}

int line_strlen(char * line) {
        int i = 0;
        while (*line && *line != '\n') {
                i++;
                line++;
        }
        return i;
}

int char_search(char chr, char * hay) {
        while (*hay) {
                if (*hay == chr) {
                        return 1;
                }
                hay++;
        }
        return 0;
}

uint64_t strtolh(const char * nptr, char ** endptr, int base) {
	register const char *s = nptr;
	register uint64_t acc;
	register int c;
	register uint64_t cutoff;
	register int neg = 0, any, cutlim;
	do {
		c = *s++;
	} while (isspace(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else if (c == '+') {
		c = *s++;
	}
	if ((base == 0 || base == 16) && c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0) {
		base = c == '0' ? 8 : 10;
	}
	cutoff = neg ? -(unsigned long long int) LONG_MIN : LONG_MAX;
	cutlim = cutoff % (unsigned long long int) base;
	cutoff /= (unsigned long long int) base;
	for (acc = 0, any = 0;; c = *s++) {
		if (isdigit(c)) {
			c -= '0';
		} else if (isalpha(c)) {
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		} else {
			break;
		}
		if (c >= base) {
			break;
		}
		if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim)) {
			any = -1;
		} else {
			any = 1;
			acc *= base;
			acc += c;
		}
	}
	if (any < 0) {
		acc = neg ? LONG_MIN : LONG_MAX;
	} else if (neg) {
		acc = -acc;
	}
	if (endptr != NULL) {
		*endptr = (char *) (any ? s - 1 : nptr);
	}
	return acc;
}

uint64_t strtolhauto(char * string) {
	if ((string[0] == '0') && (string[1] == 'x')) {
		return strtolh(string, NULL, 16);
	} else if ((string[0] == '0') && (string[1] == 'b')) {
		return strtolh(string + 2, NULL, 2);
	} else if (string[0] == '#') {
		return strtolh(string + 1, NULL, 16);
	} else if ((string[0] == '0') && (string[1] == 'o')) {
		return strtolh(string + 2, NULL, 8);
	} else if (string[strlen(string) - 1] == 'h') {
		return strtolh(string, NULL, 16);
	} else {
		return strtolh(string, NULL, 10);
	}
}