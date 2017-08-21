#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sched.h>

#include <tbx.h>

#define DEFAULT_LOOPS 10000

#define MAX_SLEEP 500

static unsigned loops = DEFAULT_LOOPS;

int main(int argc, char**argv)
{
  tbx_init(&argc, &argv);

  tbx_tick_t t1, t2;
  TBX_GET_TICK(t1);
  int i;
  for(i = 0 ; i < loops ; i++)
    {
      sched_yield();
    }
  TBX_GET_TICK(t2);
  double t = TBX_TIMING_DELAY(t1, t2);
  printf("# sched_yield = %f usec.\n", t/(double)loops);

  int s = 1;
  for(s = 1; s < MAX_SLEEP ; s *= 2)
    {
      TBX_GET_TICK(t1);
      for(i = 0 ; i < loops ; i++)
	{
	  usleep(s);
	}
      TBX_GET_TICK(t2);
      t = TBX_TIMING_DELAY(t1, t2);
      printf("# usleep(%d) = %f usec.\n", s, t/(double)loops);
    }

  for(s = 1; s < MAX_SLEEP ; s *= 2)
    {
      TBX_GET_TICK(t1);
      for(i = 0 ; i < loops ; i++)
	{
	  struct timespec ts = { .tv_sec = 0, .tv_nsec = 1000 * s };
	  nanosleep(&ts, NULL);
	}
      TBX_GET_TICK(t2);
      t = TBX_TIMING_DELAY(t1, t2);
      printf("# nanosleep(%d) = %f usec.\n", s, t/(double)loops);
    }

  for(s = 1; s < MAX_SLEEP ; s *= 2)
    {
      TBX_GET_TICK(t1);
      for(i = 0 ; i < loops ; i++)
	{
	  struct timespec ts = { .tv_sec = 0, .tv_nsec = 1000 * s };
	  clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
	}
      TBX_GET_TICK(t2);
      t = TBX_TIMING_DELAY(t1, t2);
      printf("# clock_nanosleep(%d) = %f usec.\n", s, t/(double)loops);
    }

  for(s = 1; s < MAX_SLEEP ; s *= 2)
    {
      TBX_GET_TICK(t1);
      for(i = 0 ; i < loops ; i++)
	{
	  struct timeval tv = { .tv_sec = 0, .tv_usec = s };
	  select(0, NULL, NULL, NULL, &tv);
	}
      TBX_GET_TICK(t2);
      t = TBX_TIMING_DELAY(t1, t2);
      printf("# select(%d) = %f usec.\n", s, t/(double)loops);
    }
  
  return 0;
}

