
#ifndef EDUOS_OS_SCHED_H
#define EDUOS_OS_SCHED_H

#include "third-party/queue.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))

enum sched_state {
	SCHED_FINISH,
	SCHED_READY,
	SCHED_SLEEP,
	SCHED_RUN,
};

struct sched_task {
	enum sched_state state;
	syshandler_t hnd;
	void *arg;
	int res;
	TAILQ_ENTRY(sched_task) link;
};

extern void sched_add(enum sched_state state, int res, syshandler_t hnd, void *arg);

extern void sched_notify(int res);

extern void sched_init(void);

extern void sched_loop(void);

#endif /* EDUOS_OS_SCHED_H */
