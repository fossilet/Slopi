all: goo

clean: goo	
	rm goo

goo: goo.c Makefile
	gcc -Ofast -march=core2 -Wno-unused-result -o goo goo.c modp_numtoa.o -lm -pthread
