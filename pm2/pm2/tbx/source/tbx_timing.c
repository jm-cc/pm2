
/*
 * Tbx_timing.c
 * ============
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include "tbx.h"

static double   scale = 0.0;
long long       tbx_residual;
tbx_tick_t      tbx_new_event ;
tbx_tick_t      tbx_last_event;

void tbx_timing_init()
{
  static tbx_tick_t t1, t2;
  int i;
  
  tbx_residual = (unsigned long long)1 << 32;
  for(i=0; i<5; i++) {
    TBX_GET_TICK(t1);
    TBX_GET_TICK(t2);
    tbx_residual = min(tbx_residual, TBX_TICK_RAW_DIFF(t1, t2));
  }

#ifdef X86_ARCH
  {
    struct timeval tv1,tv2;

    TBX_GET_TICK(t1);
    gettimeofday(&tv1,0);
    usleep(50000);
    TBX_GET_TICK(t2);
    gettimeofday(&tv2,0);
    scale = ((tv2.tv_sec*1e6 + tv2.tv_usec) -
	     (tv1.tv_sec*1e6 + tv1.tv_usec)) / 
             (double)(TBX_TICK_DIFF(t1, t2));
  }
#else
  scale = 1.0;
#endif

  TBX_GET_TICK(tbx_last_event);
}

double tbx_tick2usec(long long t)
{
  return (double)(t)*scale;
}
