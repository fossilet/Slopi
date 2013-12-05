/* Find self-locating numbers in numbers, esp. in Pi.
 * modp_numtoa is from stringencoders at 
 *     https://code.google.com/p/stringencoders/wiki/NumToA
 *
 *
 * Copyright (c) Yongzhi Pan, Since Dec 7 2011
 *
 * License is GPL v3
 */

#define _GNU_SOURCE
#include "modp_numtoa.h"
#include "search.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#if LONG_MAX <= 10000000
#error Your compiler`s long int type is too small!
#endif
#define CACHESIZE 10
// Number of bytes to bypass. 
// in order to make 1 the 1st digit of pi (3.1415...), we set OFFSET to 1.
#define OFFSET 1
#define err_exit_en(en, msg) \
  do { errno=en; perror(msg); exit(EXIT_FAILURE); } while (0)
#define err_exit(msg) \
  do { perror(msg); exit(EXIT_FAILURE); } while(0)

int fast_log10_ceil (long v) {
  // needs to expand for larger cases
  return (v >= 1000000000) ? 10 : (v >= 100000000) ? 9 : (v >= 10000000) ? 8 : 
    (v >= 1000000) ? 7 : (v >= 100000) ? 6 : (v >= 10000) ? 5 : 
    (v >= 1000) ? 4 : (v >= 100) ? 3 : (v >= 10) ? 2 : 1;
}


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
  //register off_t n = 1;
  int maxlen;
  
  // get file size
  stat(infile, &stat_buf);
  N = stat_buf.st_size;

  maxlen = fast_log10_ceil(N);

  // open pi data file
  fd = open(infile, O_RDONLY);
  // file won't open
  if (fd == -1)
      err_exit("open");

  // according to 
  // echo "" | gcc -E -dM -c - | grep linux
  // we should check for __linux, __linux__, __gnu_linux__, linux
  // what the heck...
  #ifdef __linux__
    s = posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);
    if(s != 0)
      err_exit_en(s, "posix_fadvise");
  #endif 

  // failed to perform 'lseek'
  if (lseek(fd, OFFSET, SEEK_SET) == (off_t)-1)
    err_exit("lseek");

  // mayber it's not needed?
  // k = lseek(...
  //long k;

  // create a buffer from string comparison
  // this method sometimes results in error 
  // like 'num_read is 1415926535?' on Mac
  //char buf[CACHESIZE + maxlen];
  //char numberString[maxlen];

  // this works fine on Mac but crash on Linux
  char * buf = (char *)malloc(CACHESIZE + maxlen);
  char * numberString = (char *)malloc(maxlen);

  int matches = 0;
  // make it a 'global variable'
  int * addressOfMatches = &matches;

  int i, len = 1;
  
  // this method cannot be used for j < CACHESIZE
  long j = CACHESIZE;
  while (j < N) { // buf never gets reset, maybe it OUGHT TO
    lseek(fd, (j+OFFSET), SEEK_SET);

    // leaving 1 bit for \0
    read(fd, buf, len+CACHESIZE);

    // reduce the number of calculation
    if (buf[0] == '1') len = fast_log10_ceil(j) - 1;

    // search for the number except the last digit
    modp_litoa10((j/CACHESIZE), numberString);

    // to be safe
    * addressOfMatches = 0;

    searchInKMP(numberString, len, buf, len+CACHESIZE, addressOfMatches);

    i = CACHESIZE;	
    while (i) {
      i--;      
      // if match found
      if (matches >> i & 1 && buf[len+i] - '0' == i)
        printf("%s%d\t\t\t%lx\n", numberString, i, (unsigned long)pthread_self());
    }
    j += CACHESIZE;
  }

  free(buf);
  free(numberString);
  
  return NULL;
}

int main(int argc, char *argv[]) {
  int num_cpu = sysconf(_SC_NPROCESSORS_ONLN);
  if (num_cpu == -1)
    err_exit("sysconf"); 
  printf("CPU number: %d\n", num_cpu);
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
