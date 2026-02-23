#ifndef UTILS_H
#define UTILS_H

#include "common.h"
#include "BigIntLib.h"

// If TESTING is undefined then debugging output is omitted, making debugging difficult
// but it avoids dependence on iostream, sstream, and iomanip.
#define TESTING
#ifdef TESTING
#include <iostream>
//#include <sstream>
#include <iomanip> // For HEX output
#endif

//#include <algorithm> // std::max, std::min
// an alternative to std::min
#define MIN(x, y) ((x) < (y) ? (x) : (y))

//#include <cstring> // memcpy, memset
//#define memcpy_generic memcpy
// alternatives to memcpy and memset
#define memcpy memcpy2_generic
//#define memcpy memcpy2
#define memset memset2
#define memcpy_generic memcpy2_generic

//#include <cstddef> // NULL
#ifndef NULL
#define NULL 0
#endif

#ifndef NO_MEMCPY
// Use static to create a separate copy for each file that includes this,
// otherwise it cannot be inlined and is much slower
static void memcpy2_generic(void* dst, void* src, uint32 n) {
  uint32 n2 = n & 7;
  word* d = (word*)dst;
  word* s = (word*)src;
  n >>= 3;
  for (uint32 i = 0; i < n; i++) {
    d[i] = s[i];
  }
  if (n2) {
    d += n;
    s += n;
    uchar* db = (uchar*) d;
    uchar* sb = (uchar*) s;
    for (uint32 i = 0; i < n2; i++) {
      db[i] = sb[i];
    }
  }
}
#endif // NO_MEMCPY

//static void memcpy2(void* dst, void* src, uint32 n) {
//  if (n & 7) {
//#ifdef TESTING
//    std::cerr << "memcpy2 failed" << std::endl;
//#endif
//    exit(1);
//  }
//  word* d = (word*)dst;
//  word* s = (word*)src;
//  n >>= 3;
//  for (uint32 i = 0; i < n; i++) {
//    d[i] = s[i];
//  }
//}

#ifndef NO_MEMSET
static void memset2(void* dst, int c, uint32 n) {
  if ((n & 7) != 0 || c != 0) {
#ifdef TESTING
    std::cerr << "memset2 failed" << std::endl;
#endif
    exit(1);
  }
  word* d = (word*)dst;
  n >>= 3;
  for (uint32 i = 0; i < n; i++) {
    d[i] = 0;
  }
}
#endif // NO_MEMSET

#endif // UTILS_H
