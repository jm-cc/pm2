/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.
*/

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
