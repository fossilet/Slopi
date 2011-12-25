/* Find self-locating numbers in numbers, esp. in Pi.
 * For speed consideration, not all call errors are handles.
 * modp_numtoa is from stringencoders at 
 *     https://code.google.com/p/stringencoders/wiki/NumToA
 *
 * Copyright (c) Yongzhi Pan, Since Dec 7 2011
 * License is GPL v3
 */

#define _GNU_SOURCE
#include "modp_numtoa.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#if LONG_MAX < 1000000000000
#error Your compiler`s long int type is too small!
#endif

#define OFFSET 2 // How many bytes to bypass. For 3.1415... OFFSET is 2.
#define err_exit(en, msg) \
    do { errno=en; perror(msg); exit(EXIT_FAILURE); } while (0)

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
    long N;
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
    long n = 1;
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
        modp_litoa10(n+1, buf);
        len = strlen(buf);
    } while((n=lseek(fd, n+OFFSET, SEEK_SET)-OFFSET+1) < N);
    //   ^ off_t is signed int by SUSv3 type requirement
}

int main(int argc, char *argv[]) {
    // FIXME detect error
    long num_cpu =  sysconf(_SC_NPROCESSORS_ONLN);
    printf("CPU number: %ld\n", num_cpu);
    //char *files[num_cpu];
    const char * const files[2] = {"xac", "xad"};

    if(num_cpu == 2) {
        // FIXME initialize once
        // FIXME get file names from argv
        //files[0] = "xaa";
        //files[1] = "xab";
        ;
    }

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

    //printf("INT_MAX: %d, LONG_MAX: %ld, LLONG_MAX, %lld\n", INT_MAX, LONG_MAX, LLONG_MAX);
    printf("int32_t: %lu, int64_t: %lu\n", sizeof(int32_t), sizeof(int64_t));
    printf("off_t: %lu, off64_t: %lu\n", sizeof(off_t), sizeof(off64_t));
    return 0;
}
