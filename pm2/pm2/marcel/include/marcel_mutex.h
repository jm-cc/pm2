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
$Log: marcel_mutex.h,v $
Revision 1.6  2000/07/03 15:51:48  rnamyst
Bug fixed in macro MARCEL_MUTEX_INITIALIZER

Revision 1.5  2000/04/28 10:40:49  rnamyst
Added the marcel_cond_timedwait primitive

Revision 1.4  2000/04/11 09:07:11  rnamyst
Merged the "reorganisation" development branch.

Revision 1.3.2.1  2000/03/15 15:54:46  vdanjean
réorganisation de marcel : commit pour CVS

Revision 1.3  2000/03/06 15:30:11  rnamyst
Added the macro MARCEL_MUTEX_INITIALIZER.

Revision 1.2  2000/01/31 15:56:24  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

#ifndef MARCEL_MUTEX_EST_DEF
#define MARCEL_MUTEX_EST_DEF

#include <sys/time.h>

#define MARCEL_MUTEX_INITIALIZER { 1 }

_PRIVATE_ typedef struct {
	int dummy;
} marcel_mutexattr_struct;

_PRIVATE_ typedef marcel_sem_t marcel_mutex_t;

_PRIVATE_ typedef struct {
	int dummy;
} marcel_condattr_struct;

_PRIVATE_ typedef marcel_sem_t marcel_cond_t;

typedef marcel_mutexattr_struct marcel_mutexattr_t;

typedef marcel_condattr_struct marcel_condattr_t;

int marcel_mutexattr_init(marcel_mutexattr_t *attr);
int marcel_mutexattr_destroy(marcel_mutexattr_t *attr);

int marcel_mutex_init(marcel_mutex_t *mutex, marcel_mutexattr_t *attr);
int marcel_mutex_destroy(marcel_mutex_t *mutex);

int marcel_mutex_lock(marcel_mutex_t *mutex);
int marcel_mutex_trylock(marcel_mutex_t *mutex);
int marcel_mutex_unlock(marcel_mutex_t *mutex);

int marcel_condattr_init(marcel_condattr_t *attr);
int marcel_condattr_destroy(marcel_condattr_t *attr);

int marcel_cond_init(marcel_cond_t *cond, marcel_condattr_t *attr);
int marcel_cond_destroy(marcel_cond_t *cond);

int marcel_cond_signal(marcel_cond_t *cond);
int marcel_cond_broadcast(marcel_cond_t *cond);

int marcel_cond_wait(marcel_cond_t *cond, marcel_mutex_t *mutex);
int marcel_cond_timedwait(marcel_cond_t *cond, marcel_mutex_t *mutex,
			  const struct timespec *abstime);
#endif
