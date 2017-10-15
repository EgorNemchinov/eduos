#ifndef EDUOS_OS_H
#define EDUOS_OS_H

struct args {
    int argc;
    char *argv;
    int res;
};

extern int os_sys_write(const char *msg);

extern int os_sys_read(char *buffer, int size);

extern int os_halt(int status);

extern int os_clone(void (*fn) (void *arg), void *arg);

extern int os_waitpid(int taskid);

extern int os_exit();

#endif /* EDUOS_OS_H */
