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

/* ********************************************************* */

#ifdef PTHREAD

#include <pthread.h>
#include <timing.h>

#define marcel_mutex_t             pthread_mutex_t
#define marcel_mutex_init          pthread_mutex_init
#define marcel_mutex_lock          pthread_mutex_lock
#define marcel_mutex_unlock        pthread_mutex_unlock

#define marcel_cond_t              pthread_cond_t
#define marcel_cond_init           pthread_cond_init
#define marcel_cond_wait           pthread_cond_wait
#define marcel_cond_signal         pthread_cond_signal

#define marcel_attr_t              pthread_attr_t
#define marcel_attr_init           pthread_attr_init
#define marcel_attr_setdetachstate pthread_attr_setdetachstate
#define MARCEL_CREATE_DETACHED     PTHREAD_CREATE_DETACHED

#define marcel_t                   pthread_t
#define marcel_create              pthread_create
#define marcel_join                pthread_join

#define marcel_main                main

#else

#define NON_BLOCKING_IO

#include <marcel.h>

#endif

/* ********************************************************* */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

typedef struct {
  int tub [2] ;
  marcel_mutex_t mutex ;
} tube ;

typedef struct {
  int inf, sup, res;
  marcel_t pid;
  marcel_mutex_t mutex;
  marcel_cond_t cond;
  int completed;
} job_t;

static tube tube_1;

static marcel_attr_t glob_attr;

void job_init(job_t *j, int inf, int sup)
{
  j->inf = inf; j->sup = sup;
  marcel_mutex_init(&j->mutex, NULL);
  marcel_cond_init(&j->cond, NULL);
  j->completed = 0;
}

void create_tube (tube *t)
{
  marcel_mutex_init (&t->mutex, NULL) ;
  pipe (t->tub) ;
#ifdef NON_BLOCKING_IO
  fcntl(t->tub[0], F_SETFL, O_NONBLOCK);
  fcntl(t->tub[1], F_SETFL, O_NONBLOCK);
#endif
}

int send_message (tube *t, char *message, int len)
{
  int n;

  marcel_mutex_lock (&t->mutex) ;

#ifdef NON_BLOCKING_IO
  while(1) {
    n = write (t->tub[1], message, len) ;
    if(n == -1 && errno == EAGAIN)
      marcel_yield();
    else
      break;
  }
#else
  n = write (t->tub[1], message, len) ;
#endif

  marcel_mutex_unlock(&t->mutex) ;

  return n ;
}

int receive_message (tube *t, char *message, int len)
{
  int n;

#ifdef NON_BLOCKING_IO
  while(1) {
    n = read (t->tub[0], message, len) ;
    if(n == -1 && errno == EAGAIN)
      marcel_yield();
    else
      break;
  }
#else
  n = read (t->tub[0], message, len) ;
#endif
    
  return n ;
}

void send_to_server(job_t *j)
{
  send_message(&tube_1, (char *)&j, sizeof(job_t *)) ;
}

/* ********************************************************* */

void *compute(void *arg);

void *server(void * arg)
{
  job_t *j;
  marcel_t pid;

  while(1) {
    receive_message (&tube_1, (char *)&j, sizeof(job_t *));
    if(j->inf == 0)
      break;
    marcel_create(&pid, &glob_attr, compute, (void *)j);
  }

  return NULL;
}

void create_new_thread(job_t *j)
{
#if 0
  marcel_t pid;
  marcel_create(&pid, &glob_attr, compute, (void *)j);
#else
  send_to_server(j);
#endif
}

void wait_thread(job_t *j)
{
  marcel_mutex_lock(&j->mutex);
  while(!j->completed) {
    marcel_cond_wait(&j->cond, &j->mutex);
  }
  marcel_mutex_unlock(&j->mutex);
}

void wake_thread(job_t *j, int res)
{
  j->res = res;
  marcel_mutex_lock(&j->mutex);
  j->completed = 1;
  marcel_cond_signal(&j->cond);
  marcel_mutex_unlock(&j->mutex);
}

void *compute(void *arg)
{
  job_t *j = (job_t *)arg;

  if(j->inf == j->sup) {
    wake_thread(j, j->inf);
  } else {
    job_t j1, j2;

    job_init(&j1, j->inf,(j->inf+j->sup)/2);
    create_new_thread(&j1);


    job_init(&j2, (j->inf+j->sup)/2+1, j->sup);
    create_new_thread(&j2);

    wait_thread(&j1);
    wait_thread(&j2);

    wake_thread(j, j1.res + j2.res);
  }

  return NULL;
}

int marcel_main(int argc, char **argv)
{
  marcel_t serv_pid;
  marcel_attr_t attr;
  job_t j;
  Tick t1, t2;
  unsigned long temps;

  create_tube (&tube_1) ;

#ifdef PTHREAD
  timing_init();
#else
  marcel_init(&argc, argv);

  marcel_trace_on();
#endif

  marcel_attr_init(&attr);

#ifdef SMP
  marcel_attr_setschedpolicy(&attr, MARCEL_SCHED_FIXED(1));
#endif
  marcel_create(&serv_pid, &attr, server, NULL);

  marcel_attr_init(&glob_attr);
  marcel_attr_setdetachstate(&glob_attr, MARCEL_CREATE_DETACHED);
#ifdef SMP
  marcel_attr_setschedpolicy(&glob_attr, MARCEL_SCHED_FIXED(0));
#endif

  job_init(&j, 1, 50);

  GET_TICK(t1);

  create_new_thread(&j);

  wait_thread(&j);

  GET_TICK(t2);

  printf("Sum from 1 to %d = %d\n", j.sup, j.res);
  temps = timing_tick2usec(TICK_DIFF(t1, t2));
  printf("time = %ld.%03ldms\n", temps/1000, temps%1000);

  /* terminaison du serveur */
  job_init(&j, 0, 0);
  send_to_server(&j);

  marcel_join(serv_pid, NULL);

#ifndef PTHREAD
  marcel_end();
#endif

  return 0;
}
