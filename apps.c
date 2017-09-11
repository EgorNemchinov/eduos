
#include "os.h"

int strLength(const char *string) {
	int count = 0;
	while(*(string + count)) {
		count++;
	}
	return count;
}

void app1(int argc, char *argv[]) {
	const char *prefix = "Hello, ";
	const char *suffix = "!\n";
	if(argc > 0) {
		os_sys_write(prefix);
		os_sys_write(argv[0]);
		os_sys_write(suffix);
	} else os_sys_write("Hello, World!\n");
}

void app2(void) {
	printLine("Type a string you want to reverse:\n");
	const char *input = readLine();
	os_sys_write(input);
	int length = strLength(input);

	char *reversed = (char *) os_sys_malloc(length + 1);
	int ptr;
	for(ptr = 0; ptr < length; ptr++) {
		reversed[ptr] = input[length - ptr - 2];
	}
	reversed[length - 1] = '\n';

	printLine(reversed);
}
