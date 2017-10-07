#ifndef EDUOS_OS_IRQ_H
#define EDUOS_OS_IRQ_H

extern void unblock_sig(int sig);
extern int block_sig(int sig);
extern int irq_init(void);

extern void (*irq_hnd)(void);

#endif /* EDUOS_OS_IRQ_H */


