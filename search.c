// Knuth-Morris-Pratt
#include "search.h"
#include <stdlib.h>
#define CACHESIZE 10

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
  int * kmpPi = (int *)malloc(patternLength << 2);

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
    bmBc[x[i]-'0'] = m - i - 1;
}

void suffixes(char *x, int m, int * suff) {
  int f=0, g, i;

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
      j += max(bmGs[i], bmBc[y[i + j]-'0'] - m + 1 + i);
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
