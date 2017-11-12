
#ifndef EDUOS_OS_MUTEX_H
#define EDUOS_OS_MUTEX_H

struct mutex {

};

extern int mutex_init(struct mutex *m);

extern void mutex_lock(struct mutex *m);
extern void mutex_unlock(struct mutex *m);


#endif /* EDUOS_OS_MUTEX_H */
