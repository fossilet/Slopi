/* Find self-locating numbers in numbers, esp. in Pi.
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

#if LONG_MAX <= 1000000000000
#error Your compiler`s long int type is too small!
#endif

#define OFFSET 2 // Number of bytes to bypass. For 3.1415... OFFSET is 2.
#define err_exit_en(en, msg) \
    do { errno=en; perror(msg); exit(EXIT_FAILURE); } while (0)
#define err_exit(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while(0)

static void *find(void *file) {
    int s;
    // set CPU affinity
    // FIXME fixed two threads onto the same core, slows down.
    /*
       cpu_set_t set;
       CPU_ZERO(&set);
       CPU_SET(1, &set);
       s = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &set);
       if(s != 0)
       err_exit_en(s, "pthread_setaffinity_np");
       */

    char *infile = (char *)file;
    struct stat stat_buf;
    int fd;
    off_t N;
    register off_t n = 1;
    int len = 1;

    stat(infile, &stat_buf);
    N = stat_buf.st_size;
    int maxlen = (int)ceil(log10f((float)N));
    char buf[2*maxlen];
    //printf("maxlen: %d\n", maxlen);
    fd = open(infile, O_RDONLY);
    if (fd == -1) 
        err_exit("open");
    s = posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);
    if(s != 0)
        err_exit_en(s, "posix_fadvise");

    sprintf(buf, "%d", 1);
    if (lseek(fd, OFFSET, SEEK_SET) == (off_t)-1)
        err_exit("lseek");
    do {
        // For speed consideration, not all call errors are handled.
        read(fd, buf+len, len);
        //printf("buf: %.*s, len: %d, n: %d\n", 2*len, buf, len, n);
        if(!memcmp(buf, buf+len, len))
            printf("%.*s\t\t\t%lx\n", \
                    len, buf, (unsigned long)(pthread_self()));
        modp_litoa10(n+1, buf);
        // strlen works here. Since modp_litoa10 appends '\0' to buf.
        len = strlen(buf);
        n = lseek(fd, n+OFFSET, SEEK_SET) - OFFSET + 1;
    } while (n < N);
    return NULL;
}

int main(int argc, char *argv[]) {
    long num_cpu =  sysconf(_SC_NPROCESSORS_ONLN);
    if (num_cpu == -1)
       err_exit("sysconf"); 
    printf("CPU number: %ld\n", num_cpu);
    //char *files[num_cpu];
    const char * const files[2] = {"xac", "xad"};
    pthread_t tid[num_cpu];
    int i;
    //int j;
    int s;

    if(num_cpu == 2) {
        // FIXME initialize once
        // FIXME get file names from argv
        //files[0] = "xaa";
        //files[1] = "xab";
        ;
    }

    /*
    for(j=0; j<argc-1; j++) {
        files[j][0] = argv[2*j+1];
        files[j][1] = argv[2*j+2];
    }
    */
    for(i=0; i<num_cpu; i++) {
        s = pthread_create(&tid[i], NULL, find, (void *)files[i]);
        if(s != 0)
            err_exit_en(s, "pthread_create");
    }

    /* now wait for all threads to terminate */
    for(i=0; i<num_cpu; i++) {
        s = pthread_join(tid[i], NULL);
        if(s != 0)
            err_exit_en(s, "pthread_join");
    }

    return 0;
??>
