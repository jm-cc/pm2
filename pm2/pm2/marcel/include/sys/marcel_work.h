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

$Log: marcel_work.h,v $
Revision 1.1  2000/05/29 08:59:24  vdanjean
work added (mainly for SMP and ACT), minor modif in polling


______________________________________________________________________________
*/

#ifndef MARCEL_WORK_EST_DEF
#define MARCEL_WORK_EST_DEF

#ifdef MA__WORK

/* flags indiquant le type de travail à faire */
enum {
  MARCEL_WORK_DEVIATE=0x1,
};

extern volatile int marcel_global_work;
extern marcel_lock_t marcel_work_lock;

#define HAS_WORK(self) (self->has_work || marcel_global_work)
#define LOCK_WORK(self) (marcel_lock_acquire(&marcel_work_lock))
#define UNLOCK_WORK(self) (marcel_lock_release(&marcel_work_lock))

void do_work(marcel_t self);

#else /* MA__WORK */

#define HAS_WORK(self) 0
#define LOCK_WORK(self) ((void)0)
#define UNLOCK_WORK(self) ((void)0)
#define do_work(marcel_self) ((void)0)

#endif /* MA__WORK */

#endif /* MARCEL_WORK_EST_DEF */

