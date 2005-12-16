#define __ALIGNED__ __attribute__ ((aligned (sizeof(int))))

char s[16] __ALIGNED__;
