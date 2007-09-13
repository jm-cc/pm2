
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

#ifdef MT
#  ifdef MARCEL
#    define MUTEX_INIT(m, a) marcel_mutex_init((m), (a))
#    define MUTEX_LOCK(m)    marcel_mutex_lock((m))
#    define MUTEX_UNLOCK(m)  marcel_mutex_unlock((m))
#  else
#    define MUTEX_INIT(m, a) pthread_mutex_init((m), (a))
#    define MUTEX_LOCK(m)    pthread_mutex_lock((m))
#    define MUTEX_UNLOCK(m)  pthread_mutex_unlock((m))
#  endif
#else
#  define MUTEX_INIT(m, a) 
#  define MUTEX_LOCK(m)
#  define MUTEX_UNLOCK(m)
#endif

extern void init_queue (struct s_tsp_queue *q);

extern void add_job (struct s_tsp_queue *q, struct s_job j);

extern int get_job (struct s_tsp_queue *q, struct s_job *j);
