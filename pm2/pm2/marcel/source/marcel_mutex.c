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
$Log: marcel_mutex.c,v $
Revision 1.6  2000/04/28 10:41:40  rnamyst
Added the marcel_cond_timedwait primitive

Revision 1.5  2000/04/11 09:07:33  rnamyst
Merged the "reorganisation" development branch.

Revision 1.4.2.1  2000/03/15 15:55:11  vdanjean
réorganisation de marcel : commit pour CVS

Revision 1.4  2000/03/06 15:30:31  rnamyst
Modified to use the MARCEL_MUTEX_INITIALIZER macro.

Revision 1.3  2000/02/28 10:25:06  rnamyst
Changed #include <> into #include "".

Revision 1.2  2000/01/31 15:57:18  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

#include "marcel.h"

#include <errno.h>

int marcel_mutexattr_init(marcel_mutexattr_t *attr)
{
   return 0;
}

int marcel_mutexattr_destroy(marcel_mutexattr_t *attr)
{
   return 0;
}

static marcel_mutexattr_t marcel_mutexattr_default = MARCEL_MUTEX_INITIALIZER;

int marcel_mutex_init(marcel_mutex_t *mutex, marcel_mutexattr_t *attr)
{
   if(!attr)
      attr = &marcel_mutexattr_default;
   mutex->value = 1; /* free */
   mutex->first = NULL;
   mutex->lock = MARCEL_LOCK_INIT;
   return 0;
}

int marcel_mutex_destroy(marcel_mutex_t *mutex)
{
   return 0;
}

int marcel_mutex_lock(marcel_mutex_t *mutex)
{
  cell c;

  lock_task();

  marcel_lock_acquire(&mutex->lock);

  if(mutex->value == 0) { /* busy */
    c.next = NULL;
    c.blocked = TRUE;
    c.task = marcel_self();
    if(mutex->first == NULL)
      mutex->first = mutex->last = &c;
    else {
      mutex->last->next = &c;
      mutex->last = &c;
    }
    marcel_give_hand(&c.blocked, &mutex->lock);
  } else {
    mutex->value = 0;

    marcel_lock_release(&mutex->lock);
    unlock_task();
  }

   return 0;
}

int marcel_mutex_trylock(marcel_mutex_t *mutex)
{
  lock_task();

  marcel_lock_acquire(&mutex->lock);

  if(mutex->value == 1) { /* free */
    mutex->value = 0;
    marcel_lock_release(&mutex->lock);
    unlock_task();
    return 1;
  } else {
    marcel_lock_release(&mutex->lock);
    unlock_task();
    return 0;
  }
}

int marcel_mutex_unlock(marcel_mutex_t *mutex)
{
  cell *c;

  lock_task();

  marcel_lock_acquire(&mutex->lock);

  if(mutex->first != NULL) {
    c = mutex->first;
    mutex->first = c->next;
    marcel_wake_task(c->task, &c->blocked);
  } else
    mutex->value = 1; /* free */

  marcel_lock_release(&mutex->lock);
  unlock_task();

  return 0;
}

int marcel_condattr_init(marcel_condattr_t *attr)
{
  return 0;
}

int marcel_condattr_destroy(marcel_condattr_t *attr)
{
   return 0;
}

static marcel_condattr_t marcel_condattr_default = { 0 };

int marcel_cond_init(marcel_cond_t *cond, marcel_condattr_t *attr)
{
   if(!attr)
      attr = &marcel_condattr_default;
   marcel_sem_init(cond, 0);
   return 0;
}

int marcel_cond_destroy(marcel_cond_t *cond)
{
   return 0;
}

int marcel_cond_signal(marcel_cond_t *cond)
{
  cell *ce;

   lock_task();

   marcel_lock_acquire(&cond->lock);

   if(cond->value < 0) {
     cond->value++;
     ce = cond->first;
     cond->first = ce->next;
     marcel_wake_task(ce->task, &ce->blocked);
   }

   marcel_lock_release(&cond->lock);
   unlock_task();

   return 0;
}

int marcel_cond_broadcast(marcel_cond_t *cond)
{
  marcel_sem_unlock_all(cond);
  return 0;
}

int marcel_cond_wait(marcel_cond_t *cond, marcel_mutex_t *mutex)
{
  cell c, *cp;

  lock_task();

  marcel_lock_acquire(&mutex->lock);

  if(mutex->first != NULL) {
    cp = mutex->first;
    mutex->first = cp->next;
    marcel_wake_task(cp->task, &cp->blocked);
  } else
    mutex->value = 1; /* free */

  marcel_lock_release(&mutex->lock);

  marcel_lock_acquire(&cond->lock);

  if(--(cond->value) < 0) {
    c.next = NULL;
    c.blocked = TRUE;
    c.task = marcel_self();
    if(cond->first == NULL)
      cond->first = cond->last = &c;
    else {
      cond->last->next = &c;
      cond->last = &c;
    }
    marcel_give_hand(&c.blocked, &cond->lock);
  } else {
   marcel_lock_release(&cond->lock);
   unlock_task();
  }

  lock_task();

  marcel_lock_acquire(&mutex->lock);

  if(mutex->value == 0) { /* busy */
    c.next = NULL;
    c.blocked = TRUE;
    c.task = marcel_self();
    if(mutex->first == NULL)
      mutex->first = mutex->last = &c;
    else {
      mutex->last->next = &c;
      mutex->last = &c;
    }
    marcel_give_hand(&c.blocked, &mutex->lock);
  } else {
    mutex->value = 0;

    marcel_lock_release(&mutex->lock);
    unlock_task();
  }

  return 0;
}

int marcel_cond_timedwait(marcel_cond_t *cond, marcel_mutex_t *mutex,
			  const struct timespec *abstime)
{
  cell c, *cp;
  struct timeval now, tv;
  unsigned long timeout;

  tv.tv_sec = abstime->tv_sec;
  tv.tv_usec = abstime->tv_nsec / 1000;

  gettimeofday(&now, NULL);

  if(timercmp(&tv, &now, <=))
    return ETIMEDOUT;

  timeout = ((tv.tv_sec*1e6 + tv.tv_usec) -
	     (now.tv_sec*1e6 + now.tv_usec)) / 1000;

  printf("timeout = %ld\n", timeout);

  lock_task();

  marcel_lock_acquire(&mutex->lock);

  if(mutex->first != NULL) {
    cp = mutex->first;
    mutex->first = cp->next;
    marcel_wake_task(cp->task, &cp->blocked);
  } else
    mutex->value = 1; /* free */

  marcel_lock_release(&mutex->lock);

  marcel_lock_acquire(&cond->lock);

  if(--(cond->value) < 0) {
    c.next = NULL;
    c.blocked = TRUE;
    c.task = marcel_self();
    if(cond->first == NULL)
      cond->first = cond->last = &c;
    else {
      cond->last->next = &c;
      cond->last = &c;
    }
    BEGIN
      marcel_tempo_give_hand(timeout, &c.blocked, cond);
    EXCEPTION
      WHEN(TIME_OUT)
        return ETIMEDOUT;
    END
  } else {
   marcel_lock_release(&cond->lock);
   unlock_task();
  }

  lock_task();

  marcel_lock_acquire(&mutex->lock);

  if(mutex->value == 0) { /* busy */
    c.next = NULL;
    c.blocked = TRUE;
    c.task = marcel_self();
    if(mutex->first == NULL)
      mutex->first = mutex->last = &c;
    else {
      mutex->last->next = &c;
      mutex->last = &c;
    }
    marcel_give_hand(&c.blocked, &mutex->lock);
  } else {
    mutex->value = 0;

    marcel_lock_release(&mutex->lock);
    unlock_task();
  }

  return 0;
}
