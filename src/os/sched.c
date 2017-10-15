#include <unistd.h>
#include <stdlib.h>
<<<<<<< HEAD
#include <string.h>

#include "util.h"
#include "assert.h"

#include "os.h"
#include "os/sched.h"
#include "os/irq.h"

static struct {
	struct sched_task tasks[256];
	TAILQ_HEAD(listhead, sched_task) head;
	struct sched_task *current;
	struct sched_task *idle;
} sched_task_queue;

struct sched_task *get_task_by_id(int id) {
	return &sched_task_queue.tasks[id];
}

static struct sched_task *new_task(void) {
	irqmask_t irq = irq_disable();	
	for (int i = 0; i < ARRAY_SIZE(sched_task_queue.tasks); ++i) {
		if (sched_task_queue.tasks[i].state == SCHED_FINISH) {
			sched_task_queue.tasks[i].state = SCHED_READY;
			sched_task_queue.tasks[i].id = i;
			irq_enable(irq);
			return &sched_task_queue.tasks[i];
		}
	}
	irq_enable(irq);	
	return NULL;
}

void task_tramp(sched_task_entry_t entry, void *arg) {
	irq_enable(IRQ_ALL);
	entry(arg);
	abort();
}

static void task_init(struct sched_task *task) {
	ucontext_t *ctx = &task->ctx;
	const int stacksize = sizeof(task->stack);
	memset(ctx, 0, sizeof(*ctx));
	getcontext(ctx);

	ctx->uc_stack.ss_sp = task->stack + stacksize;
	ctx->uc_stack.ss_size = 0;
}

struct sched_task *sched_add(sched_task_entry_t entry, void *arg) {
	struct sched_task *task = new_task();

	if (!task) {
		abort();
	}

	task_init(task);
	makecontext(&task->ctx, (void(*)(void)) task_tramp, 2, entry, arg);
	TAILQ_INSERT_TAIL(&sched_task_queue.head, task, link);

	return task;
}

void sched_notify(struct sched_task *task) {
	task->state = SCHED_READY;
	//TODO: inserty by priority
	TAILQ_INSERT_TAIL(&sched_task_queue.head, task, link);
}

void sched_wait(void) {
	//TODO: check if irq disabled
	sched_current()->state = SCHED_SLEEP;
	TAILQ_REMOVE(&sched_task_queue.head, sched_current(), link);
}

struct sched_task *sched_current(void) {
	return sched_task_queue.current;
}

static struct sched_task *next_task(void) {
	struct sched_task *task;
	TAILQ_FOREACH(task, &sched_task_queue.head, link) {
		assert(task->state == SCHED_READY);
		/* TODO priority */
		if (task != sched_task_queue.idle) {
			return task;
		}
	}

	return sched_task_queue.idle;
}

void sched(void) {
	irqmask_t irq = irq_disable();

	struct sched_task *cur = sched_current();
	struct sched_task *next = next_task();

	if (cur != next) {
		sched_task_queue.current = next;
		swapcontext(&cur->ctx, &next->ctx);
	}

	irq_enable(irq);
}

void sched_init(void) {
	TAILQ_INIT(&sched_task_queue.head);

	struct sched_task *task = new_task();
	task_init(task);
	task->state = SCHED_READY;
	TAILQ_INSERT_TAIL(&sched_task_queue.head, task, link);

	sched_task_queue.idle = task;
	sched_task_queue.current = task;
}

void sched_loop(void) {
	irq_enable(IRQ_ALL);

	sched();

	while (1) {
		pause();
	}
}
=======

#include "os.h"
#include "os/sched.h"

static struct sched_task tasks[100];

TAILQ_HEAD(listhead, sched_task) free_blocks;
struct listhead ready_tasks;
struct listhead sleeping_tasks;

//init free with all blocks, but sleep and ready empty
void sched_init(void) {
	TAILQ_INIT(&free_blocks);
	TAILQ_INIT(&ready_tasks);
	TAILQ_INIT(&sleeping_tasks);

	int i;
	for (i = 0; i < ARRAY_SIZE(tasks); i++) {
		struct sched_task *empty = &tasks[i];
		empty->state = SCHED_FINISH;
		TAILQ_INSERT_TAIL(&free_blocks, empty, link);
	}
}

void sched_add(enum sched_state state, int res, syshandler_t hnd, void *arg) {
	if(TAILQ_EMPTY(&free_blocks)) {
		//perror("no free tasks left");
		exit(1);
	}

	struct sched_task *new = TAILQ_FIRST(&free_blocks);
	TAILQ_REMOVE(&free_blocks, new, link);
	new->state = state;
	new->res = res;
	new->hnd = hnd;
	new->arg = arg;

	//Adding to corresponding list
	switch(state) {
		case SCHED_SLEEP:
			TAILQ_INSERT_HEAD(&sleeping_tasks, new, link);	
			break;
		case SCHED_READY:
			TAILQ_INSERT_HEAD(&ready_tasks, new, link);
			break;
		case SCHED_RUN:
			//perror("not allowed state RUN in sched_add()");
			exit(1);
		case SCHED_FINISH:
			//perror("not allowed state FINISH in sched_add()");
			exit(1);
	}
}

void sched_notify(int res) {
	struct sched_task *cur, *temp;
	TAILQ_FOREACH_SAFE(cur, &sleeping_tasks, link, temp) {
		cur->state = SCHED_READY;
		cur->res = res;
		TAILQ_REMOVE(&sleeping_tasks, cur, link); 
		TAILQ_INSERT_TAIL(&ready_tasks, cur, link);
	}
}

void sched_loop(void) {
	while(1) {
		if(TAILQ_EMPTY(&ready_tasks)) {
			if(TAILQ_EMPTY(&sleeping_tasks)) //no tasks at all
				return;
			else //tasks are not ready yet
				pause();
		}
		struct sched_task *task, *next_task;
		TAILQ_FOREACH_SAFE(task, &ready_tasks, link, next_task) {
			task->state = SCHED_FINISH;		
			TAILQ_REMOVE(&ready_tasks, task, link);
			task->hnd(task->res, task->arg);
			TAILQ_INSERT_TAIL(&free_blocks, task, link);
		}
	}
}
>>>>>>> c9f4369... Add split os files (somehow they got lost previously).
