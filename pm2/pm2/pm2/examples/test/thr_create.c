
#include "pm2.h"

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#define ESSAIS 5

static tbx_tick_t t1, t2;

static void term_func(void *arg)
{
  slot_free(NULL, marcel_stackbase((marcel_t)arg));
}

static void *null_thread(void *arg)
{
  marcel_cleanup_push(term_func, marcel_self());
  return NULL;
}

static void *null_static_thread(void *arg)
{
  return NULL;
}

static void null_pm2_thread(void *arg)
{
}

static void eval_thread_creation(void)
{
  marcel_attr_t attr;
  int i, granted;
  unsigned long temps;
  void *stack;

  marcel_attr_init(&attr);
  marcel_attr_setdetachstate(&attr, TRUE);

  fprintf(stderr, "With isomalloc :\n");
  temps = ~0L;
  for(i=0; i<10; i++) {

    TBX_GET_TICK(t1);

    stack = slot_general_alloc(NULL, 0, &granted, NULL, NULL);
    marcel_attr_setstackaddr(&attr, stack);
    marcel_attr_setstacksize(&attr, granted);

    /* marcel_trace_on(); */
    marcel_create(NULL, &attr, (marcel_func_t)null_thread, NULL);
    /* marcel_trace_off(); */

    TBX_GET_TICK(t2);

    temps = min(temps, TBX_TIMING_DELAY(t1, t2));
  }
  fprintf(stderr, "thread create : %ld.%03ldms\n", temps/1000, temps%1000);

  fprintf(stderr, "With static stacks :\n");
  temps = ~0L;
  {
    char *stack = tmalloc(2*SLOT_SIZE);
    unsigned long stsize = (((unsigned long)(stack + 2*SLOT_SIZE) & 
			     ~(SLOT_SIZE-1)) - (unsigned long)stack);

    for(i=0; i<10; i++) {

      TBX_GET_TICK(t1);

      marcel_attr_setstackaddr(&attr, stack);
      marcel_attr_setstacksize(&attr, stsize);

      /* marcel_trace_on(); */
      marcel_create(NULL, &attr, (marcel_func_t)null_static_thread, NULL);
      /* marcel_trace_off(); */

      TBX_GET_TICK(t2);

      temps = min(temps, TBX_TIMING_DELAY(t1, t2));
    }
    fprintf(stderr, "thread create : %ld.%03ldms\n", temps/1000, temps%1000);

    tfree(stack);
  }

  fprintf(stderr, "With pm2_thread_create :\n");
  temps = ~0L;
  for(i=0; i<10; i++) {

    TBX_GET_TICK(t1);

    marcel_trace_on();
    pm2_thread_create(null_pm2_thread, NULL);
    marcel_trace_off();

    TBX_GET_TICK(t2);

    temps = min(temps, TBX_TIMING_DELAY(t1, t2));
  }
  fprintf(stderr, "thread create : %ld.%03ldms\n", temps/1000, temps%1000);
}

int pm2_main(int argc, char **argv)
{
  pm2_init(&argc, argv);

  if(pm2_self() == 0) {

    eval_thread_creation();

    pm2_halt();

  }

  pm2_exit();

  tfprintf(stderr, "Main is ending\n");
  return 0;
}
