#ifndef EDUOS_OS_H
#define EDUOS_OS_H

struct args {
    int argc;
    char *argv;
    int res;
};

extern int os_sys_write(const char *msg);

extern int os_sys_read(char *buffer, int size);

extern int os_clone(void (*fn) (void *arg), void *arg);

extern int os_waitpid(int task_id);

extern int os_halt(int status);

extern int os_exit(int status);

extern int os_wait(void);

extern int os_task_id(void);

extern int os_sem_init(int cnt);

extern int os_sem_use(int semid, int add);

extern int os_sem_free(int semid);

#endif /* EDUOS_OS_H */
