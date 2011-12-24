/* Find self-locating numbers in numbers, esp. in Pi.
 * For speed consideration, we do not check call failures.
 * modp_numtoa is from stringencoders at 
 *     https://code.google.com/p/stringencoders/wiki/NumToA
 * Since Dec 7 2011
 */

#define _GNU_SOURCE
#include "modp_numtoa.h"
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define OFFSET 2 // How many bytes to bypass. For 3.1415... OFFSET is 2.
#define err_exit(en, msg) do { errno=en; perror(msg); exit(EXIT_FAILURE); } while (0)

static void *find(void *file) {
    // set CPU affinity
    // FIXME fixed two threads onto the same core, slows down.
    /*
    int s;
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(1, &set);
    s = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &set);
    if(s != 0)
        err_exit(s, "pthread_setaffinity_np");
        */

    char *infile = (char *)file;
    struct stat stat_buf;
    int fd;
    int N;
    int num_read;

    stat(infile, &stat_buf);
    N = stat_buf.st_size;
    fd = open(infile, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "File %s open error!\n", infile);
        exit(1);
    }

    int maxlen = (int)ceil(log10f((float)N));
    char buf[2*maxlen];
    //printf("maxlen: %d\n", maxlen);
    int n = 1;
    int len = 1;
    sprintf(buf, "%d", 1);
    // FIXME detect error
    lseek(fd, OFFSET, SEEK_SET);
    do {
        num_read = read(fd, buf+len, len);
        //printf("buf: %.*s, len: %d, n: %d\n", 2*len, buf, len, n);
        if(!memcmp(buf, buf+len, len))
            printf("%.*s\t\t\t%lx\n", \
                    len, buf, (unsigned long)(pthread_self()));
        modp_itoa10(n+1, buf); // XXX Only support up to int32
        len = strlen(buf);
    } while((n=lseek(fd, n+OFFSET, SEEK_SET)-OFFSET+1) < N);
    //   ^ off_t is signed int by SUSv3 type requirement
}

int main(int argc, char *argv[]) {
    // FIXME detect error
    long num_cpu =  sysconf(_SC_NPROCESSORS_ONLN);
    printf("CPU number: %ld\n", num_cpu);
    char *files[num_cpu];

    if(num_cpu == 2) {
        // FIXME initialize once
        files[0] = "xaa";
        files[1] = "xab";
    }
    //const char * const files[num_cpu] = {"xac", "xad"};

    //char *files[argc-1][num_cpu];
    pthread_t tid[num_cpu];
    int i;
    //int j;
    int error;
/*
    for(j=0; j<argc-1; j++) {
        files[j][0] = argv[2*j+1];
        files[j][1] = argv[2*j+2];
    }
    */

    for(i=0; i<num_cpu; i++) {
        error = pthread_create(&tid[i], NULL, find, (void *)files[i]);
        if(error)
            fprintf(stderr, "Couln't run thread No. %d, errno %d\n",
                    i, error);
    }

    /* now wait for all threads to terminate */
    for(i=0; i<num_cpu; i++)
        // FIXME detect error
        pthread_join(tid[i], NULL);
    //printf("%lu, %d\n", sizeof(cpu_set_t), CPU_SETSIZE);

    return 0;
}
