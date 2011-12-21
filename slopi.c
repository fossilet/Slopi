/* Find self-locating numbers in numbers, esp. in Pi.
 * For speed consideration, we do not check call failures.
 * modp_numtoa is from stringencoders at 
 *     https://code.google.com/p/stringencoders/wiki/NumToA
 * Since Dec 7 2011
*/

#include "modp_numtoa.h"
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define OFFSET 2
#define NUMT 2

const char * const files[NUMT] = {"xaa", "xab"};
// ^ split files to int32 maxsize segments

static void *find(void *file) {
    char *infile = (char *)file;
    struct stat stat_buf;
    int fd;
    int N;
    int num_read;

    stat(infile, &stat_buf);
    N = stat_buf.st_size;
    fd = open(infile, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "File open error!\n");
        exit(1);
    }

    int maxlen = (int)ceil(log10f((float)N));
    char buf[2*maxlen];
    //printf("maxlen: %d\n", maxlen);
    int n = OFFSET;
    int len = 1;
    sprintf(buf, "%d", 1);
    lseek(fd, OFFSET, SEEK_SET);
    do {
        num_read = read(fd, buf+len, len);
        //printf("buf: %.*s, len: %d, n: %d\n", 2*len, buf, len, n);
        if(!memcmp(buf, buf+len, len)) // XXX slow
            printf("%.*s\n", len, buf);
        modp_itoa10(n, buf); // XXX Only support up to uint32; still slow
        len = strlen(buf);
    } while((n=(int)lseek(fd, n+1, SEEK_SET)) < N);
        //   ^ off_t is signed int by SUSv3 type requirement
}

int main(int argc, char *argv[]) {
    // XXX
    pthread_t tid[NUMT];
    int i;
    int error;

    for(i=0; i<NUMT; i++) {
        error = pthread_create(&tid[i], NULL, find, (void *)files[i]);
        if(error)
            fprintf(stderr, "Couln't run thread No. %d, errno %d\n", i, error);
    }

    /* now wait for all threads to terminate */
    for(i=0; i<NUMT; i++) {
        pthread_join(tid[i], NULL);
    }
    return 0;
}
