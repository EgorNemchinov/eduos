
#include <string.h>

#include "os.h"
#include "apps.h"

// extern char *strtok_r(char *str, const char *delim, char **saveptr);

static int echo(int argc, char *argv[]) {
	for (int i = 1; i < argc; ++i) {
		os_sys_write(argv[i]);
		os_sys_write(i == argc - 1 ? "\n" : " ");
	}
	return 0;
}

static const struct {
	const char *name;
	int(*fn)(int, char *[]);
} app_list[] = {
	{ "echo", echo },
};

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


// #define ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))

/*static int do_task(char *command) {
	char *saveptr;
	char *arg = strtok_r(command, " ", &saveptr);
	char *argv[256];
	int argc = 0;
	while (arg) {
		argv[argc++] = arg;
		arg = strtok_r(NULL, " ", &saveptr);
	}

	for (int i = 0; i < ARRAY_SIZE(app_list); ++i) {
		if (!strcmp(argv[0], app_list[i].name)) {
			return app_list[i].fn(argc, argv);
		}
	}

	char msg[256] = "No such function: ";
	strcat(msg, argv[0]);
	strcat(msg, "\n");
	os_sys_write(msg);
	return 1;
}*/

/*void shell() {
	while (1) {
		os_sys_write("> ");	
		char buffer[256];
		int bytes = os_sys_read(buffer, sizeof(buffer));
		if (!bytes) {
			break;
		}

		if (bytes < sizeof(buffer)) {
			buffer[bytes] = '\0';
		}

		char *saveptr;
		const char *comsep = "\n;";
		char *cmd = strtok_r(buffer, comsep, &saveptr);
		while (cmd) {
			do_task(cmd);
			cmd = strtok_r(NULL, comsep, &saveptr);
		}
	}
	os_sys_write("\n");
}*/
