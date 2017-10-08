#ifndef EDUOS_OS_H
#define EDUOS_OS_H

extern int os_sys_write(const char *msg);

extern int os_sys_read(char *buffer, int size);

extern int os_halt(int status);

#endif /* EDUOS_OS_H */
