
/* This file has been autogenerated from source/nptl_barrier.c.m4 */
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
#include "marcel.h"

#include <errno.h>

#include "marcel_fastlock.h"

int marcel_barrierattr_init(marcel_barrierattr_t *attr) {
	LOG_IN();
	attr->pshared	= MARCEL_PROCESS_PRIVATE;
	attr->mode	= MA_BARRIER_SLEEP_MODE;
	LOG_RETURN(0);
}
int marcel_barrierattr_destroy (marcel_barrierattr_t *attr TBX_UNUSED) {
	LOG_IN();
	LOG_RETURN(0);
}
int marcel_barrierattr_getpshared(__const marcel_barrierattr_t *attr, int *pshared) {
	LOG_IN();
	*pshared = attr->pshared;
	LOG_RETURN(0);
}
int marcel_barrierattr_setpshared (marcel_barrierattr_t *attr, int pshared) {
	LOG_IN();
	if (pshared != MARCEL_PROCESS_PRIVATE
	    && __builtin_expect(pshared != MARCEL_PROCESS_SHARED, 0)) {
		LOG_RETURN(EINVAL);
	}
	if (pshared == MARCEL_PROCESS_SHARED) {
		LOG_RETURN(ENOTSUP);
	}
	attr->pshared = pshared;
	LOG_RETURN(0);
}
int marcel_barrierattr_getmode(__const marcel_barrierattr_t *attr, ma_barrier_mode_t *mode) {
	LOG_IN();
	*mode = attr->mode;
	LOG_RETURN(0);
}
int marcel_barrierattr_setmode (marcel_barrierattr_t *attr, ma_barrier_mode_t mode) {
	LOG_IN();
	attr->mode = mode;
	LOG_RETURN(0);
}
int marcel_barrier_setcount(marcel_barrier_t *b, unsigned int count) {
	LOG_IN();
#ifdef MA_BARRIER_USE_MUTEX
	marcel_mutex_lock(&b->m);
#else
	ma_spin_lock(&b->lock.__spinlock);
#endif
	b->init_count = count;
	ma_atomic_set(&b->leftB, count);
	ma_atomic_set(&b->leftE, 0);
#ifdef MA_BARRIER_USE_MUTEX
	marcel_cond_signal(&b->c);
	marcel_mutex_unlock(&b->m);
#else
	ma_spin_unlock(&b->lock.__spinlock);
#endif
	LOG_RETURN(0);
}
int marcel_barrier_getcount(const marcel_barrier_t * b,
		unsigned int *count) {
	LOG_IN();
	*count = b->init_count;
	LOG_RETURN(0);
}
int marcel_barrier_addcount(marcel_barrier_t *b, int v) {
	LOG_IN();
#ifdef MA_BARRIER_USE_MUTEX
	marcel_mutex_lock(&b->m);
#else
	ma_spin_lock(&b->lock.__spinlock);
#endif
	b->init_count	+= v;
	ma_atomic_add(v, &b->leftB);
#ifdef MA_BARRIER_USE_MUTEX
	marcel_cond_signal(&b->c);
	marcel_mutex_unlock(&b->m);
#else
	ma_spin_unlock(&b->lock.__spinlock);
#endif
	LOG_RETURN(0);
}
int marcel_barrier_init(marcel_barrier_t *b,
		__const marcel_barrierattr_t *attr,
		unsigned int count) {
	ma_barrier_mode_t mode;
	LOG_IN();
	mode = MA_BARRIER_SLEEP_MODE;
	if (__builtin_expect(count == 0, 0)) {
		LOG_RETURN(EINVAL);
	}
	if (attr != NULL) {
		if (attr->pshared != MARCEL_PROCESS_PRIVATE
		    && __builtin_expect(attr->pshared != MARCEL_PROCESS_SHARED,
			0)) {
			LOG_RETURN(EINVAL);
		}

		if (attr->pshared == MARCEL_PROCESS_SHARED) {
			LOG_RETURN(ENOTSUP);
		}

		mode = attr->mode;
	}
#ifdef MA_BARRIER_USE_MUTEX
	marcel_mutex_init(&b->m, NULL);
	marcel_cond_init(&b->c, NULL);
#else
	b->lock = (struct _marcel_fastlock) MA_MARCEL_FASTLOCK_UNLOCKED;
#endif
	b->init_count = count;
	ma_atomic_init(&b->leftB, count);
	ma_atomic_init(&b->leftE, 0);
	b->mode = mode;
	LOG_RETURN(0);
}


int marcel_barrier_destroy (marcel_barrier_t *b) {
	int ret = 0;

	LOG_IN();
	if (b->mode == MA_BARRIER_SLEEP_MODE)
#ifdef MA_BARRIER_USE_MUTEX
		marcel_mutex_lock(&b->m);
#else
		ma_spin_lock(&b->lock.__spinlock);
#endif

	if (b->mode == MA_BARRIER_SLEEP_MODE) {
		while (ma_atomic_read(&b->leftE)) {
#ifdef MA_BARRIER_USE_MUTEX
			marcel_cond_wait(&b->c, &b->m);
#else
			blockcell c;
			__marcel_register_spinlocked(&b->lock, marcel_self(),
					&c);
			INTERRUPTIBLE_SLEEP_ON_CONDITION_RELEASING(c.blocked,
					ma_spin_unlock(&b->lock.__spinlock),
					ma_spin_lock(&b->lock.__spinlock));
#endif
		}
	} else {
		while (ma_atomic_read(&b->leftE))
			marcel_yield();
	}
	if (__builtin_expect(ma_atomic_read(&b->leftB) != b->init_count, 0)) {
		ret = EBUSY;
	}
	if (b->mode == MA_BARRIER_SLEEP_MODE)
#ifdef MA_BARRIER_USE_MUTEX
		marcel_mutex_unlock(&b->m);
#else
		ma_spin_unlock(&b->lock.__spinlock);
#endif
	LOG_RETURN(ret);
}
int marcel_barrier_wait(marcel_barrier_t *b) {
	int ret = 0;
	LOG_IN();
	marcel_barrier_wait_begin(b);
	if (!marcel_barrier_wait_end(b))
		ret = MARCEL_BARRIER_SERIAL_THREAD;
	LOG_RETURN(ret);
}
int marcel_barrier_wait_begin(marcel_barrier_t *b) {
	int ret;
	LOG_IN();
	if (b->mode == MA_BARRIER_SLEEP_MODE)
#ifdef MA_BARRIER_USE_MUTEX
		marcel_mutex_lock(&b->m);
#else
		ma_spin_lock(&b->lock.__spinlock);
#endif
	if (b->mode == MA_BARRIER_SLEEP_MODE)
		while (ma_atomic_read(&b->leftE)) {
#ifdef MA_BARRIER_USE_MUTEX
			marcel_cond_wait(&b->c, &b->m);
#else
			blockcell c;
			__marcel_register_spinlocked(&b->lock, marcel_self(),
					&c);
			INTERRUPTIBLE_SLEEP_ON_CONDITION_RELEASING(c.blocked,
					ma_spin_unlock(&b->lock.__spinlock),
					ma_spin_lock(&b->lock.__spinlock));
#endif
		}
	else
		while(ma_atomic_read(&b->leftE))
			marcel_yield();
	ret = ma_atomic_dec_return(&b->leftB);
	if (!ret) {
		if (b->mode == MA_BARRIER_SLEEP_MODE) {
#ifdef MA_BARRIER_USE_MUTEX
#else
			while (__marcel_unlock_spinlocked(&b->lock));
#endif
		}
		ma_atomic_set(&b->leftB, b->init_count);
		ma_atomic_set(&b->leftE, b->init_count);
	}
	if (b->mode == MA_BARRIER_SLEEP_MODE)
#ifdef MA_BARRIER_USE_MUTEX
		marcel_cond_signal(&b->c);
		marcel_mutex_unlock(&b->m);
#else
		ma_spin_unlock(&b->lock.__spinlock);
#endif
	LOG_RETURN(ret);
}
int marcel_barrier_wait_end(marcel_barrier_t *b) {
	int ret;
	LOG_IN();
	if (b->mode == MA_BARRIER_SLEEP_MODE)
#ifdef MA_BARRIER_USE_MUTEX
		marcel_mutex_lock(&b->m);
#else
		ma_spin_lock(&b->lock.__spinlock);
#endif
	if (b->mode == MA_BARRIER_SLEEP_MODE) {
		while (!ma_atomic_read(&b->leftE)) {
#ifdef MA_BARRIER_USE_MUTEX
			marcel_cond_wait(&b->c, &b->m);
#else
			blockcell c;
			__marcel_register_spinlocked(&b->lock, marcel_self(),
					&c);
			INTERRUPTIBLE_SLEEP_ON_CONDITION_RELEASING(c.blocked,
					ma_spin_unlock(&b->lock.__spinlock),
					ma_spin_lock(&b->lock.__spinlock));
#endif
		}
	} else {
		while(!ma_atomic_read(&b->leftE))
			marcel_yield();
	}
	ret = ma_atomic_dec_return(&b->leftE);
	if (b->mode == MA_BARRIER_SLEEP_MODE) {
#ifdef MA_BARRIER_USE_MUTEX
		marcel_cond_signal(&b->c);
		marcel_mutex_unlock(&b->m);
#else
		if (!ret)
			while (__marcel_unlock_spinlocked(&b->lock));
		ma_spin_unlock(&b->lock.__spinlock);
#endif
	}
	LOG_RETURN(ret);
}

#ifdef MA__IFACE_PMARCEL
int pmarcel_barrierattr_init(pmarcel_barrierattr_t *attr) {
	return marcel_barrierattr_init(attr);
}
int pmarcel_barrierattr_destroy (pmarcel_barrierattr_t *attr) {
	return marcel_barrierattr_destroy(attr);
}
int pmarcel_barrierattr_getpshared(__const pmarcel_barrierattr_t *attr, int *pshared) {
	return marcel_barrierattr_getpshared(attr, pshared);
}
int pmarcel_barrierattr_setpshared (pmarcel_barrierattr_t *attr, int pshared) {
	return marcel_barrierattr_setpshared(attr, pshared);
}
int pmarcel_barrierattr_getmode(__const pmarcel_barrierattr_t *attr, ma_barrier_mode_t *mode) {
	return marcel_barrierattr_getmode(attr, mode);
}
int pmarcel_barrierattr_setmode (pmarcel_barrierattr_t *attr, ma_barrier_mode_t mode) {
	return marcel_barrierattr_setmode(attr, mode);
}
int pmarcel_barrier_init(pmarcel_barrier_t *b,
			__const pmarcel_barrierattr_t *attr,
			unsigned int count) {
	return marcel_barrier_init(b, attr, count);
}
int pmarcel_barrier_destroy (pmarcel_barrier_t *b) {
	return marcel_barrier_destroy(b);
}
int pmarcel_barrier_wait(pmarcel_barrier_t *b) {
	return marcel_barrier_wait(b);
}
int pmarcel_barrier_wait_begin(pmarcel_barrier_t *b) {
	return marcel_barrier_wait_begin(b);
}
int pmarcel_barrier_wait_end(pmarcel_barrier_t *b) {
	return marcel_barrier_wait_end(b);
}
#endif

