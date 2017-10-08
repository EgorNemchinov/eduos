
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

#include "os.h"
#include "os/sched.h"
#include "os/irq.h"

static struct {
	struct sched_task tasks[256];
	TAILQ_HEAD(listhead, sched_task) head;
	struct sched_task *current;
	struct sched_task *idle;
} sched_task_queue;

static struct sched_task *new_task(void) {
	for (int i = 0; i < ARRAY_SIZE(sched_task_queue.tasks); ++i) {
		if (sched_task_queue.tasks[i].state == SCHED_FINISH) {
			return &sched_task_queue.tasks[i];
		}
	}
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
	irqmask_t irq = irq_disable();
	struct sched_task *task = new_task();
	task->state = SCHED_READY;
	irq_enable(irq);

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
}

void sched_wait(void) {
	sched_current()->state = SCHED_SLEEP;
}

struct sched_task *sched_current(void) {
	return sched_task_queue.current;
}

static struct sched_task *next_task(void) {
	struct sched_task *task;
	TAILQ_FOREACH(task, &sched_task_queue.head, link) {
		/* TODO only READY tasks in queue */
		/* TODO priority */
		if (task != sched_task_queue.idle && task->state == SCHED_READY) {
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
