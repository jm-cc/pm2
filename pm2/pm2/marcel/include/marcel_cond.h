
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

#ifndef MARCEL_COND_EST_DEF
#define MARCEL_COND_EST_DEF

#include <sys/time.h>

_PRIVATE_ typedef struct {
	int dummy;
} marcel_condattr_struct;

DEC_MARCEL(int, condattr_init, (marcel_condattr_t *attr))
DEC_MARCEL(int, condattr_destroy, (marcel_condattr_t *attr))

DEC_MARCEL(int, cond_init, (marcel_cond_t *cond,
			    __const marcel_condattr_t *attr))
DEC_MARCEL(int, cond_destroy, (marcel_cond_t *cond))

DEC_MARCEL(int, cond_signal, (marcel_cond_t *cond))
DEC_MARCEL(int, cond_broadcast, (marcel_cond_t *cond))

DEC_MARCEL(int, cond_wait, (marcel_cond_t *cond,
			    marcel_mutex_t *mutex))
DEC_MARCEL(int, cond_timedwait, (marcel_cond_t *cond, marcel_mutex_t *mutex,
				 const struct timespec *abstime))


DEC_POSIX(int, condattr_init, (pmarcel_condattr_t *attr))
DEC_POSIX(int, condattr_destroy, (pmarcel_condattr_t *attr))

DEC_POSIX(int, cond_init, (pmarcel_cond_t *cond,
			   __const pmarcel_condattr_t *attr))
DEC_POSIX(int, cond_destroy, (pmarcel_cond_t *cond))

DEC_POSIX(int, cond_signal, (pmarcel_cond_t *cond))
DEC_POSIX(int, cond_broadcast, (pmarcel_cond_t *cond))

DEC_POSIX(int, cond_wait, (pmarcel_cond_t *cond,
			   pmarcel_mutex_t *mutex))
DEC_POSIX(int, cond_timedwait, (pmarcel_cond_t *cond, pmarcel_mutex_t *mutex,
				const struct timespec *abstime))
#endif
