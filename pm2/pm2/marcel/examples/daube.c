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

#include <marcel.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

typedef struct {
  int tub [2] ;
  marcel_sem_t sem ;
} tube ;

tube tube_global ;

void create_tube (tube *t)
{
  marcel_sem_init (&t->sem, 1) ;
  pipe (t->tub) ;
}

void delete_tube (tube *t)
{
  close (t->tub[0]) ;
  close (t->tub[1]) ;
}

int send_message (tube *t, char *message, int len)
{
  int n;

  marcel_sem_P (&t->sem) ;
  n = write (t->tub[1], message, len) ;
  marcel_sem_V (&t->sem) ;

  return n ;
}

int receive_message (tube *t, char *message, int len)
{
  int n;

  n = read (t->tub[0], message, len) ;
    
  return n ;
}

any_t producteur (any_t arg)
{
  int i;

  for (i=0; i < 100 ; i++) {
    marcel_yield();
    send_message (&tube_global, (char *)&i, sizeof(int)) ;
  }
  return NULL;
}

any_t consommateur (any_t arg)
{
  int i, n ;
  Tick t1, t2;
  unsigned long temps;

  GET_TICK(t1);
  for (i=0; i < 100 ; i++) {
    receive_message (&tube_global, (char *)&n, sizeof(int)) ; 
  }
  GET_TICK(t2);

  temps = timing_tick2usec(TICK_DIFF(t1, t2));
  printf("time = %ld.%03ldms\n", temps/1000, temps%1000);

  return NULL;
}


int marcel_main(int argc, char **argv)
{
  marcel_attr_t attr;

  create_tube (&tube_global) ;

  marcel_init(&argc, argv);

  marcel_attr_init(&attr);
  marcel_attr_setdetachstate(&attr, TRUE);

  marcel_create (NULL, &attr, producteur, (void *) 0) ;
  marcel_create (NULL, &attr, consommateur, (void *) 0) ;

  marcel_end();
  delete_tube (&tube_global) ;

  return 0;
}


