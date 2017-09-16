
#include "os.h"

void printLine(const char *string) {
	os_sys_write(string);
}

const char* readLine() {
	int length = 255;
	char *buffer = (char *) os_sys_malloc(length);
	os_sys_read(buffer, length);
	return buffer;
}

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
	int length = strLength(input);

	char *reversed = (char *) os_sys_malloc(length + 1);
	int ptr;
	for(ptr = 0; ptr < length; ptr++) {
		reversed[ptr] = input[length - ptr - 2];
	}
	reversed[length - 1] = '\n';

	printLine(reversed);
}
