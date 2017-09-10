
#include "os.h"
#include "string.h"

void app1(int argc, char *argv[]) {
	const char *prefix = "Hello, ";
	const char *suffix = "!\n";
	char *msg;
	if(argc > 0) {
		msg = (char *) os_sys_malloc(strlen(prefix) + strlen(argv[0]) + strlen(suffix) + 1);
		msg[0] = 0;
		strcat(msg, prefix);
		strcat(msg, argv[0]);
		strcat(msg, suffix);
	} else msg = "Hello, World!\n";
	printLine(msg);
}

void app2(void) {
	printLine("Type a string you want to reverse:\n");
	const char *input = readLine();
	int length = strlen(input);

	char *reversed = (char *) os_sys_malloc(length + 1);
	int ptr;
	for(ptr = 0; ptr < length; ptr++) {
		reversed[ptr] = input[length - ptr - 2];
	}
	reversed[length - 1] = '\n';

	printLine(reversed);
}
