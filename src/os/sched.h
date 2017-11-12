
#ifndef EDUOS_OS_SCHED_H
#define EDUOS_OS_SCHED_H

#include "third-party/queue.h"

#include <ucontext.h>

#define priority_t unsigned char
#define MAX_PRIORITY 255
#define MID_PRIORITY 127
#define MIN_PRIORITY 1

enum sched_state {
	SCHED_EMPTY,
	SCHED_FINISH,
	SCHED_READY,
	SCHED_SLEEP,
	SCHED_RUN,
};

typedef void (*sched_task_entry_t)(void *arg);

struct sched_task {
	TAILQ_ENTRY(sched_task) link;
	ucontext_t ctx;
	char stack[0x10000];
	enum sched_state state;
	struct sched_task *parent;
	priority_t priority;
	int exit_status;
};

extern int get_task_id(struct sched_task *task);
extern struct sched_task *get_task(int task_id);
extern void remove_task(struct sched_task *task);

extern struct sched_task *sched_add(sched_task_entry_t entry, void *arg, priority_t priority);
extern void sched_wait(void);
extern void sched_notify(struct sched_task *task);

extern struct sched_task *sched_current(void);
extern int sched_user_id(struct sched_task *task);

extern void sched(void);

extern void sched_init(void);
extern void sched_loop(void);

#endif /* EDUOS_OS_SCHED_H */
