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
$Log: marcel_lock.h,v $
Revision 1.11  2000/05/25 00:23:49  vdanjean
marcel_poll with sisci and few bugs fixes

Revision 1.10  2000/05/09 11:18:04  vdanjean
pm2debug module : minor fixes

Revision 1.9  2000/05/09 10:52:42  vdanjean
pm2debug module

Revision 1.8  2000/04/28 18:33:35  vdanjean
debug actsmp + marcel_key

Revision 1.7  2000/04/11 09:07:11  rnamyst
Merged the "reorganisation" development branch.

Revision 1.6.2.3  2000/03/22 16:28:31  vdanjean
*** empty log message ***

Revision 1.6.2.2  2000/03/22 02:10:55  vdanjean
*** empty log message ***

Revision 1.6.2.1  2000/03/15 15:54:45  vdanjean
réorganisation de marcel : commit pour CVS

Revision 1.6  2000/03/08 17:37:23  vdanjean
*** empty log message ***

Revision 1.5  2000/03/06 14:55:42  rnamyst
Modified to include "marcel_flags.h".

Revision 1.4  2000/03/01 16:45:22  oaumage
- suppression des warnings en compilation  -g

Revision 1.3  2000/01/31 15:56:23  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

#ifndef MARCEL_LOCK_EST_DEF
#define MARCEL_LOCK_EST_DEF

#include "sys/marcel_flags.h"
#include "testandset.h"

#define MARCEL_SPIN_ITERATIONS 100

#ifdef X86_ARCH

#ifdef MA__LWPS
#define LOCK_PREFIX "lock\n\t"
#else
#define LOCK_PREFIX ""
#endif

#define __atomic_fool_gcc(x) (*(volatile struct { int a[100]; } *)x)

#if defined(MA__LWPS) || defined(MA__ACT)
typedef struct { volatile int counter; } atomic_t;
#else
typedef struct { int counter; } atomic_t;
#endif

#define ATOMIC_INIT(i)	{ (i) }

#define atomic_read(v)		((v)->counter)
#define atomic_set(v,i)		(((v)->counter) = (i))

static __inline__ 
void atomic_inc(volatile atomic_t *v) __attribute__ ((unused));
static __inline__ void atomic_inc(volatile atomic_t *v)
{
	__asm__ __volatile__(
		"incl %0"
		:"=m" (__atomic_fool_gcc(v))
		:"m" (0));
}

static __inline__ 
void atomic_dec(volatile atomic_t *v) __attribute__ ((unused));
static __inline__ void atomic_dec(volatile atomic_t *v)
{
	__asm__ __volatile__(
		"decl %0"
		:"=m" (__atomic_fool_gcc(v))
		:"m" (0));
}


#endif /* X86_ARCH */

/* 
 * Generic Part.
 */


typedef unsigned marcel_lock_t;

#define MARCEL_LOCK_INIT_UNLOCKED   0
#define MARCEL_LOCK_INIT_LOCKED   1
#define MARCEL_LOCK_INIT   MARCEL_LOCK_INIT_UNLOCKED

void marcel_lock_init(marcel_lock_t *lock);

static __inline__ void marcel_lock_acquire(marcel_lock_t *lock)
{
#ifdef MA__LWPS
  unsigned counter = 0;

  while(testandset(lock)) {
    if(++counter > MARCEL_SPIN_ITERATIONS) {
      SCHED_YIELD();
      counter=0;
    }
  }
#endif
}

static __inline__ unsigned marcel_lock_tryacquire(marcel_lock_t *lock) __attribute__ ((unused));
static __inline__ unsigned marcel_lock_tryacquire(marcel_lock_t *lock)
{
#ifdef MA__LWPS
  return !testandset(lock);
#else
  return 1;
#endif
}
static __inline__ void marcel_lock_release(marcel_lock_t *lock)
{
#ifdef MA__LWPS
  release(lock);
#endif
}

#endif
