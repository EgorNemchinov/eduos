
#define _GNU_SOURCE

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <ucontext.h>
#include <sys/ucontext.h>

#include "third-party/queue.h"

#include "os.h"
#include "apps.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))

#define SYSCALL_X(x) \
	x(write) \
	x(read) \


#define ENUM_LIST(name) os_syscall_nr_ ## name,
enum syscalls_num {
	SYSCALL_X(ENUM_LIST)
};
#undef ENUM_LIST

typedef long(*sys_call_t)(int syscall,
		unsigned long arg1, unsigned long arg2,
		unsigned long arg3, unsigned long arg4,
		void *rest);

static long errwrap(long res) {
	return res == -1 ? -errno : res;
}

static long sys_write(int syscall,
		unsigned long arg1, unsigned long arg2,
		unsigned long arg3, unsigned long arg4,
		void *rest) {
	const char *msg = (const char *) arg1;
	return errwrap(write(STDOUT_FILENO, msg, strlen(msg)));
}

static int block_sig(int sig) {
	sigset_t newmask, oldmask;
	sigemptyset(&newmask);
	sigaddset(&newmask, sig);

	if (-1 == sigprocmask(SIG_BLOCK, &newmask, &oldmask)) {
		perror("block_sig: sigprocmask");
		exit(1);
	}

	return sigismember(&newmask, sig) ? sig : 0;
}

static void unblock_sig(int sig) {
	if (!sig) {
		return;
	}

	sigset_t newmask;
	sigemptyset(&newmask);
	sigaddset(&newmask, sig);

	if (-1 == sigprocmask(SIG_UNBLOCK, &newmask, NULL)) {
		perror("block_sig: sigprocmask");
		exit(1);
	}
}

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
		perror("no free tasks left");
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
			perror("not allowed state RUN in sched_add()");
			exit(1);
		case SCHED_FINISH:
			perror("not allowed state FINISH in sched_add()");
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
	free(temp);
	free(cur);
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

static struct {
	void *buffer;
	int size;
} g_posted_read;

static long sys_read(int syscall,
		unsigned long arg1, unsigned long arg2,
		unsigned long arg3, unsigned long arg4,
		void *rest) {
	void *buffer = (void *) arg1;
	const int size = (int) arg2;
	const syshandler_t hnd = (syshandler_t) arg3;
	void *const arg = (void *) arg4;


	int sig = block_sig(SIGIO);
	assert("sig should be enabled at that point" && sig == SIGIO);

	int bytes = errwrap(read(STDIN_FILENO, buffer, size));
	if (bytes == -EAGAIN) {
		g_posted_read.buffer = buffer;
		g_posted_read.size = size;
		sched_add(SCHED_SLEEP, 0, hnd, arg);
	} else {
		sched_add(SCHED_READY, bytes, hnd, arg);
	}

	unblock_sig(sig);
	return bytes;
}

#define TABLE_LIST(name) sys_ ## name,
static const sys_call_t sys_table[] = {
	SYSCALL_X(TABLE_LIST)
};
#undef TABLE_LIST

static void os_sighnd(int sig, siginfo_t *info, void *ctx) {
	ucontext_t *uc = (ucontext_t *) ctx;
	greg_t *regs = uc->uc_mcontext.gregs;

	if (0x81cd == *(uint16_t *) regs[REG_RIP]) {
		int ret = sys_table[regs[REG_RAX]](regs[REG_RAX],
				regs[REG_RBX], regs[REG_RCX],
				regs[REG_RDX], regs[REG_RSI],
				(void *) regs[REG_RDI]);
		regs[REG_RAX] = ret;
		regs[REG_RIP] += 2;
	}
}

static void os_sigiohnd(int sig, siginfo_t *info, void *ctx) {
	int bytes = errwrap(read(STDIN_FILENO, g_posted_read.buffer, g_posted_read.size));
	if (bytes != -EAGAIN) {
		sched_notify(bytes);
	}
}

static void os_init(void) {
	struct sigaction act = {
		.sa_sigaction = os_sighnd,
		.sa_flags = SA_RESTART,
	};
	sigemptyset(&act.sa_mask);

	if (-1 == sigaction(SIGSEGV, &act, NULL)) {
		perror("signal set failed");
		exit(1);
	}

	struct sigaction actio = {
		.sa_sigaction = os_sigiohnd,
		.sa_flags = SA_RESTART,
	};
	sigemptyset(&actio.sa_mask);
	if (-1 == sigaction(SIGIO, &actio, NULL)) {
		perror("SIGIO set failed");
		exit(1);
	}

	if (-1 == fcntl(STDIN_FILENO, F_SETOWN, getpid())) {
		perror("fcntl SETOWN");
		exit(1);
	}

	int flags;
	if (-1 == (flags = fcntl(STDIN_FILENO, F_GETFL))) {
		perror("fcntl GETFL");
		exit(1);
	}

	flags |= O_NONBLOCK | O_ASYNC;
	if (-1 == fcntl(STDIN_FILENO, F_SETFL, flags)) {
		perror("fcntl SETFL");
		exit(1);
	}
}

static long os_syscall(int syscall,
		unsigned long arg1, unsigned long arg2,
		unsigned long arg3, unsigned long arg4,
		void *rest) {
	long ret;
	__asm__ __volatile__(
		"int $0x81\n"
		: "=a"(ret)
		: "a"(syscall), "b"(arg1), "c"(arg2), "d"(arg3), "S"(arg4), "D"(rest)
		:
	);
	return ret;
}

int os_sys_write(const char *msg) {
	return os_syscall(os_syscall_nr_write, (unsigned long) msg, 0, 0, 0, NULL);
}


int os_sys_read(char *buffer, int size, syshandler_t hnd, void *arg) {
	return os_syscall(os_syscall_nr_read, (unsigned long) buffer, size,
			(unsigned long) hnd, (unsigned long) arg, NULL);
}

int main(int argc, char *argv[]) {
	os_init();
	sched_init();
	shell();
	sched_loop();
	return 0;
}
