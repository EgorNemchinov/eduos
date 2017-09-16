#include "string.h"
#include "os.h"
#include "apps.h"

#define MAX_ARGS 10
#define MAX_ARG_LENGTH 100

const char *BEGIN_STRING = "Welcome, stranger!\nTo find out possible commands, type 'help' or 'h'\n";
const char *HELP_STRING = "Possible commands:\n    app1(name='World') - prints 'Hello, $name$!'\n"
											  "    app2() - reads line, reverses it and prints back.\n"
											  "    quit or q - exits the shell.\n"
											  "For example, you can use a combination of these: 'app1 brother; app1; app2;'\n";


int execute(int argc, char *argv[]) {
	char *appName = argv[0];
	if((strcmp(appName, "quit") == 0) | (strcmp(appName, "q") == 0)) {
		os_sys_write("Okay, bye!");
		return 0;
	} else if(strcmp(appName, "app1") == 0) {
		if(argc > 2) {
			os_sys_write("Too many arguments for app1. Type 'help' or 'h' to learn more about possible commands.\n");
			return 1;
		}
		app1(argc - 1, argv + 1);
	} else if(strcmp(appName, "app2") == 0) {
		if(argc > 1) {
			os_sys_write("Too many arguments for app2. Type 'help' or 'h' to learn more about possible commands.\n");
			return 1;
		}
		app2();
	} else if((strcmp(appName, "help") == 0 ) | ( strcmp(appName, "h") == 0)) {
		os_sys_write(HELP_STRING);
	} else if(strlen(appName) > 0) {
		os_sys_write("Unknown command: \"");
		os_sys_write(appName);
		os_sys_write("\". To find out possible commands, type 'help' or 'h'\n");
	}
	return 1;
}

// Recursive function, each time passing into *string unprocessed part of input line
int parse_input(const char *string, int argIndex, char *argv[], int ptr) {
	if((argIndex == 0 ) & ( ptr == 0)) {
		argv[0] = (char *) os_sys_malloc(MAX_ARG_LENGTH);
	}

	int res, argc;
	switch(string[0]) {
		case ';':
		case '\0':
		case '\n':
			//Execute and possibly return
			if(ptr != 0) {
				argv[argIndex][ptr] = 0;
				argIndex++;
				ptr = 0;
			}
			argc = argIndex;
			res = execute(argc, argv);

			for(argIndex = 0; argIndex < argc; argIndex++) {
				os_sys_free(argv[argIndex]);
			}
			argIndex = 0;

			if((res == 0 ) | ( string[0] != ';')) return res;
			break;
		case ' ':
			//Allocate memory for the next argument
			// Checking that we haven't done that already (supporting multi-space)
			if(ptr != 0) {
				argv[argIndex][ptr] = 0;
				argIndex++;
				ptr = 0;

				argv[argIndex] = (char *) os_sys_malloc(MAX_ARG_LENGTH);
			}
			break;
		default:
			//Write symbol into arg
			argv[argIndex][ptr] = string[0];
			ptr++;
			break;
	}
	return parse_input(string + 1, argIndex, argv, ptr);
}

void run_shell() {
	os_sys_write(BEGIN_STRING);

	char *buffer = (char *) os_sys_malloc(255);
	char **args = (char **) os_sys_malloc(MAX_ARGS*sizeof(char*));
	do {
		os_sys_write("> ");
		os_sys_read(buffer, 255);
	} while(parse_input(buffer, 0, args, 0));
	os_sys_free(args);
}
