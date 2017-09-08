
#include "os.h"

void app1(void) {
	const char *msg = "Hello, World!\n";
	os_sys_write(msg);
}

void app2(void) {
	int size = 255;
	char *buffer = (char *) malloc(size*sizeof(char));
	int readBytes = os_sys_read(buffer, size*sizeof(char));
	os_sys_write(buffer);
}