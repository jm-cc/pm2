
/*
 * Tbx_timing.h
 * ============
 */

#ifndef TBX_TIMING_H
#define TBX_TIMING_H

#include <sys/types.h>
#include <sys/time.h>

typedef union
{
  unsigned long long tick;
  struct
  {
    unsigned long low, high;
  } sub;
  struct timeval timev;
} tbx_tick_t, *p_tbx_tick_t;

/* Hum hum... Here we suppose that X86ARCH => Pentium! */
#ifdef X86_ARCH
#define TBX_GET_TICK(t) __asm__ volatile("rdtsc" : "=a" ((t).sub.low), "=d" ((t).sub.high))
#define TBX_TICK_RAW_DIFF(t1, t2) ((t2).tick - (t1).tick)
#else /* X86_ARCH */
#define TBX_GET_TICK(t) gettimeofday(&(t).timev, NULL)
#define TBX_TICK_RAW_DIFF(t1, t2) \
    ((t2.timev.tv_sec * 1000000L + t2.timev.tv_usec) - \
     (t1.timev.tv_sec * 1000000L + t1.timev.tv_usec))
#endif /* X86_ARCH */

#define TBX_TICK_DIFF(t1, t2) (TBX_TICK_RAW_DIFF(t1, t2) - tbx_residual)
#define TBX_TIMING_DELAY(t1, t2) tbx_tick2usec(TBX_TICK_DIFF(t1, t2))

extern long long     tbx_residual;
extern tbx_tick_t    tbx_new_event, tbx_last_event;

#endif /* TBX_TIMING_H */
