#include <stdio.h>
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <sys/time.h>

#include "tsp-types.h"
#include "tsp.h"
#include "tsp-job.h"

#ifdef	MARCEL
# ifdef	MT
#  undef MT
# endif	/* MT */
# define MT
#endif	/* MARCEL */

int    InitSeed = 20 ;

int    minimum ;

#ifdef MT
#  ifdef MARCEL
marcel_mutex_t mutex ;
#  else
pthread_mutex_t mutex ;
#  endif
#endif

int          nb_workers ;
TSPqueue     q ;
struct s_distance_table distance;


void genmap (struct s_distance_table *map)
{
const int n = map->NrTowns;
int tempdist [MAXE] ;
struct {
	int x;
	int y;
} towns[MAXE];
int i, j, k, x =0 ;
int dx, dy, tmp ;


 for (i=0; i<n; i++)
  {
   towns [i].x = rand () % MAXX ;
   towns [i].y = rand () % MAXY ;
  }

 for (i=0; i<n; i++)
  {
   for (j=0; j<n; j++)
    {
     dx = towns [i].x - towns [j].x ;
     dy = towns [i].y - towns [j].y ;
     tempdist [j] = (int) sqrt ((double) ((dx * dx) + (dy * dy))) ;
    }

   for (j=0; j<n; j++)
    {
     tmp = INT_MAX ;
     for (k=0; k<n; k++)
      {
       if (tempdist [k] < tmp)
         {
          tmp = tempdist [k] ;
          x = k ;
         }
      }
     tempdist [x] = INT_MAX ;
     map->dst[i][j].ToCity = x ;
     map->dst[i][j].dist = tmp ;
    }
  }
}
 
void PrintDistTab ()
{
 int i, j ;

 marcel_printf ("distance.NrTowns = %d\n", distance.NrTowns) ;

 for (i=0; i<distance.NrTowns; i++)
  {
   marcel_printf ("distance.dst [%1d]",i) ;
   for (j=0; j<distance.NrTowns; j++)
    {
     marcel_printf (" [d:%2d, to:%2d] ", distance.dst[i][j].dist, distance.dst[i][j].ToCity) ;
    }
   marcel_printf (";\n\n") ;
  }
 marcel_printf ("done ...\n") ;

}


int marcel_main (int argc, char **argv)
{
  tbx_tick_t t1, t2;
#ifdef MT
  long i ;
  void *status ;

#ifdef MARCEL
  marcel_t *tids ;
  marcel_attr_t att ;
#else
  pthread_t *tids ;
  pthread_attr_t att ;
#endif
#endif
  unsigned long temps;

#ifdef MARCEL
 marcel_init (&argc, argv) ;
#endif

 if (argc != 4)
   {
    marcel_fprintf (stderr, "Usage: %s <nb_threads > <ncities> <seed> \n",argv[0]) ;
    exit (1) ;
   }

 srand (atoi(argv[3])) ;

#ifdef MT
#ifdef MARCEL
 marcel_mutex_init (&mutex, NULL) ; 
#else
 pthread_mutex_init (&mutex, NULL) ; 
#endif
#endif

 minimum = INT_MAX ;
 nb_workers = atoi (argv[1]) ;

 marcel_printf ("nb_threads = %3d ncities = %3d\n", nb_workers, atoi(argv[2])) ;

 init_queue (&q) ;
 distance.NrTowns = atoi (argv[2]) ;
 genmap (&distance) ; 

 GenerateJobs () ;

 TBX_GET_TICK(t1);
#ifdef MT

#  ifdef MARCEL

 marcel_attr_init (&att) ;


 tids = (marcel_t *) malloc (sizeof(marcel_t) * nb_workers) ;
 marcel_attr_setschedpolicy(&att, MARCEL_SCHED_AFFINITY);

 for (i=0; i<nb_workers; i++)
  {
   marcel_create (&tids [i], &att, worker, (void *)i) ;
  }

 for (i=0; i<nb_workers; i++)
  {
   marcel_join (tids[i], &status) ;
  }

#  else
 pthread_attr_init (&att) ;
 /* pthread_attr_setstacksize (&att, 20000) ; */
 pthread_attr_setscope (&att, PTHREAD_SCOPE_SYSTEM) ;

 tids = (pthread_t *) malloc (sizeof(pthread_t) * nb_workers) ;

 for (i=0; i<nb_workers; i++)
  {
   pthread_create (&tids [i], &att, worker, (void *)i) ;
  }

 for (i=0; i<nb_workers; i++)
  {
   pthread_join (tids[i], &status) ;
  }
#  endif
#else

    worker ((void *)1) ;

#endif
 TBX_GET_TICK(t2);
 temps = TBX_TIMING_DELAY(t1, t2);
 marcel_printf("time = %ld.%03ldms\n", temps/1000, temps%1000);
 marcel_end();
 return 0 ;
}

