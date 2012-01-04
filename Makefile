all: slopi

clean: slopi
	rm slopi

slopi: slopi.c Makefile
	gcc -Ofast -march=barcelona -std=c99 -Wall -Wno-trigraphs -Wno-unused-result -o slopi slopi.c modp_numtoa.c -lm -pthread
