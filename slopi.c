/* Find self-locating numbers in numbers, esp. in Pi.
 * modp_numtoa is from stringencoders at 
 *     https://code.google.com/p/stringencoders/wiki/NumToA
 *
 *     Last Modified: Wed 04 Jan 2012 10:17:44 PM
 *
 * Copyright (c) Yongzhi Pan, Since Dec 7 2011
 * License is GPL v2
 */

#include "modp_numtoa.h"
#include <errno.h>
#include <fcntl.h>
//#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
//#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

// COMMENTS
/*
#if LONG_MAX <= 10000000
#error Your compiler`s long int type is too small!
#endif
*/
// redundant, but what the heck...
#define SEEK_SET 0
// Number of bytes to bypass. 
// in order to make 1 the 1st digit of pi (3.1415...), we set OFFSET to 1.
// This is DIFFERENT from Peter's definition!
#define OFFSET 1 
#define err_exit_en(en, msg) \
  do { errno=en; perror(msg); exit(EXIT_FAILURE); } while (0)
#define err_exit(msg) \
  do { perror(msg); exit(EXIT_FAILURE); } while(0)

int fast_log10_ceil (int v) {
  // needs to expand for larger cases
  return (v >= 1000000000) ? 10 : (v >= 100000000) ? 9 : (v >= 10000000) ? 8 : 
    (v >= 1000000) ? 7 : (v >= 100000) ? 6 : (v >= 10000) ? 5 : 
    (v >= 1000) ? 4 : (v >= 100) ? 3 : (v >= 10) ? 2 : 1;
}

void prefixKMP (char *pattern, int patternLength, int kmpPi[]) {
  int i, j;
  
  //i = 0;
  //j = kmpPi[0] = -1;
  j = 0;
  kmpPi[1] = 0;
  
  // in the first run, when the second while is finished
  // the j = 0, which agrees with KMP algorithm
  
  for(i = 1; i < patternLength; i++ ) {
    if( pattern[i] == pattern[j] ) {
      kmpPi[i] = ++j;
    } 
    else {
      j = 0;
      kmpPi[i] = j;
    }
  }   
  /*
  while (i < patternLength) {
    while (j > -1 && pattern[i] != pattern[j])
      j = kmpPi[j];
	  
    i++;
    j++;
	
    if(pattern[i] == pattern[j])
      kmpPi[i] = kmpPi[j];
    else
      kmpPi[i] = j;
  }*/
}

void searchInKMP(char * pattern, int patternLength, 
		 char *text, int textLength,  int * addr) {
        int j = 0; /* number of charactes matched */
        int i; /* start pos of scan */
        
        
        for( i = 0 ; i < textLength - 1; i++ ) {
                while ( j > 0 && pattern[j] != textLength[i] )   
                        j = kmpPi[j-1];

                if( pattern[j] == text[i] ) 
                        ++j;   
                /* 
                   number of charactes matched equals to the length of 
                   the pattern so we hava a winner.
                */
                if( j == patternLength ) 
                        *addr += (1  << i);//return i;
        }

  /*
  int i, j;
  int kmpPi[patternLength];
  // this is somewhat unreliable because we assumed sizeof(int) = 4
  //int * kmpPi = (int *)malloc((patternLength << 2));

  // preprocessing
  prefixKMP(pattern, patternLength, kmpPi);

  // searching
  i = j = 0;
  while (j < textLength) {
    while (i > -1 && pattern[i] != text[j])
      i = kmpPi[i];
    i++;
    j++;
    if (j >= patternLength) {
      // binary representation of matches
      *addr += (1 << (j-i));
      i = kmpPi[i];
    }
  }
  //free(kmpPi);*/
}

void searchInBrutalForce(char *x, int m, char *y, int n, int *addr) {
   int i, j;

   /* Searching */
   for (j = 0; j <= n - m; ++j) {
      for (i = 0; i < m && x[i] == y[i + j]; ++i);
      if (i >= m)
         *addr += (1 << j);
   }
}

static void * find(void * file) {
  int s;

  // COMMENTS
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

  char * infile = (char *)file;
  struct stat stat_buf;
  int fd;
  off_t N;
  off_t n = 1;
  int maxlen;
  int num_read;
  int len = 1;

  // get file size
  stat(infile, &stat_buf);
  N = stat_buf.st_size;

  // testing: TODO / why float?
  maxlen = fast_log10_ceil(N);


  // open pi data file
  fd = open(infile, O_RDONLY);
  // file won't open
  if (fd == -1) err_exit("open");

  // COMMENTS
  // Linux stuff; not needed at the moment
  /*    s = posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);
        if(s != 0)
        err_exit_en(s, "posix_fadvise");
        */

  // failed to perform 'lseek'
  //if (lseek(fd, OFFSET, SEEK_SET) == (off_t)-1)
  //  err_exit("lseek");


  // pending deletion
  int i;

  // don't know what's off_t
  long j = 0;
  // I don't even know what this is!
  long k;

  // create a buffer from string comparison
  // this is somehow wrong! 
  // old method results in error like 'num_read is 1415926535?'
  //char buf[10+maxlen];
  char * buf = (char *)malloc(10 + maxlen);

  int matches = 0;
  int * addressOfMatches = &matches;
  
  char * numberString = (char *)malloc(maxlen);


  // end of testing 
  * addressOfMatches = 0
  
  while (j < N) {
    k = lseek(fd, (j+OFFSET), SEEK_SET);

    len = fast_log10_ceil(j) - 1;
    num_read = read(fd, buf, len+10);

    // search for the number except the last digit
    modp_litoa10((j/10), numberString);

      // to be safe
    * addressOfMatches = 0;

    searchInBrutalForce(numberString, len, buf, len+10, addressOfMatches);
    
    i = 9;
	
    while (i >= 0) {
      if (((matches >> i) & 1) && (buf[len+i]-'0' == i)) {
        printf("%ld\n", j+i);
      }
      i--;
    }

    j+=10;
  }

  free(numberString);
  free(buf);
}

int main(int argc, char *argv[]) {
  int num_cpu = sysconf(_SC_NPROCESSORS_ONLN);
  if (num_cpu == -1) err_exit("sysconf"); 
  printf("CPU number: %d\n", num_cpu);
  //char *files[num_cpu];
  const char * const files[2] = {"xac", "xad"};
  pthread_t tid[num_cpu];
  int i;
  //int j;
  int s;


  // COMMENTS
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
}
