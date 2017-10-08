
#define _GNU_SOURCE

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>

#include "os/time.h"

static void os_sigalrmhnd(int signal, siginfo_t *info, void *ctx) {
	struct itimerval cur_it;
	getitimer(ITIMER_REAL, &cur_it);
	/*fprintf(stderr, "%s: tv_usec=%ld\n", __func__, cur_it.it_value.tv_usec);*/
}


void time_init(void) {
	struct sigaction alrmact = {
		.sa_sigaction = os_sigalrmhnd,
	};
	sigemptyset(&alrmact.sa_mask);
	if (-1 == sigaction(SIGALRM, &alrmact, NULL)) {
		perror("SIGALRM set failed");
		exit(1);
	}

	const struct itimerval setup_it = {
		.it_interval = { 1 /*sec*/, 0 /*usec*/},
		.it_value    = { 1 /*sec*/, 0 /*usec*/},
	};
	if (-1 == setitimer(ITIMER_REAL, &setup_it, NULL)) {
		perror("SIGALRM set failed");
		exit(1);
	}
}
