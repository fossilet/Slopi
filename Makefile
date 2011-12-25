all: slopi itoa

clean: slopi itoa	
	rm slopi itoa

itoa: itoa.c
	gcc -o itoa itoa.c modp_numtoa.o
slopi: slopi.c Makefile
	gcc -Ofast -march=core2 -Wno-unused-result -o slopi slopi.c modp_numtoa.c -lm -pthread
