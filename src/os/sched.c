
#include <unistd.h>
#include <stdlib.h>

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