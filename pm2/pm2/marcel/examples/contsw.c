
#include "marcel.h"
#include <stdio.h>

static unsigned long temps;

any_t f(any_t arg)
{
  register int n = (int)arg;
  tbx_tick_t t1, t2;

  TBX_GET_TICK(t1);
  while(--n)
    marcel_yield();
  TBX_GET_TICK(t2);

  temps = TBX_TIMING_DELAY(t1, t2);
  return NULL;
}

extern void stop_timer(void);

int marcel_main(int argc, char *argv[])
{ 
  marcel_t pid;
  any_t status;
  int nb;
  register int n;

  marcel_init(&argc, argv);

  stop_timer();

  while(1) {
    printf("How many context switches ? ");
    scanf("%d", &nb);
    n = nb;
    if(n==0)
      break;

    n >>= 1;
    n++;

    marcel_create(&pid, NULL, f, (any_t)n);
    marcel_yield();

    while(--n)
      marcel_yield();

    marcel_join(pid, &status);

    printf("time =  %ld.%03ldms\n", temps/1000, temps%1000);
  }
  return 0;
}
