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

static int MY_SCHED_POLICY;

#define NB    3

char *mess[NB] = { "boys", "girls", "people" };

any_t writer(any_t arg)
{
  int i;

   for(i=0;i<10;i++) {
      tprintf("Hi %s! (I'm %p on vp %d)\n",
	      (char*)arg, marcel_self(), marcel_current_vp());
      marcel_delay(100);
   }

   return NULL;
}

/* Sample (reverse) cyclic distribution */
static __lwp_t *my_sched_func(marcel_t pid, __lwp_t *current_lwp)
{
  static __lwp_t *lwp = &__main_lwp;

  mdebug("User scheduler called for thread %p\n", pid);

  lwp = lwp->prev;
  return lwp;
}


int marcel_main(int argc, char *argv[])
{
  marcel_t pid[NB];
  marcel_attr_t attr;
  int i;

   marcel_init(&argc, argv);

   marcel_schedpolicy_create(&MY_SCHED_POLICY, my_sched_func);

   marcel_attr_init(&attr);
   marcel_attr_setschedpolicy(&attr, MY_SCHED_POLICY);

   for(i=0; i<NB; i++)
     marcel_create(&pid[i], &attr, writer, (any_t)mess[i]);

   for(i=0; i<NB; i++)
     marcel_join(pid[i], NULL);

   marcel_end();
   return 0;
}



