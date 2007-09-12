
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

#define MAXE	30
#define MAXX	100
#define MAXY	100

#ifdef	MARCEL
# ifdef	MT
#  undef MT
# endif	/* MT */
# define MT
#endif	/* MARCEL */

#ifdef MT
#  ifdef MARCEL
#    include "marcel.h"
#  else
#    include <pthread.h>
#  endif
#endif

#ifdef MT
#  ifdef MARCEL
#    define MUTEX_T(m) marcel_mutex_t (m)
#  else
#    define MUTEX_T(m) pthread_mutex_t (m)
#  endif
#else
#  define MUTEX_T(m)
#endif

struct s_distance_table {
	int NrTowns;
	struct {
		int ToCity;
		int dist;
	} dst[MAXE][MAXE];
};

/* Job types */

typedef int Path_t [MAXE] ;

typedef struct {
		int len ;
		Path_t path ;
	       } Job_t ;

/* TSQ Queue */

typedef struct Maillon 
             {
	      Job_t tsp_job ;
              struct Maillon *next ;
	     } Maillon ;

typedef struct {
                Maillon *first ;
		Maillon *last ;
		int end;

		MUTEX_T(mutex);
               } TSPqueue ;
		



