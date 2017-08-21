#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sched.h>

#include <tbx.h>

#define DEFAULT_LOOPS 100000

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
  printf("# sched_yield = %fsec.\n", t/(double)loops);
  return 0;
}

