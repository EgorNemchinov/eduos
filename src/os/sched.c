#include <unistd.h>
#include <stdlib.h>
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
		if (sched_task_queue.tasks[i].state == SCHED_EMPTY) {
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
	os_exit(0);
}

static void task_init(struct sched_task *task) {
	ucontext_t *ctx = &task->ctx;
	const int stacksize = sizeof(task->stack);
	memset(ctx, 0, sizeof(*ctx));
	getcontext(ctx);

	ctx->uc_stack.ss_sp = task->stack + stacksize;
	ctx->uc_stack.ss_size = 0;
}

struct sched_task *sched_add(sched_task_entry_t entry, void *arg, priority_t priority) {
	struct sched_task *task = new_task();

	if (!task) {
		abort();
	}

	task_init(task);
	task->priority = priority;
	makecontext(&task->ctx, (void(*)(void)) task_tramp, 2, entry, arg);
	TAILQ_INSERT_TAIL(&sched_task_queue.head, task, link);

	return task;
}

void sched_remove_from_queue(struct sched_task *task) {
	task->state = SCHED_FINISH;
	TAILQ_REMOVE(&sched_task_queue.head, task, link);
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
	struct sched_task *task, *best_task = sched_task_queue.idle;
	TAILQ_FOREACH(task, &sched_task_queue.head, link) {
		if(task->priority > best_task->priority) {
			best_task = task;
		}
	}

	return best_task;
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
	task->priority = MIN_PRIORITY - 1;
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
