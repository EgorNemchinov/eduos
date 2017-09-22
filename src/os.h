#ifndef EDUOS_OS_H
#define EDUOS_OS_H

extern int os_sys_write(const char *msg);

typedef void (*syshandler_t)(int res, void *arg);
extern int os_sys_read(char *buffer, int size, syshandler_t hnd, void *arg);

#endif /* EDUOS_OS_H */
