
#define _GNU_SOURCE

#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "os.h"
#include "os/irq.h"

void (*irq_hnd)(void);

static void os_sigiohnd(int sig, siginfo_t *info, void *ctx) {
	if (irq_hnd) {
		irq_hnd();
	}
}

int block_sig(int sig) {
	sigset_t newmask, oldmask;
	sigemptyset(&newmask);
	sigaddset(&newmask, sig);

	if (-1 == sigprocmask(SIG_BLOCK, &newmask, &oldmask)) {
		perror("block_sig: sigprocmask");
		exit(1);
	}

	return sigismember(&newmask, sig) ? sig : 0;
}

void unblock_sig(int sig) {
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

int irq_init(void) {
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
	return 0;
}

