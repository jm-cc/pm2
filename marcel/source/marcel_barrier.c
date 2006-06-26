
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
#include <errno.h>

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

DEF_POSIX(int, barrier_destroy, (pmarcel_barrier_t *b),(b),
{
#ifdef MA__DEBUG
    if (b->curwait == 1)
    {
        errno = EBUSY;
        return -1;
    }
#endif
    return marcel_barrier_destroy(b);
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


/************************barrierattr*****************************/
DEF_POSIX(int,barrierattr_init,(marcel_barrierattr_t *attr),(attr),
{
    attr->__pshared = PTHREAD_PROCESS_PRIVATE; 
    return 0; 
})

DEF_POSIX(int,barrierattr_destroy,(marcel_barrierattr_t *attr),(attr),
{
    attr->__pshared = -1;
    return 0;
})


DEF_POSIX(int,barrierattr_setpshared,(marcel_barrierattr_t *attr,
                 int pshared),(attr,pshared),
{
#ifdef MA__DEBUG
    if ((pshared != PTHREAD_PROCESS_SHARED)
      &&(pshared != PTHREAD_PROCESS_PRIVATE))
    {
        errno = EINVAL;
        return -1;
    } 
#endif
    if (pshared != PTHREAD_PROCESS_PRIVATE)
    {
        errno = ENOTSUP;
        return -1;
    } 
    attr->__pshared = pshared;
    return 0;
}) 


DEF_POSIX(int, barrierattr_getpshared,(const marcel_barrierattr_t *
                 __restrict attr, int *__restrict pshared),(attr,pshared),
{
#ifdef MA__DEBUG
    if (pshared == NULL)
    {
        errno = EINVAL;
        return -1;
    }
    if ((attr->__pshared != PTHREAD_PROCESS_SHARED)
      &&(attr->__pshared != PTHREAD_PROCESS_PRIVATE))
    {
        errno = EINVAL;
        return -1;
    } 
    if (attr->__pshared != PTHREAD_PROCESS_PRIVATE)
    {
        errno = ENOTSUP;
        return -1;
    } 
#endif 
    *pshared = attr-> __pshared;
    return 0;
})
