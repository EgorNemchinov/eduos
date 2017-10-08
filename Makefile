all : image

#src/apps.o : CFLAGS = -ffreestanding --sysroot=/tmp -Wimplicit-function-declaration -Werror

ifneq ($(DEBUG),)
CFLAGS += -g
LDFLAGS += -g
endif

CC = gcc
CFLAGS += -std=c99 -I$(PWD) -Wall -Werror

KERNEL = os os/irq os/syscall os/sched os/asm_sched os/time

$(KERNEL:%=src/%.o) : CFLAGS += -I$(PWD)/src

image : $(KERNEL:%=src/%.o) src/apps.o
	$(CC) $(LDFLAGS) -o $@ $^

clean :
	rm -f image
	find -name '*.o' -delete
