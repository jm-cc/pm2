
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

#if defined(MA__LIBPTHREAD) && !defined(MA__LWPS)
#warning "marcel/pmarcel/pthread_spin_lock won't work in mono mode"
#endif

DEF_MARCEL_POSIX(int, spin_init,(marcel_spinlock_t *lock, int pshared TBX_UNUSED),(lock,pshared),
{
#ifdef MA__DEBUG
	if ((pshared != MARCEL_PROCESS_SHARED)
	    && (pshared != MARCEL_PROCESS_PRIVATE)) {
		fprintf(stderr,
		    "(p)marcel_spin_init : valeur pshared(%d) invalide",
		    pshared);
		return EINVAL;
	}
#endif
	ma_spin_lock_init(&lock->lock);
	return 0;
})

DEF_PTHREAD(int, spin_init,(pthread_spinlock_t *lock, int pshared),(lock,pshared));
DEF___PTHREAD(int, spin_init,(pthread_spinlock_t *lock, int pshared),(lock,pshared));


/* POSIX destroy :The pthread_spin_destroy() function shall destroy the spin lock referenced by lock and release any resources used by the lock. The effect of subsequent use of the lock is undefined until the lock is reinitialized by another call to pthread_spin_init().
 */

DEF_MARCEL_POSIX(int, spin_destroy,(marcel_spinlock_t *lock),(lock),
{
	memset(lock, 0x7E, sizeof(*lock));
	return 0;
})

DEF_PTHREAD(int, spin_destroy,(pthread_spinlock_t *lock),(lock));
DEF___PTHREAD(int, spin_destroy,(pthread_spinlock_t *lock),(lock));


DEF_MARCEL_POSIX(int, spin_lock, (marcel_spinlock_t *lock),(lock),
{
	marcel_thread_preemption_disable();
	_ma_raw_spin_lock(&lock->lock);
	return 0;
})

DEF_PTHREAD(int, spin_lock,(pthread_spinlock_t *lock),(lock,pshared));
DEF___PTHREAD(int, spin_lock,(pthread_spinlock_t *lock),(lock,pshared));


DEF_MARCEL_POSIX(int, spin_trylock, (marcel_spinlock_t *lock),(lock),
{
	marcel_thread_preemption_disable();
	if (!_ma_raw_spin_trylock(&lock->lock)) {
		marcel_thread_preemption_enable();
		return EBUSY;
	}
	return 0;
})

DEF_PTHREAD(int, spin_trylock,(pthread_spinlock_t *lock),(lock,pshared));
DEF___PTHREAD(int, spin_trylock,(pthread_spinlock_t *lock),(lock,pshared));


DEF_MARCEL_POSIX(int, spin_unlock, (marcel_spinlock_t *lock),(lock),
{
	_ma_raw_spin_unlock(&lock->lock);
	marcel_thread_preemption_enable();
	return 0;
})

DEF_PTHREAD(int, spin_unlock,(pthread_spinlock_t *lock),(lock));
DEF___PTHREAD(int, spin_unlock,(pthread_spinlock_t *lock),(lock));
