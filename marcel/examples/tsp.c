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

#include "tsp-types.h"
#include "tsp-job.h"

#ifdef	MARCEL
# ifdef	MT
#  undef MT
# endif	/* MT */
# define MT
#endif	/* MARCEL */

extern int          minimum ;
extern struct s_tsp_queue     q ;
extern struct s_distance_table distance;
extern MUTEX_T(mutex);

#define MAXHOPS		3

int present (int city, int hops, Path_t path)
{
 unsigned int i ;

 for (i=0; i<hops; i++)
   if (path [i] == city) return 1 ;
 return 0 ;
}

void tsp (int hops, int len, Path_t path, int *cuts, int num_worker)
{
 int i ;
 int city, me, dist ;

 if (len >= minimum)
  {
   (*cuts)++ ;
   return;
  }

 if (hops == distance.n)
  {
    MUTEX_LOCK(&mutex);
    if (len < minimum)
      {
       minimum = len ;
       marcel_printf ("worker[%d] finds path len = %3d :", num_worker, len) ;
       for (i=0; i < distance.n; i++)
         marcel_printf ("%2d ", path[i]) ;
       marcel_printf ("\n") ;
      }
    MUTEX_UNLOCK(&mutex);
  }
 else
  {
   me = path [hops-1] ;

   for (i=0; i < distance.n; i++)
    {
     city = distance.t [me][i].to ;
     if (!present (city, hops, path))
      {
       path [hops] = city ;
       dist = distance.t[me][i].dist ;
       tsp (hops+1, len+dist, path, cuts, num_worker) ;
      }
    }
  }
}

void distributor (int hops, int len, Path_t path, struct s_tsp_queue *q)
{
 struct s_job j ;
 int i ;
 int me, city, dist ;

 if (hops == MAXHOPS)
  {
   j.len = len ;
   for (i=0; i<hops;i++)
     j.path[i] = path[i] ;
   add_job (q, j) ;
  }
 else
  {
   me = path [hops-1] ;
   for (i=0;i<distance.n;i++)
    {
     city = distance.t[me][i].to ;
     if (!present(city,hops, path))
      {
       path [hops] = city ;
       dist = distance.t[me][i].dist ;
       distributor (hops+1, len+dist, path, q) ;       
      }
    }
  } 
}

void GenerateJobs () 
{
 Path_t path ;

 path [0] = 0 ;
 distributor (1, 0, path, &q) ; 
 no_more_jobs (&q) ;
}

void *worker (int num_worker)
{
 int jobcount = 0 ;
 struct s_job job ;
 int cuts = 0 ;

 marcel_printf ("Worker [%2d] starts \n", num_worker) ;

#ifdef MARCEL
 marcel_printf ("thread %p on LWP %d\n", marcel_self(), marcel_current_vp ()) ;
#endif

 while (get_job (&q, &job)) 
   {
    jobcount++ ;
    tsp (MAXHOPS, job.len, job.path, &cuts, num_worker) ;
   }

 marcel_printf ("Worker [%2d] terminates, %4d jobs done with %4d cuts.\n", num_worker, jobcount, cuts) ;
 return (void *) 0 ;
}




