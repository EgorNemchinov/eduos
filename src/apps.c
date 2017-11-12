
#include <string.h>
#include <stdio.h>

#include <stdbool.h>

#include "os.h"

#include "apps.h"
#include <inttypes.h>

#include "util.h"

extern char *strtok_r(char *str, const char *delim, char **saveptr);

static int echo(int argc, char *argv[]) {
	for (int i = 1; i < argc; ++i) {
		os_sys_write(argv[i]);
		os_sys_write(i == argc - 1 ? "\n" : " ");
	}
	return 0;
}

static int sleep(int argc, char *argv[]) {
	int num = (int) strtoumax(argv[1], NULL, 10);
	os_sleep(num);
	return 0;
}

static int uptime(int argc, char *argv[]) {
	char time_string[20];
	double uptime = (double) os_uptime() / 1000000;
	snprintf(time_string, ARRAY_SIZE(time_string), "%f", uptime);
	strcat(time_string, "\n");
	os_sys_write(time_string);
	return 0;
}

struct mutex_test_arg {
	int semid;
	int cnt;
	bool fin;
};

static void mutex_test_task(void *_arg) {
	struct mutex_test_arg *arg = _arg;
	char msg[256];

	while (!arg->fin) {
		snprintf(msg, sizeof(msg), "%s: %d\n", __func__, os_task_id());
		os_sys_write(msg);

		os_sem_use(arg->semid, -1);

		if (arg->cnt) {
			os_sys_write("multiple process in critical section!");
			os_halt(1);
		}

		++arg->cnt;
		os_wait();
		--arg->cnt;

		os_sem_use(arg->semid, +1);
	}

	/*os_exit();*/
}

static int mutex_test(int argc, char *argv[]) {
	struct mutex_test_arg arg;

	arg.semid = os_sem_init(1);
	arg.cnt = 0;
	arg.fin = false;

	int t1 = os_clone(mutex_test_task, &arg);
	int t2 = os_clone(mutex_test_task, &arg);

	for (int i = 0; i < 10; i++) {
		os_wait();
	}
	arg.fin = true;

	os_waitpid(t1);
	os_waitpid(t2);

	return 0;
}

static const struct {
	const char *name;
	int(*fn)(int, char *[]);
} app_list[] = {
	{ "echo", echo },
	{ "sleep", sleep },
	{ "uptime", uptime },
	{ "mutex_test", mutex_test },
};

struct params {
	char *cmd;
};

static void do_task(void *args) {
	char *saveptr;
	struct params *p = (struct params*) args;
	char *arg = strtok_r(p->cmd, " ", &saveptr);
	char *argv[256];
	int argc = 0;
	while (arg) {
		argv[argc++] = arg;
		arg = strtok_r(NULL, " ", &saveptr);
	}

	for (int i = 0; i < ARRAY_SIZE(app_list); ++i) {
		if (!strcmp(argv[0], app_list[i].name)) {
			os_exit(app_list[i].fn(argc, argv));
			return;
		}
	}

	char msg[256] = "No such function: ";
	strcat(msg, argv[0]);
	strcat(msg, "\n");
	os_sys_write(msg);
	return;
}

void shell(void *args) {
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
		struct params args;
		args.cmd = strtok_r(buffer, comsep, &saveptr);
		while (args.cmd) {

			int task_id = os_clone(do_task, (void *)&args);

			int status = os_waitpid(task_id);
			if(status != 0) {
				os_halt(status);
			}

			args.cmd = strtok_r(NULL, comsep, &saveptr);
		}
	}

	os_sys_write("\n");
	os_halt(0);
}
