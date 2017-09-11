
all : shell

clean :
	rm -f shell 1.out

test : shell
	./shell < 1.in > 1.out
	diff -au 1.out.gold 1.out
