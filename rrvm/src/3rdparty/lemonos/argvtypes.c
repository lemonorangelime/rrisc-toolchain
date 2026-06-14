#include <string.h>
#include <3rdparty/lemonos/argvtypes.h>
#include <ctype.h>

// our isdigit() that also accepts `.eE`
int _my_isdigit(char chr) {
	return isdigit(chr) || chr == '.' || chr == 'E' || chr == 'e' || chr == 'O' || chr == 'o' || chr == 'X' || chr == 'x' || chr == 'B' || chr == 'b' || chr == 'h' || chr == 'H' || chr == '#' || (chr >= 'a' && chr <= 'f') || (chr >= 'A' && chr <= 'F');
}

// todo: get rid of this
int string2bool(char * string) {
	if (!string) {
		return -1;
	}
	int size = strlen(string);
	if (size == 1) {
		char c = *string;
		if (c == '0' || c == 'x' || c == 'n' || c == '!') {
			return 0;
		}
		if (c == '1' || c == 'y' || c == 'o') {
			return 1;
		}
		return -1;
	}

	char * disapproving[] = {"false", "no", "non", "nope", "nah", "❌"};
	char * approving[] = {"true", "yes", "yep", "yeah", "✅"};
	int disapproving_size = sizeof(disapproving) / sizeof(disapproving[0]);
	int approving_size = sizeof(approving) / sizeof(approving[0]);
	for (int i = 0; i < disapproving_size; i++) {
		char * disaprove = disapproving[i];
		if (strcmp(disaprove, string) == 0) {
			return 0;
		}
	}
	for (int i = 0; i < approving_size; i++) {
		char * approve = approving[i];
		if (strcmp(approve, string) == 0) {
			return 1;
		}
	}
	return -1;
}

int guess_string_type(char * string) {
	int size = strlen(string);
	for (int i = 0; i < size; i++) {
		if (!_my_isdigit(string[i])) {
			return TYPE_STRING;
		}
	}
	return TYPE_INT;
}