all : image

#src/apps.o : CFLAGS = -ffreestanding --sysroot=/tmp -Wimplicit-function-declaration -Werror

CC = gcc
CFLAGS = -std=c99 -I$(PWD) -Wall -Werror

KERNEL = os os/irq os/syscall os/sched os/time

$(KERNEL:%=src/%.o) : CFLAGS += -I$(PWD)/src

image : $(KERNEL:%=src/%.o) src/apps.o
	$(CC) $(LDFLAGS) -o $@ $^

clean :
	rm -f image
	find -name '*.o' -delete
