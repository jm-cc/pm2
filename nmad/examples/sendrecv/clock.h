#include <time.h>

/* TODO- integrate into tbx_timing.h */

static double clock_offset = 0.0;


static inline void clock_init(void)
{
  /* clock calibration */
  double offset = 100.0;
  int i;
  for(i = 0; i < 1000; i++)
    {
      struct timespec t1, t2;
      clock_gettime(CLOCK_MONOTONIC, &t1);
      clock_gettime(CLOCK_MONOTONIC, &t2);
      const double delay = 1000000.0*(t2.tv_sec - t1.tv_sec) + (t2.tv_nsec - t1.tv_nsec) / 1000.0;
      if(delay < offset)
        offset = delay;
    }
  clock_offset = offset;
}

static inline double clock_diff(struct timespec t1, struct timespec t2)
{
  return 1000000.0*(t2.tv_sec - t1.tv_sec) + (t2.tv_nsec - t1.tv_nsec) / 1000.0 - clock_offset;
}
