all: slopi 

clean: slopi 
	rm slopi

slopi: slopi.c Makefile
	clear
	gcc -O3 -o slopi slopi.c modp_numtoa.c -lm -pthread
