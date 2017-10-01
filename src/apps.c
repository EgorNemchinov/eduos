
#include <string.h>

#include "os.h"
#include "apps.h"

extern char *strtok_r(char *str, const char *delim, char **saveptr);

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

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))

static int do_task(char *command) {
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
}

static struct shell_env {
	char buffer[256];
} g_shell_env;

static void shell_input(int bytes, void *arg);

static void shell_prompt(void *arg) {
	struct shell_env *env = (struct shell_env *) arg;
	os_sys_write("> ");
	os_sys_read(env->buffer, sizeof(env->buffer), shell_input, arg);
}

static void shell_input(int bytes, void *arg) {
	struct shell_env *env = (struct shell_env *) arg;

	if (!bytes) {
		os_sys_write("\n");
		return;
	}

	if (bytes < sizeof(env->buffer)) {
		env->buffer[bytes] = '\0';
	}

	char *saveptr;
	const char *comsep = "\n;";
	char *cmd = strtok_r(env->buffer, comsep, &saveptr);
	while (cmd) {
		do_task(cmd);
		cmd = strtok_r(NULL, comsep, &saveptr);
	}

	shell_prompt(arg);
}


void shell() {
	shell_prompt(&g_shell_env);
}
