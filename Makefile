
image : os.o apps.o shell.o
	$(CC) -o $@ $^
clean :
	rm -f image *.o
