#ifndef EDUOS_OS_IRQ_H
#define EDUOS_OS_IRQ_H

#define IRQ_MAX_SHIFT 32
#define IRQ_ALL 0

typedef unsigned long irqmask_t;

extern irqmask_t irq_disable(void);
extern void irq_enable(irqmask_t mask);

extern int irq_init(void);

typedef void (*irq_hnd_t)(void *arg);

extern void irq_set_hnd(irq_hnd_t hnd, void *arg);

#endif /* EDUOS_OS_IRQ_H */


