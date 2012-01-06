<<<<<<< HEAD
all: slopi

clean: slopi
	rm slopi

slopi: slopi.amd

slopi.amd: slopi.c Makefile
	gcc -Ofast -march=barcelona -std=c99 -Wall -Wno-trigraphs -Wno-unused-result -o slopi slopi.c modp_numtoa.c -lm -pthread

slopi.intel: slopi.c Makefile
	gcc -Ofast -march=core2 -std=c99 -Wall -Wno-trigraphs -Wno-unused-result -o slopi slopi.c modp_numtoa.c -lm -pthread

slopi.oldgcc: slopi.c Makefile
	gcc -O3 -std=c99 -Wall -o slopi slopi.c modp_numtoa.c -lm -pthread
=======
all: slopi 

clean: slopi 
	rm slopi

slopi: slopi.c Makefile
	gcc -O3 -o slopi slopi.c modp_numtoa.c -lm -pthread
>>>>>>> th507/master
