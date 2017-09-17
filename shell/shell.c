#include "string.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/wait.h>

#define MAX_ARGS 10
#define MAX_ARG_LENGTH 100
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))

const char *BEGIN_STRING = "Welcome, stranger!\n";
/*const char *HELP_STRING = "Possible commands:\n    app1(name='World') - prints 'Hello, $name$!'\n"
											  "    app2() - reads line, reverses it and prints back.\n"
											  "    quit or q - exits the shell.\n"
											  "For example, you can use a combination of these: 'app1 brother; app1; app2;'\n";
*/
static const struct {
	char *name;
	const char *path;
} app_list[] = {
	{ "echo", "/bin/echo" },
	{ "grep", "/bin/grep"},
	{ "seq", "/usr/bin/seq"},
};											  

const char* find_path(char *app_name) {
	int i;
	for(i = 0; i < ARRAY_SIZE(app_list); i++) {
		if(strcmp(app_list[i].name, app_name) == 0) {
			return app_list[i].path;
		}
	}
	write(STDOUT_FILENO, "No such app: \"", 14);
	write(STDOUT_FILENO, app_name, strlen(app_name));
	write(STDOUT_FILENO, "\".\n", 3);
	return NULL;
}

struct app {
	int argc;
	char *argv[256];
} app;

struct args {
	int count;
	struct app* apps[256];
};

//TODO: optimize
struct args to_array_of_apps(int argc, char *argv[]) {
	int i = 0, argPtr = 0;

	struct args result_apps = {.count = 0};
	struct app new_app = {.argc = 0};
	result_apps.apps[0] = &new_app;

	for(i = 0; i < argc; i++) {
		if(strcmp(argv[i], "|") == 0) {
			result_apps.count++;
			argPtr = 0;

			struct app new_app = {.argc = 0};
			result_apps.apps[result_apps.count] = &new_app;
		} else {
			result_apps.apps[result_apps.count]->argv[argPtr] = argv[i];
			result_apps.apps[result_apps.count]->argc++;
			argPtr++;
		}
	}
	result_apps.count++;
	return result_apps;
}

int execute_cmd(int argc, char *argv[], int in_fd, int out_fd) {
	int status;
	int pid = fork();
	if(pid < 0) {
		perror("fork");
		exit(1);
	} else if(pid == 0) {
		if(in_fd != STDIN_FILENO) {
			dup2(in_fd, STDIN_FILENO);
			close(in_fd);
		}
		if(out_fd != STDOUT_FILENO) {
			dup2(out_fd, STDOUT_FILENO);
			close(out_fd);
		}
		const char* path = find_path(argv[0]);
		if(path != NULL)
			execv(path, argv);
		else
			exit(1);
	} else {
		waitpid(-1, &status, 0);
		// printf("%d", status);
	}
	return 0;
}

void pipeline(int amount, struct app *apps[]) {
	int i, pid, status;
	int prev_fd[2], cur_fd[2];
	for(i = 0; i < amount; i++) {
		if(i != amount - 1) {
			if(pipe(cur_fd) < 0) {
				perror("pipe");
				exit(1);
			}
		}

		pid = fork();
		if(pid < 0) {
			perror("fork");
			exit(1);
		} else if(pid == 0) {
			if(i != 0) {
				if(dup2(prev_fd[0], 0) < 0) {
					perror("dup2");
					exit(1);
				}
				close(prev_fd[0]);
				close(prev_fd[1]);
			}
			if(i != amount-1) {
				close(cur_fd[0]);
				if(dup2(cur_fd[1], 1) < 0) {
					perror("dup2");
					exit(1);
				}
				close(cur_fd[1]);
			}
			const char* path = find_path(apps[i]->argv[0]);
			if(path != NULL)
				execv(path, apps[i]->argv);
			else
				exit(1);
			perror("execv");
			exit(1);
		} else {
			if(i != 0) {
				close(prev_fd[0]);
				close(prev_fd[1]);
			}
			if(i != amount - 1) {
				prev_fd[0] = cur_fd[0];
				prev_fd[1] = cur_fd[1];
			}
			waitpid(-1, &status, 0);
		}
	}
	for(i = 0; i < 2; i++) {
		close(prev_fd[i]);
		close(cur_fd[i]);
	}

}

int execute(int argc, char *argv[], bool piping) {
	if((argc == 1) & ((!strcmp(argv[0], "q")) | (!strcmp(argv[0], "quit")))) {
		write(STDOUT_FILENO, "Bye!", 4);
		return 0;
	}
	//TODO: help, quit etc.
	if(piping) {
		struct args arguments = to_array_of_apps(argc, argv);
		pipeline(arguments.count, arguments.apps);
	} else {
		execute_cmd(argc, argv, STDIN_FILENO, STDOUT_FILENO);
	}
	return 1;
}

// Recursive function, each time passing into *string unprocessed part of input line
int parse_input(const char *string, int argIndex, char *argv[], int ptr, bool piping) {
	if((argIndex == 0 ) & (ptr == 0)) {
		argv[0] = (char *) malloc(MAX_ARG_LENGTH);
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
			res = execute(argc, argv, piping);

			for(argIndex = 0; argIndex < argc; argIndex++) {
				free(argv[argIndex]);
			}
			argIndex = 0;

			if((res == 0) | ( string[0] != ';')) return res;
			break;
		case ' ':
			//Allocate memory for the next argument
			// Checking that we haven't done that already (supporting multi-space)
			if(ptr != 0) {
				argv[argIndex][ptr] = 0;
				argIndex++;
				ptr = 0;

				argv[argIndex] = (char *) malloc(MAX_ARG_LENGTH);
			}
			break;
		case '|':
			piping = true;
		default:
			//Write symbol into arg
			argv[argIndex][ptr] = string[0];
			ptr++;
			break;
	}
	return parse_input(string + 1, argIndex, argv, ptr, piping);
}

void run_shell() {
	write(STDOUT_FILENO, BEGIN_STRING, strlen(BEGIN_STRING));
	
	char *buffer = (char *) malloc(255);
	char **args = (char **) malloc(MAX_ARGS*sizeof(char*));
	do {
		write(STDOUT_FILENO, "> ", 2);
		read(STDIN_FILENO, buffer, 255);
	} while(parse_input(buffer, 0, args, 0, false));
	free(args);
	free(buffer);
}

int main(int argc, char *argv[]) {
	run_shell();
	return 0;
}
