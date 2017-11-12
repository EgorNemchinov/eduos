
#include <errno.h>

#include <stdbool.h>

#include "os/sem.h"

struct sem {
	bool used;
	/* TODO */
};

struct sem g_sem;

int sem_init(int cnt) {
	if (g_sem.used) {
		return -ENOMEM;
	}
	return 0;
}

int sem_use(int semid, int add) {
	if (semid != 0 || !g_sem.used) {
		return -EINVAL;
	}

	return 0;
}

int sem_free(int semid) {
	if (semid != 0 || !g_sem.used) {
		return -EINVAL;
	}
	g_sem.used = false;
	return 0;
}

