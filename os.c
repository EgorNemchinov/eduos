
#define _GNU_SOURCE

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <ucontext.h>
#include <sys/ucontext.h>

void run_shell();

typedef long(*sys_call_t)(int syscall,
		unsigned long arg1, unsigned long arg2,
		unsigned long arg3, unsigned long arg4,
		void *rest);

static long sys_write(int syscall,
		unsigned long arg1, unsigned long arg2,
		unsigned long arg3, unsigned long arg4,
		void *rest) {
	const char *msg = (const char *) arg1;
	return write(STDOUT_FILENO, msg, strlen(msg));
}

static long sys_read(int syscall,
		unsigned long arg1, unsigned long arg2,
		unsigned long arg3, unsigned long arg4,
		void *rest) {
	void *buffer = (void *) arg1;
	int bytes = (int) arg2;
	return read(STDIN_FILENO, buffer, bytes);
}

static long sys_malloc(int syscall, 
		unsigned long arg1, unsigned long arg2,
		unsigned long arg3, unsigned long arg4,
		void *rest) {
	int size = (int) arg1;
	return (long) malloc(size);
}

static long sys_free(int syscall,
		unsigned long arg1, unsigned long arg2,
		unsigned long arg3, unsigned long arg4,
		void *rest) {
	void *ptr = (void *) arg1;
	free(ptr);
	return 1;
}

static const sys_call_t sys_table[] = {
	sys_write, sys_read, sys_malloc, sys_free, 
};

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

long os_sys_write(const char *msg) {
	return os_syscall(0, (unsigned long) msg, 0, 0, 0, NULL);
}

long os_sys_read(char *buffer, int bytes) {
	return os_syscall(1, (unsigned long) buffer, bytes, 0, 0, NULL);
}

long os_sys_malloc(int size) {
	return os_syscall(2, (unsigned long) size, 0, 0, 0, NULL);
}

long os_sys_free(void *ptr) {
	return os_syscall(3, (unsigned long) ptr, 0, 0, 0, NULL);
}

int main(int argc, char *argv[]) {
	os_init();
	run_shell();
	return 0;
}
