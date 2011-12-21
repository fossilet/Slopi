all: slopi

clean: slopi	
	rm slopi

slopi: slopi.c Makefile
	gcc -Ofast -march=core2 -Wno-unused-result -o slopi slopi.c modp_numtoa.o -lm -pthread
