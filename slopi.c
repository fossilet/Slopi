/* Find self-locating numbers in numbers, esp. in Pi.
 * modp_numtoa is from stringencoders at 
 *     https://code.google.com/p/stringencoders/wiki/NumToA
 *
 *  Last Modified: Fri 06 Jan 2012 11:35:35 PM (tweaked/forked by @th507)
 *
 * Copyright (c) Yongzhi Pan, Since Dec 7 2011
 *
 * License is GPL v3
 */

#define _GNU_SOURCE
#include "modp_numtoa.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
// seems that this file is not needed
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
#define CACHESIZE 10
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


// Knuth-Morris-Pratt
void prefixKMP (char *pattern, int patternLength, int * kmpPi) {
  int i, j;

  i = 0;
  j = kmpPi[0] = -1;

  // in the first run, when the second while is finished
  // the j = 0, which agrees with KMP algorithm

  while (i < patternLength) {
    while (j > -1 && pattern[i] != pattern[j])
      j = kmpPi[j];

    i++;
    j++;

    if(pattern[i] == pattern[j])
      kmpPi[i] = kmpPi[j];
    else
      kmpPi[i] = j;
  }
}

void searchInKMP(char * pattern, int patternLength, 
    char * text, int textLength, int * addr) {

  int i, j;
  //int kmpPi[patternLength];
  // this is somewhat unreliable because we assumed sizeof(int) = 4
  int * kmpPi = (int *)malloc((patternLength << 2));

  // preprocessing
  i = 0;
  j = kmpPi[0] = -1;

  // in the first run, when the second while is finished
  // the j = 0, which agrees with KMP algorithm
  prefixKMP(pattern, patternLength, kmpPi);

  // searching
  i = j = 0;
  while (j < textLength) {
    while (i > -1 && pattern[i] != text[j])
      i = kmpPi[i];
    i++;
    j++;
    if (i >= patternLength ) {
      // binary representation of matches
      *addr += (1 << (j - i));
      i = kmpPi[i];
    }
  }

  free(kmpPi);
}


// Boyer-Moore
#define max(a, b) ((a < b) ? b : a)
void preBmBc(char *x, int m, int bmBc[]) {
  int i;

  for (i = 0; i < CACHESIZE; ++i)
    bmBc[i] = m;
  for (i = 0; i < m - 1; ++i)
    bmBc[x[i]] = m - i - 1;
}

void suffixes(char *x, int m, int * suff) {
  int f, g, i;

  suff[m - 1] = m;
  g = m - 1;
  for (i = m - 2; i >= 0; --i) {
    if (i > g && suff[i + m - 1 - f] < i - g)
      suff[i] = suff[i + m - 1 - f];
    else {
      if (i < g)
        g = i;
      f = i;
      while (g >= 0 && x[g] == x[g + m - 1 - f])
        --g;
      suff[i] = f - g;
    }
  }
}

void preBmGs(char *x, int m, int bmGs[]) {
  int i, j, suff[m];

  suffixes(x, m, suff);

  for (i = 0; i < m; ++i)
    bmGs[i] = m;
  j = 0;
  for (i = m - 1; i >= 0; --i)
    if (suff[i] == i + 1)
      for (; j < m - 1 - i; ++j)
        if (bmGs[j] == m)
          bmGs[j] = m - 1 - i;
  for (i = 0; i <= m - 2; ++i)
    bmGs[m - 1 - suff[i]] = m - 1 - i;

  free(suff);
}

void searchInBM(char *x, int m, char *y, int n, int * addr) {
  int i, j, bmGs[m], bmBc[CACHESIZE];

  /* Preprocessing */
  preBmGs(x, m, bmGs);
  preBmBc(x, m, bmBc);

  /* Searching */
  j = 0;
  while (j <= n - m) {
    for (i = m - 1; i >= 0 && x[i] == y[i + j]; --i);
    if (i < 0) {
      // binary representation of matches      
      *addr += (1 << j);
      j += bmGs[0];
    }
    else
      j += max(bmGs[i], bmBc[y[i + j]] - m + 1 + i);
  }
}


// Brutal Force
void searchInBF(char * pattern, int patternLength, 
    char * text, int textLength, int *addr) {
  int i, j;

  /* Searching */
  for (j = 0; j <= textLength - patternLength; ++j) {
    for (i = 0; i < patternLength && pattern[i] == text[i + j]; ++i);
    if (i >= patternLength)
      // binary representation of matches
      *addr += (1 << j);
  }
}

static void * find(void * file) {
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

  char * infile = (char *)file;
  struct stat stat_buf;
  int fd;
  off_t N;
  //register off_t n = 1;
  int maxlen;
  
  // maybe it's not needed?
  // num_read = read(...
  //int num_read;

  // get file size
  stat(infile, &stat_buf);
  N = stat_buf.st_size;

  maxlen = fast_log10_ceil(N);

  // open pi data file
  fd = open(infile, O_RDONLY);
  // file won't open
  if (fd == -1) err_exit("open");

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
        printf("%s%d\n", numberString, i);
    }
    j += CACHESIZE;
  }

  free(buf);
  free(numberString);
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
}
