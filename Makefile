
all : image

#src/apps.o : CFLAGS = -ffreestanding --sysroot=/tmp -Wimplicit-function-declaration -Werror

CC = gcc
CFLAGS = -std=c99 -Wall -Werror

image : src/os.o src/apps.o
	$(CC) $(LDFLAGS) -o $@ $^

clean :
	rm -f image
	find -name '*.o' -delete
