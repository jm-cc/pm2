
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

#ifndef MARCEL_MUTEX_EST_DEF
#define MARCEL_MUTEX_EST_DEF

#include <sys/time.h>

//#define MARCEL_MUTEX_INITIALIZER { 1 }

//_PRIVATE_ typedef struct {
//	int dummy;
//} marcel_mutexattr_struct;

//_PRIVATE_ typedef marcel_sem_t marcel_mutex_t;

//_PRIVATE_ typedef marcel_sem_t marcel_cond_t;

//typedef marcel_mutexattr_struct marcel_mutexattr_t;

//typedef marcel_condattr_struct marcel_condattr_t;

DEC_MARCEL(int, mutex_init, (marcel_mutex_t *mutex,
			     __const marcel_mutexattr_t *attr))
DEC_MARCEL(int, mutex_destroy, (marcel_mutex_t *mutex))

DEC_MARCEL(int, mutex_lock, (marcel_mutex_t *mutex))
DEC_MARCEL(int, mutex_trylock, (marcel_mutex_t *mutex))
DEC_MARCEL(int, mutex_unlock, (marcel_mutex_t *mutex))

DEC_MARCEL(int, mutexattr_init, (marcel_mutexattr_t *attr))
DEC_MARCEL(int, mutexattr_destroy, (marcel_mutexattr_t *attr))



DEC_POSIX(int, mutex_init, (pmarcel_mutex_t *mutex,
			    __const pmarcel_mutexattr_t *attr))
DEC_POSIX(int, mutex_destroy, (pmarcel_mutex_t *mutex))

DEC_POSIX(int, mutex_lock, (pmarcel_mutex_t *mutex))
DEC_POSIX(int, mutex_trylock, (pmarcel_mutex_t *mutex))
DEC_POSIX(int, mutex_unlock, (pmarcel_mutex_t *mutex))

DEC_POSIX(int, mutexattr_init, (pmarcel_mutexattr_t *attr))
DEC_POSIX(int, mutexattr_destroy, (pmarcel_mutexattr_t *attr))
DEC_POSIX(int, mutexattr_settype, (pmarcel_mutexattr_t *attr, int kind))
DEC_POSIX(int, mutexattr_setkind_np, (pmarcel_mutexattr_t *attr, int kind))
DEC_POSIX(int, mutexattr_gettype, (const pmarcel_mutexattr_t *attr, int *kind))
DEC_POSIX(int, mutexattr_getkind_np, (const pmarcel_mutexattr_t *attr, int *kind))
DEC_POSIX(int, mutexattr_getpshared, (const pmarcel_mutexattr_t *attr,
				      int *pshared))
DEC_POSIX(int, mutexattr_setpshared, (pmarcel_mutexattr_t *attr, int pshared))

DEC_POSIX(int, once, (pmarcel_once_t * once_control, 
		      void (*init_routine)(void)))
#ifdef MA__PTHREAD_FUNCTIONS
extern void __pthread_once_fork_prepare (void);
extern void __pthread_once_fork_parent (void);
extern void __pthread_once_fork_child (void);
extern void __pmarcel_once_fork_prepare (void);
extern void __pmarcel_once_fork_parent (void);
extern void __pmarcel_once_fork_child (void);
extern void __flockfilelist (void);
extern void __funlockfilelist (void);
extern void __fresetlockfiles (void);
#endif
#endif
