#ifndef UTILS_H
#define UTILS_H

#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>

#define max(a, b) \
       ({__typeof__ ((a)) _a = (a); \
         __typeof__ ((b)) _b = (b); \
         _a > _b ? _a : _b; })
#define min(a, b) \
       ({__typeof__ ((a)) _a = (a); \
         __typeof__ ((b)) _b = (b); \
         _a < _b ? _a : _b; })


typedef union u_tick
{
	unsigned long long tick;

	struct
	{
		unsigned long low;
		unsigned long high;
	} sub;
	
	struct timeval timev;
} tick_t, *p_tick_t;

static double scale = 0.0;
static unsigned long long residual = 0;
static tick_t last_event;

#define GET_TICK(t) __asm__ volatile("rdtsc" : "=a" ((t).sub.low), "=d" ((t).sub.high))
#define TICK_RAW_DIFF(t1, t2) ((t2).tick - (t1).tick)
#define TICK_DIFF(t1, t2) (TICK_RAW_DIFF(t1, t2) - residual)
#define TIMING_DELAY(t1, t2) tick2usec(TICK_DIFF(t1, t2))

void timing_init(void)
{
	static tick_t t1, t2;
	int i;
  
	residual = (unsigned long long)1 << 63;

	for(i=0; i<20; i++) {
		GET_TICK(t1);
		GET_TICK(t2);
		residual = min(residual, TICK_RAW_DIFF(t1, t2));
	}
	
	{
		struct timeval tv1,tv2;
		
		GET_TICK(t1);
		gettimeofday(&tv1,0);
		usleep(50000);
		GET_TICK(t2);
		gettimeofday(&tv2,0);
		scale = ((tv2.tv_sec*1e6 + tv2.tv_usec) -
			 (tv1.tv_sec*1e6 + tv1.tv_usec)) / 
			(double)(TICK_DIFF(t1, t2));
	}
	
	GET_TICK(last_event);
}

double tick2usec(long long t)
{
	return (double)(t)*scale;
}



#endif // UTILS_H
