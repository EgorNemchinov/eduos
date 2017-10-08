
#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>

#include "os.h"
#include "os/sched.h"
#include "os/irq.h"
#include "os/syscall.h"

typedef long(*sys_call_t)(int syscall,
		unsigned long arg1, unsigned long arg2,
		unsigned long arg3, unsigned long arg4,
		void *rest);

#define SYSCALL_X(x) \
	x(write) \
	x(read) \
	x(halt) \



#define ENUM_LIST(name) os_syscall_nr_ ## name,
enum syscalls_num {
	SYSCALL_X(ENUM_LIST)
};
#undef ENUM_LIST

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

static void read_irq_hnd(void *arg) {
	sched_notify(arg);
}

static long sys_read(int syscall,
		unsigned long arg1, unsigned long arg2,
		unsigned long arg3, unsigned long arg4,
		void *rest) {
	void *buffer = (void *) arg1;
	const int size = (int) arg2;

	irqmask_t cur = irq_disable();

	int bytes = errwrap(read(STDIN_FILENO, buffer, size));
	while (bytes == -EAGAIN) {
		irq_set_hnd(read_irq_hnd, sched_current());
		sched_wait();
		sched();
		bytes = errwrap(read(STDIN_FILENO, buffer, size));
	}

	irq_enable(cur);
	return bytes;
}

static long sys_halt(int syscall,
		unsigned long arg1, unsigned long arg2,
		unsigned long arg3, unsigned long arg4,
		void *rest) {
	exit(arg1);
}

#define TABLE_LIST(name) sys_ ## name,
static const sys_call_t sys_table[] = {
	SYSCALL_X(TABLE_LIST)
};
#undef TABLE_LIST

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

int os_sys_read(char *buffer, int size) {
	return os_syscall(os_syscall_nr_read, (unsigned long) buffer, size,
			0, 0, NULL);
}

int os_halt(int status) {
	return os_syscall(os_syscall_nr_halt, status, 0, 0, 0, NULL);
}

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
	} else {
		abort();
	}
}

int syscall_init(void) {
	struct sigaction act = {
		.sa_sigaction = os_sighnd,
		.sa_flags = SA_RESTART,
	};
	sigemptyset(&act.sa_mask);

	if (-1 == sigaction(SIGSEGV, &act, NULL)) {
		perror("signal set failed");
		exit(1);
	}
	return 0;
}
