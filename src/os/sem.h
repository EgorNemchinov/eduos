#ifndef EDUOS_OS_SEM_H
#define EDUOS_OS_SEM_H

extern int sem_init(int cnt);

extern int sem_use(int semid, int add);

extern int sem_free(int semid);

#endif /* EDUOS_OS_SEM_H */

