#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#ifdef __BBO

#define BIT_1_POS(__value, __ret) { \
  unsigned long __n = __value; \
  unsigned __lg = 0; \
  if (__n > 0x80000000) { __lg += 32; __n >>= 32; } \
  if (__n > 0x8000) { __lg += 16; __n >>= 16; } \
  if (__n > 0x80) { __lg += 8; __n >>= 8; } \
  if (__n > 8) { __lg += 4; __n >>= 4; } \
  if (__n == 8) __lg += 4; \
  else if (__n == 4) __lg += 3; \
  else if (__n == 2) __lg += 2; \
  else if (__n == 1) __lg += 1; \
  __ret = __lg; \
}

#elif defined(__SBO)

#define BIT_1_POS(__value, __ret) { \
  int __i = 0; \
  for (unsigned long __n = __value; __n != 0; ++__i) __n >>= 1; \
	__ret = __i; \
} 

#elif defined(__BSF)

// bit scan forward for 64 bit integral number
/* ============================================ */
static inline int __bsf_folded (unsigned long bb) {
   static const int lsb_64_table[] = {
      63, 30,  3, 32, 59, 14, 11, 33,
      60, 24, 50,  9, 55, 19, 21, 34,
      61, 29,  2, 53, 51, 23, 41, 18,
      56, 28,  1, 43, 46, 27,  0, 35,
      62, 31, 58,  4,  5, 49, 54,  6,
      15, 52, 12, 40,  7, 42, 45, 16,
      25, 57, 48, 13, 10, 39,  8, 44,
      20, 47, 38, 22, 17, 37, 36, 26
   };
   unsigned int folded;
   bb ^= bb - 1;
   folded = (int) bb ^ (bb >> 32);
   return lsb_64_table[folded * 0x78291ACF >> 26];
}

#define BIT_1_POS(__value, __ret) __ret = __bsf_folded(__value)

#elif defined(__ASM)

#define BIT_1_POS(__value, __ret) { \
  int x; \
  asm ("bsr %1,%0\n" "jnz 1f\n" "bsr %0,%0\n" "subl $32,%0\n" \
     "1: addl $32,%0\n": "=&q" (__ret), "=&q" (x):"1" ((int) \
     ((__value) >> 32)), "0" ((int) (__value))); \
}

#elif defined(__LG2)
#define BIT_1_POS(__value, __ret) __ret = log2(__value)

#endif 

unsigned int _bit_scan_reverse(unsigned long n) {
	unsigned int pos=0;
	BIT_1_POS(n,pos);
	return pos;
}

unsigned int __div(unsigned int number, unsigned int denom) {
    if(1 == denom) {
        return number;
    }

    if(0 == denom) {
	    abort();
	    return 0;
    }

    const int shift_count = _bit_scan_reverse(number) - _bit_scan_reverse(denom);
    if(shift_count <= 0) {
    	return number >= denom?1:0;
    }

    unsigned int bits = 1 << shift_count;
    unsigned int result = 0;
    denom <<= shift_count;
    while(bits > 0) {
	    if(number >= denom) {
		    result += bits;
		    number -= denom;
	    }
	    bits >>= 1;
	    denom >>= 1;
    }

    return result;
}

int bench_mark() {
  int pos;
  struct timespec tp0, tp1;
  clock_gettime(CLOCK_REALTIME, &tp0);
  for (int i = 0; i < 1; ++i) {
  //for (int i = 0; i < 1000000; ++i) {
    unsigned long n = (1L<<63);
    while (n) {
      BIT_1_POS(n, pos);
      printf("%i\n", pos);
      n >>= 1;
    }
    pos = 0;
    n = 0;
    BIT_1_POS(n, pos);
    printf("%i\n", pos);
  }
  clock_gettime(CLOCK_REALTIME, &tp1);

  tp1.tv_sec -= tp0.tv_sec;
  if (tp1.tv_nsec >= tp0.tv_nsec) {
    tp1.tv_nsec -= tp0.tv_nsec;
  } else {
    tp1.tv_sec--;
    tp1.tv_nsec += 1000000000 - tp0.tv_nsec;
  }
  printf("elsp. time: %li.%09li\n", (long)tp1.tv_sec, (long)tp1.tv_nsec);
  return 0;
}

int main(int argc, char *argv[]) {
	unsigned u1=26;
	unsigned u2=5;
	printf("\t\t%u/%u=%u [%u]\n",u1,u2,__div(u1,u2),u1/u2);
	bench_mark();
}
