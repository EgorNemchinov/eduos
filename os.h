
#ifndef EDUOS_OS_H
#define EDUOS_OS_H

extern int os_sys_write(const char *msg);
extern int os_sys_read(char *buffer, int bytes);
extern int os_sys_malloc(int size);
extern int os_sys_free(void *ptr);

extern void printLine(const char *string);
extern const char* readLine();

#endif /* EDUOS_OS_H */
