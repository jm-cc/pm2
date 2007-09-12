
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

/** \brief Table of distances
 */
struct s_distance_table {
	int n;			/**< \brief Number of towns			*/
	struct {
		int to;		/**< \brief Destination city			*/
		int dist;	/**< \brief Distance to destination city	*/
	} t[MAXE][MAXE];
};

/** \brief One path
 */
typedef int Path_t [MAXE] ;

/** \brief One job
 */
struct s_job {
	int len;
	Path_t path;
};

/** \brief Item of the job queue
 */
struct s_maillon {
	struct s_job tsp_job;
	struct s_maillon *next;
};

/** \brief Queue of jobs
 */ 
struct s_tsp_queue {
	struct s_maillon *first;
	struct s_maillon *last;
	MUTEX_T(mutex);
};
		



