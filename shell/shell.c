#define _GNU_SOURCE

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

extern char *strtok_r(char *str, const char *delim, char **saveptr);

struct app {
	char *argv[256];
};

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))

static int split_command_by_pipeline(struct app apps[], char *command) {
	char *saveptr;
	char *arg = strtok_r(command, " ", &saveptr);
	int app_count = 0, app_argc = 0;
	while (arg) {
		if (!strcmp(arg, "|")) {
			apps[app_count].argv[app_argc++] = (char *)0;
			++app_count;
			app_argc = 0;
		} else {
			apps[app_count].argv[app_argc++] = arg;
		}
		arg = strtok_r(NULL, " ", &saveptr);
	}
	return app_count + 1;
}

static void do_task(char *argv[], int in_fd, int out_fd) {
	int status;
	if (fork() != 0) {
		waitpid(-1, &status, 0);
	} else {
		if (in_fd != 0) {
			dup2(in_fd, 0);
			close(in_fd);
		}
		if (out_fd != 1) {
			dup2(out_fd, 1);
			close(out_fd);
		}
		char pathMain[] = "/usr/bin/";
		char pathError[] = "/bin/";
		strcat(pathMain, argv[0]);
		strcat(pathError, argv[0]);
		if (execv(pathMain, argv) == -1) {
			if (execv(pathError, argv) == -1) {
				write(1, "No such function: ", 18);
				write(1, argv[0], strlen(argv[0]));
				write(1, "\n", 1);
			}
		}
	}	
}

static void do_pipeline(int app_count, struct app apps[]) {
	int pipefd[2];
	int in_fd = 0;
	for (int i = 0; i < app_count - 1; ++i) {
		pipe(pipefd);
		do_task(apps[i].argv, in_fd, pipefd[1]);
		close(pipefd[1]);
		in_fd = pipefd[0];
	}
	do_task(apps[app_count - 1].argv, in_fd, 1);
	in_fd = 0;
}

int main(int argc, char *argv[]) {
	while (1) {
		char buffer[256];
		size_t bufsize = 256;
		size_t bytes = read(0, buffer, bufsize);
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
			struct app apps[256] = {};
			int app_count = split_command_by_pipeline(apps, cmd);
			do_pipeline(app_count, apps);
			cmd = strtok_r(NULL, comsep, &saveptr);
		}
	}
	return 0;
}
