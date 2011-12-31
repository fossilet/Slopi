all: slopi

clean: slopi
	rm slopi

slopi: slopi.c Makefile
	gcc -Ofast -march=core2 -Wall -Wno-unused-result -o slopi slopi.c modp_numtoa.c -lm -pthread
