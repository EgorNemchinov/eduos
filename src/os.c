
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

#include "os.h"
#include "apps.h"

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

static int g_have_input;

static void do_other_things(void) {
	while (!g_have_input) {
		usleep(500);
	}
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

static long sys_read(int syscall,
		unsigned long arg1, unsigned long arg2,
		unsigned long arg3, unsigned long arg4,
		void *rest) {
	void *buffer = (void *) arg1;
	const int size = (int) arg2;

	int sig = block_sig(SIGIO);
	assert("sig should be enabled at that point" && sig == SIGIO);

	int bytes = errwrap(read(STDIN_FILENO, buffer, size));
	while (bytes == -EAGAIN) {
		g_have_input = 0;
		unblock_sig(sig);
		do_other_things();
		sig = block_sig(SIGIO);
		bytes = errwrap(read(STDIN_FILENO, buffer, size));
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
	g_have_input = 1;
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

int os_sys_read(char *buffer, int size) {
	return os_syscall(os_syscall_nr_read, (unsigned long) buffer, size, 0, 0, NULL);
}

int main(int argc, char *argv[]) {
	os_init();
	shell();
	return 0;
}
