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

#include "marcel.h"
#include <stdio.h>
#include <sys/time.h>

static boolean finished = FALSE;

any_t looper(any_t arg)
{
  marcel_detach(marcel_self());

  fprintf(stderr, "Looper lauched on LWP %d\n",
	  marcel_current_vp());

  while(!finished)
    marcel_yield();

  return NULL;
}

any_t f(any_t arg)
{
  marcel_sem_V((marcel_sem_t *)arg);
  /* fprintf(stderr, "Hi! I'm on LWP %d\n", marcel_current_vp()); */
  return NULL;
}

extern void stop_timer(void);

tbx_tick_t t1, t2;

any_t main_thread()
{
  marcel_attr_t attr;
  marcel_sem_t sem;
  int nb;

  marcel_detach(marcel_self());

  fprintf(stderr, "Main thread launched on LWP %d\n",
	  marcel_current_vp());

  marcel_attr_init(&attr);
  marcel_attr_setdetachstate(&attr, TRUE);
#ifdef SMP
  marcel_attr_setschedpolicy(&attr, MARCEL_SCHED_FIXED(1));

  marcel_create(NULL, &attr, looper, NULL);
#endif

  marcel_sem_init(&sem, 0);

  while(1) {
    printf("How many creations ? ");
    scanf("%d", &nb);
    if(nb == 0)
      break;

    while(nb--) {
#ifndef SMP
      lock_task();
#endif

      if(nb == 0) {
	TBX_GET_TICK(t1);
#ifdef PROFILE
	profile_activate(FUT_ENABLE, MARCEL_PROF_MASK);
#endif
	marcel_create(NULL, &attr, f, (any_t)&sem);
#ifdef PROFILE
	profile_stop();
#endif
	TBX_GET_TICK(t2);
      } else
	marcel_create(NULL, &attr, f, (any_t)&sem);

#ifndef SMP
      unlock_task();
#endif
      marcel_sem_P(&sem);
    }
    printf("time =  %fus\n", TBX_TIMING_DELAY(t1, t2));
  }

  finished = TRUE;
  return NULL;
}

int marcel_main(int argc, char *argv[])
{
  marcel_attr_t attr;

  marcel_init(&argc, argv);

  marcel_attr_init(&attr);
  marcel_attr_setschedpolicy(&attr, MARCEL_SCHED_FIXED(0));
  marcel_create(NULL, &attr, main_thread, NULL);

  marcel_end();
  return 0;
}
