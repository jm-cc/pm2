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

______________________________________________________________________________
$Log: marcel_work.c,v $
Revision 1.1  2000/05/29 08:59:26  vdanjean
work added (mainly for SMP and ACT), minor modif in polling


______________________________________________________________________________
*/

#include "marcel.h"

#ifdef MA__WORK

volatile int marcel_global_work=0;
marcel_lock_t marcel_work_lock=MARCEL_LOCK_INIT_UNLOCKED;


void do_work(marcel_t self)
{
  int has_done_work=0;
#ifdef DEBUG
  if(locked() != 1)
    RAISE(LOCK_TASK_ERROR);
#endif
  LOCK_WORK(self);
  MTRACE("DO_WORK", self);
  mdebug_work("do_work : marcel_global_work=%p, has_work=%p\n", 
	      (void*)marcel_global_work, (void*)self->has_work);
  while (marcel_global_work || self->has_work) {
    if (self->has_work & MARCEL_WORK_DEVIATE) {
      handler_func_t h=self->deviation_func;
      any_t arg=self->deviation_arg;
      self->deviation_func=NULL;
      self->has_work &= ~MARCEL_WORK_DEVIATE;
      UNLOCK_WORK(self);
      unlock_task();
      (*h)(arg);
      lock_task();
      LOCK_WORK(self);     
    }
    if ((marcel_global_work  || self->has_work) && !has_done_work) {
      RAISE(PROGRAM_ERROR);
    }
  }

  UNLOCK_WORK(self);

}

#endif






