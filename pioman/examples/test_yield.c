
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sched.h>
#include <cpuid.h>

#include <x86intrin.h>

#include <tbx.h>

#define DEFAULT_LOOPS 10000

#define MAX_SLEEP 500

static unsigned loops = DEFAULT_LOOPS;

static inline int supports_invariant_tsc(void)
{
  unsigned a, b, c, d;
  if (__get_cpuid(0x80000007, &a, &b, &c, &d))
    return (d&(1<<8) ? 1 : 0);
 else
   return 0;
}

static inline int supports_getcpu(void)
{
  int rc = sched_getcpu();
  return (rc != -1);
}

static inline int supports_rdtscp(void)
{
  unsigned a, b, c, d;
  if (__get_cpuid(0x80000001, &a, &b, &c, &d))
    return (d&(1<<27) ? 1 : 0);
 else
   return 0;
}

static inline uint64_t rdtscp(uint32_t*aux)
{
  /*
    uint64_t rax,rdx;
    asm volatile ( "rdtscp\n" : "=a" (rax), "=d" (rdx), "=c" (*aux) : : );
    return (rdx << 32) + rax;
  */
  return __rdtscp(aux);
}

int main(int argc, char**argv)
{
  tbx_init(&argc, &argv);

  fprintf(stderr, "support invarient tsc: %d\n", supports_invariant_tsc());
  fprintf(stderr, "support rdtscp: %d\n", supports_rdtscp());
  if(supports_rdtscp())
    {
      uint32_t aux = -1;
      rdtscp(&aux);
      fprintf(stderr, "  rdtscp aux = %d\n", aux);
    }
  fprintf(stderr, "support getcpu: %d\n", supports_getcpu());
  if(supports_getcpu())
    {
      int rc = sched_getcpu();
      fprintf(stderr, "  sched_getcpu = %d\n", rc);
    }

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

  if(supports_getcpu())
    {
      TBX_GET_TICK(t1);
      for(i = 0 ; i < loops ; i++)
	{
	  sched_getcpu();
	}
      TBX_GET_TICK(t2);
      t = TBX_TIMING_DELAY(t1, t2);
      printf("# sched_getcpu = %f usec.\n", t/(double)loops);
    }

  if(supports_rdtscp())
    {
      TBX_GET_TICK(t1);
      for(i = 0 ; i < loops ; i++)
	{
	  uint32_t a = -1;
	  rdtscp(&a);
	}
      TBX_GET_TICK(t2);
      t = TBX_TIMING_DELAY(t1, t2);
      printf("# rdtscp = %f usec.\n", t/(double)loops);
    }

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

