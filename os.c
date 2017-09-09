
#define _GNU_SOURCE

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <ucontext.h>
#include <sys/ucontext.h>

typedef int(*sys_call_t)(int syscall,
		unsigned long arg1, unsigned long arg2,
		unsigned long arg3, unsigned long arg4,
		void *rest);

int sys_write(int syscall,
		unsigned long arg1, unsigned long arg2,
		unsigned long arg3, unsigned long arg4,
		void *rest) {
	const char *msg = (const char *) arg1;
	return write(STDOUT_FILENO, msg, strlen(msg));
}
int sys_read(int syscall,
		unsigned long arg1, unsigned long arg2,
		unsigned long arg3, unsigned long arg4,
		void *rest) {
	void *buf = (void *) arg1;
	int bytes_amount = (int) arg2;
	return read(STDIN_FILENO, buf, bytes_amount);
}

static const sys_call_t sys_table[] = {
	sys_write,
	sys_read,
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

static int os_syscall(int syscall,
		unsigned long arg1, unsigned long arg2,
		unsigned long arg3, unsigned long arg4,
		void *rest) {
	int ret;
	__asm__ __volatile__(
		"int $0x81\n"
		: "=a"(ret)
		: "a"(syscall), "b"(arg1), "c"(arg2), "d"(arg3), "S"(arg4), "D"(rest)
		:
	);
	return ret;
}

int os_sys_write(const char *msg) {
	return os_syscall(0, (unsigned long) msg, 0, 0, 0, NULL);
}
int os_sys_read(char *buf, int bytes_amount) {
	return os_syscall(1, (unsigned long) buf, (unsigned long) bytes_amount, 0, 0, NULL);
}

int main(int argc, char *argv[]) {
	os_init();
	shell();
	return 0;
}

int shell(void) {
	while (1) {
		const int string_size = 255;
		char buf[string_size];
		int actual_size = os_sys_read(buf, string_size * sizeof(char));
		buf[actual_size - 1] = ' ';
		buf[actual_size] = '\0'; 

		char* calls[30];
		int calls_amount = 0;
		char* call = strtok(buf, ";");
		while (call != NULL) {
			calls[calls_amount++] = call;
			call = strtok(NULL, ";");
		}

		for (int i = 0; i < calls_amount; i++) {
			char* words[30];
			int words_amount = 0;
			char* word = strtok(calls[i], " ");
			while (word != NULL) {
				words[words_amount++] = word;
				word = strtok(NULL, " ");
			}

			if (strcmp(words[0], "app1") == 0) {
				app1(words_amount, words);
			}
		}
	}
}