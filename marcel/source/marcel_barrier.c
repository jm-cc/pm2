
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2006 "the PM2 team" (see AUTHORS file)
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

#include "marcel.h"

DEF_MARCEL_POSIX(int, barrier_init, (marcel_barrier_t * __restrict b,
		const marcel_barrierattr_t * __restrict attr, unsigned num),
		(b, attr, num),
{
	marcel_cond_init(&b->cond, NULL);
	marcel_mutex_init(&b->mutex, NULL);
	b->num = num;
	b->curwait = 0;
	return 0;
})
unsigned marcel_barrier_begin(marcel_barrier_t *b) {
	unsigned num;
	marcel_mutex_lock(&b->mutex);
	if ((num = ++b->curwait) == b->num) {
		b->curwait = 0;
		marcel_cond_broadcast(&b->cond);
	}
	marcel_mutex_unlock(&b->mutex);
	return num;
}
void marcel_barrier_end(marcel_barrier_t *b, unsigned num) {
	marcel_mutex_lock(&b->mutex);
	if (num != b->num)
		marcel_cond_wait(&b->cond,&b->mutex);
	marcel_mutex_unlock(&b->mutex);
}
DEF_MARCEL_POSIX(int, barrier_wait, (marcel_barrier_t *b), (b),
{
	int ret = 0;
	marcel_mutex_lock(&b->mutex);
	if (++b->curwait == b->num) {
		b->curwait = 0;
		ret = MARCEL_BARRIER_SERIAL_THREAD;
		marcel_cond_broadcast(&b->cond);
	} else
		marcel_cond_wait(&b->cond,&b->mutex);
	marcel_mutex_unlock(&b->mutex);
	return ret;
})
