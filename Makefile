
all : image

apps.o : CFLAGS = -ffreestanding --sysroot=/tmp -Wimplicit-function-declaration -Werror

image : os.o apps.o shell.o
	$(CC) -o $@ $^

clean :
	rm -f image *.o
