
/* ********************************************************* */

#ifdef PTHREAD

#include <pthread.h>
#include "timing.h"

#define marcel_mutex_t             pthread_mutex_t
#define marcel_mutex_init          pthread_mutex_init
#define marcel_mutex_lock          pthread_mutex_lock
#define marcel_mutex_unlock        pthread_mutex_unlock

#define marcel_attr_t              pthread_attr_t
#define marcel_attr_init           pthread_attr_init
#define marcel_t                   pthread_t
#define marcel_create              pthread_create
#define marcel_join                pthread_join

#define marcel_main                main

#define marcel_self()              pthread_self()

#else

#ifndef MARCEL_ACT
/* #define NON_BLOCKING_IO */
#endif

#include "marcel.h"

#endif

/* ********************************************************* */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#define ITERATIONS    1000

typedef struct {
  int tub [2] ;
  marcel_mutex_t mutex ;
} tube ;

tube tube_1, tube_2 ;

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
#ifdef SHOW
  printf("Before write %p\n", marcel_self());
#endif
  n = write (t->tub[1], message, len) ;
#ifdef SHOW
  printf("After write %p\n", marcel_self());
#endif
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
#ifdef SHOW
  printf("Before read %p\n", marcel_self());
#endif
  //marcel_yield();
  n = read (t->tub[0], message, len) ;
#ifdef SHOW
  printf("After read %p\n", marcel_self());
#endif
#endif
    
  return n ;
}

void * producteur (void * arg)
{
  int i, n;

  for (i=0; i < ITERATIONS ; i++) {
    send_message (&tube_1, (char *)&i, sizeof(int)) ;
    receive_message (&tube_2, (char *)&n, sizeof(int)) ; 
  }
  return NULL;
}

#ifdef __ACT__
extern int nb;
extern unsigned long temps_act;
#endif
void * consommateur (void * arg)
{
  int i, n ;
  tbx_tick_t t1, t2;
  unsigned long temps;

  for (i=0; i < 3 ; i++) {
    receive_message (&tube_1, (char *)&n, sizeof(int)) ; 
    send_message (&tube_2, (char *)&i, sizeof(int)) ;
  }
  TBX_GET_TICK(t1);
  for (i=3; i < ITERATIONS ; i++) {
    receive_message (&tube_1, (char *)&n, sizeof(int)) ; 
    send_message (&tube_2, (char *)&i, sizeof(int)) ;
  }
  TBX_GET_TICK(t2);

  temps = TBX_TIMING_DELAY(t1, t2);
  printf("time = %ld.%03ldms\n", temps/1000, temps%1000);
#ifdef __ACT__
  printf("nb=%i\n",nb);
  printf("dont time idle = %ld.%03ldms\n", temps_act/1000, temps_act%1000);
#endif

  return NULL;
}


int marcel_main(int argc, char **argv)
{
  marcel_attr_t attr;
  marcel_t pid[2];

  create_tube (&tube_1) ;
  create_tube (&tube_2) ;

#ifdef PTHREAD
  timing_init();
#else
  marcel_init(&argc, argv);
#endif

  marcel_attr_init(&attr);

#ifdef SMP
  marcel_attr_setschedpolicy(&attr, MARCEL_SCHED_FIXED(0));
#endif
  marcel_create (&pid[0], &attr, consommateur, (void *) 0) ;
#ifdef SMP
  marcel_attr_setschedpolicy(&attr, MARCEL_SCHED_FIXED(1));
#endif
  marcel_create (&pid[1], &attr, producteur, (void *) 0) ;

  marcel_join(pid[0], NULL); marcel_join(pid[1], NULL);

#ifndef PTHREAD
  marcel_end();
#endif

  return 0;
}
