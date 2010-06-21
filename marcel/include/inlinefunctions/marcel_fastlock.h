
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


#ifndef __INLINEFUNCTIONS_MARCEL_FASTLOCK_H__
#define __INLINEFUNCTIONS_MARCEL_FASTLOCK_H__


#ifdef __MARCEL_KERNEL__


#include "marcel_sched_generic.h"


__tbx_inline__ static int lpt_lock_acquire(long int *lock)
{
	ma_bit_spin_lock(0, (unsigned long int *) lock);
	return 0;
}

__tbx_inline__ static int lpt_lock_release(long int *lock)
{
	ma_bit_spin_unlock(0, (unsigned long int *) lock);
	return 0;
}

__tbx_inline__ static void lpt_fastlock_acquire(struct _lpt_fastlock *fastlock)
{
	lpt_lock_acquire(&fastlock->__status);
}

__tbx_inline__ static void lpt_fastlock_release(struct _lpt_fastlock *fastlock)
{
	lpt_lock_release(&fastlock->__status);
}


TBX_VISIBILITY_PUSH_INTERNAL
    __tbx_inline__ static void ma_fastlock_acquire(struct _marcel_fastlock *fastlock)
{
	ma_spin_lock(&fastlock->__spinlock);
}

__tbx_inline__ static void ma_fastlock_release(struct _marcel_fastlock *fastlock)
{
	ma_spin_unlock(&fastlock->__spinlock);
}

struct blockcell_struct {
	marcel_t task;
	struct blockcell_struct *next;
	struct blockcell_struct *last;	/* Valide uniquement dans la cellule de tête */
	volatile tbx_bool_t blocked;
};

/* `lpt_blockcell_t' objects must be 2-byte aligned.  See `struct
   _lpt_fastlock' for details.  */
typedef struct blockcell_struct blockcell __attribute__ ((__aligned__((2))));

/* `lpt_blockcell_t' objects must be 4-byte aligned.  See `struct
   _lpt_fastlock' for details.  */
typedef struct blockcell_struct lpt_blockcell_t __attribute__ ((__aligned__((4))));



__tbx_inline__ static int __marcel_init_lock(struct _marcel_fastlock *lock)
{
	MARCEL_LOG_IN();
	*lock = (struct _marcel_fastlock) MA_MARCEL_FASTLOCK_UNLOCKED;
	MARCEL_LOG_RETURN(0);
}

#define __pmarcel_init_lock(lock) __marcel_init_lock(lock)
__tbx_inline__ static int __lpt_init_lock(struct _lpt_fastlock *lock)
{
	MARCEL_LOG_IN();
	*lock = (struct _lpt_fastlock) MA_LPT_FASTLOCK_UNLOCKED;
	MARCEL_LOG_RETURN(0);
}

__tbx_inline__ static int __marcel_register_spinlocked(struct _marcel_fastlock *lock,
						       marcel_t self, blockcell * c)
{
	blockcell *first;

	MARCEL_LOG_IN();
	c->next = NULL;
	c->blocked = tbx_true;
	c->task = (self) ? : marcel_self();
	first = MA_MARCEL_FASTLOCK_WAIT_LIST(lock);
	if (first == NULL) {
		/* On est le premier à attendre */
		MA_MARCEL_FASTLOCK_SET_WAIT_LIST(lock, c);
		first = c;
	} else {
		first->last->next = c;
	}
	first->last = c;
	MARCEL_LOG("registering %p (cell %p) in lock %p\n", self, c, lock);
	MARCEL_LOG_RETURN(0);
}

#define __pmarcel_register_spinlocked(l,s,c) __marcel_register_spinlocked(l,s,c)
__tbx_inline__ static int __lpt_register_spinlocked(struct _lpt_fastlock *lock,
						    marcel_t self, blockcell * c)
{
	lpt_blockcell_t *first;

	MARCEL_LOG_IN();
	c->next = NULL;
	c->blocked = tbx_true;
	c->task = (self) ? : marcel_self();
	first = MA_LPT_FASTLOCK_WAIT_LIST(lock);
	if (first == NULL) {
		/* On est le premier à attendre */
		MA_LPT_FASTLOCK_SET_WAIT_LIST(lock, c);
		first = c;
	} else {
		first->last->next = c;
	}
	first->last = c;
	MARCEL_LOG("registering %p (cell %p) in lock %p\n", self, c, lock);
	MARCEL_LOG_RETURN(0);
}


__tbx_inline__ static int __marcel_unregister_spinlocked(struct _marcel_fastlock *lock,
							 blockcell * c)
{
	blockcell *first, *prev;

	MARCEL_LOG_IN();
	MARCEL_LOG("unregistering %p (cell %p) in lock %p\n", c->task, c, lock);
	first = NULL;
	first = MA_MARCEL_FASTLOCK_WAIT_LIST(lock);
	if (first == NULL) {
		/* Il n'y a rien */
	} else if (first == c) {
		/* On est en premier */
		MA_MARCEL_FASTLOCK_SET_WAIT_LIST(lock, first->next);
		if (first->next) {
			first->next->last = first->last;
		}
	} else {
		/* On doit être plus loin */
		prev = first;
		while (prev->next != c) {
			prev = prev->next;
			if (!prev) {
				MARCEL_LOG_RETURN(1);
			}
		}
		prev->next = c->next;
		if (first->last == c) {
			first->last = prev;
		}
	}
	MARCEL_LOG_RETURN(0);
}

#define __pmarcel_unregister_spinlocked(l,c) __marcel_unregister_spinlocked(l,c)
__tbx_inline__ static int __lpt_unregister_spinlocked(struct _lpt_fastlock *lock,
						      blockcell * c)
{
	lpt_blockcell_t *first, *prev;

	MARCEL_LOG("unregistering %p (cell %p) in lock %p\n", c->task, c, lock);
	first = MA_LPT_FASTLOCK_WAIT_LIST(lock);
	if (first == NULL) {
		/* Nobody here.  */
	} else if (first == c) {
		/* C is the head of LOCK's wait list.  */
		MA_LPT_FASTLOCK_SET_WAIT_LIST(lock, first->next);
		if (first->next) {
			first->next->last = first->last;
		}
	} else {
		/* C must be somewhere down the list.  */
		prev = first;
		while (prev->next != c) {
			prev = prev->next;
			if (!prev) {
				return 1;
			}
		}
		prev->next = c->next;
		if (first->last == c) {
			first->last = prev;
		}
	}

	return 0;
}

__tbx_inline__ static int __marcel_lock_spinlocked(struct _marcel_fastlock *lock,
						   marcel_t self)
{
	int ret = 0;

	MARCEL_LOG_IN();
	if (MA_MARCEL_FASTLOCK_BUSY(lock)) {
		blockcell c;

		ret = __marcel_register_spinlocked(lock, self, &c);

		MARCEL_LOG("blocking %p (cell %p) in lock %p\n", self, &c, lock);
		INTERRUPTIBLE_SLEEP_ON_CONDITION_RELEASING(c.blocked,
							   ma_fastlock_release(lock),
							   ma_fastlock_acquire(lock));
		ma_fastlock_release(lock);
		MARCEL_LOG("unblocking %p (cell %p) in lock %p\n", self, &c, lock);

	} else {		/* was free */
		MA_MARCEL_FASTLOCK_SET_STATUS(lock, 1);
		ma_fastlock_release(lock);
	}
	MARCEL_LOG("getting lock %p in task %p\n", lock, self);
	MARCEL_LOG_RETURN(ret);
}

__tbx_inline__ static int __lpt_lock_spinlocked(struct _lpt_fastlock *lock, marcel_t self)
{
	int ret = 0;

	MARCEL_LOG_IN();
	if (MA_LPT_FASTLOCK_TAKEN(lock)) {
		lpt_blockcell_t c;

		ret = __lpt_register_spinlocked(lock, self, &c);

		MARCEL_LOG("blocking %p (cell %p) in lock %p\n", self, &c, lock);
		INTERRUPTIBLE_SLEEP_ON_CONDITION_RELEASING(c.blocked,
							   lpt_fastlock_release(lock),
							   lpt_fastlock_acquire(lock));
		lpt_fastlock_release(lock);
		MARCEL_LOG("unblocking %p (cell %p) in lock %p\n", self, &c, lock);
	} else {		/* was free */
		MA_LPT_FASTLOCK_SET_STATUS(lock, 1);
		lpt_fastlock_release(lock);
	}
	MARCEL_LOG("getting lock %p in task %p\n", lock, self);
	MARCEL_LOG_RETURN(ret);
}

__tbx_inline__ static int __marcel_unlock_spinlocked(struct _marcel_fastlock *lock)
{
	int ret;
	blockcell *first;

	MARCEL_LOG_IN();
	first = MA_MARCEL_FASTLOCK_WAIT_LIST(lock);
	if (first != 0) {	/* waiting threads */
		MA_MARCEL_FASTLOCK_SET_WAIT_LIST(lock, first->next);
		if (first->next) {
			first->next->last = first->last;
		}
		MARCEL_LOG("releasing lock %p in task %p to %p\n", lock, marcel_self(),
			   first->task);
		first->blocked = tbx_false;
		ma_smp_wmb();
		ma_wake_up_thread(first->task);
		ret = 1;
	} else {
		MARCEL_LOG("releasing lock %p in task %p\n", lock, marcel_self());
		MA_MARCEL_FASTLOCK_SET_STATUS(lock, 0);	/* free */
		ret = 0;
	}

	MARCEL_LOG_RETURN(ret);
}

#define __pmarcel_unlock_spinlocked(lock) __marcel_unlock_spinlocked(lock)
__tbx_inline__ static int __lpt_unlock_spinlocked(struct _lpt_fastlock *lock)
{
	int ret;
	lpt_blockcell_t *first;

	first = MA_LPT_FASTLOCK_WAIT_LIST(lock);
	if (first != 0) {	/* waiting threads */
		MA_LPT_FASTLOCK_SET_WAIT_LIST(lock, first->next);
		if (first->next) {
			first->next->last = first->last;
		}
		MARCEL_LOG("releasing lock %p in task %p to %p\n", lock, marcel_self(),
			   first->task);
		first->blocked = tbx_false;
		ma_smp_wmb();
		ma_wake_up_thread(first->task);
		ret = 1;
	} else {
		MARCEL_LOG("releasing lock %p in task %p\n", lock, marcel_self());
		MA_LPT_FASTLOCK_SET_STATUS(lock, 0);	/* free */
		ret = 0;
	}

	return ret;
}

__tbx_inline__ static int __marcel_lock(struct _marcel_fastlock *lock, marcel_t self)
{
	int ret;

	MARCEL_LOG_IN();
	ma_fastlock_acquire(lock);
	ret = __marcel_lock_spinlocked(lock, self);
	MARCEL_LOG_RETURN(ret);
}

__tbx_inline__ static int __pmarcel_lock(struct _marcel_fastlock *lock, marcel_t self)
{
	return __marcel_lock(lock, self);
}

__tbx_inline__ static int __lpt_lock(struct _lpt_fastlock *lock, marcel_t self)
{
	int ret;

	MARCEL_LOG_IN();
	lpt_fastlock_acquire(lock);
	ret = __lpt_lock_spinlocked(lock, self);
	MARCEL_LOG_RETURN(ret);
}

__tbx_inline__ static int __marcel_trylock(struct _marcel_fastlock *lock)
{
	MARCEL_LOG_IN();
	ma_smp_mb();
	if (MA_MARCEL_FASTLOCK_BUSY(lock))
		return 0;

	ma_fastlock_acquire(lock);

	if (!MA_MARCEL_FASTLOCK_BUSY(lock)) {
		MA_MARCEL_FASTLOCK_SET_STATUS(lock, 1);
		ma_fastlock_release(lock);
		MARCEL_LOG_RETURN(1);
	} else {
		ma_fastlock_release(lock);
		MARCEL_LOG_RETURN(0);
	}
}

__tbx_inline__ static int __pmarcel_trylock(struct _marcel_fastlock *lock)
{
	return __marcel_trylock(lock);
}

__tbx_inline__ static int __lpt_trylock(struct _lpt_fastlock *lock)
{
	int taken;
	ma_smp_mb();
	if (MA_LPT_FASTLOCK_TAKEN(lock))
		return 0;

	lpt_fastlock_acquire(lock);

	if (!MA_LPT_FASTLOCK_TAKEN(lock)) {
		/* LOCK was free, take it.  */
		MA_LPT_FASTLOCK_SET_STATUS(lock, 1);
		taken = 1;
	} else {
		taken = 0;
	}

	lpt_fastlock_release(lock);

	return taken;
}

__tbx_inline__ static int __marcel_unlock(struct _marcel_fastlock *lock)
{
	int ret;

	MARCEL_LOG_IN();
	ma_fastlock_acquire(lock);
	ret = __marcel_unlock_spinlocked(lock);
	ma_fastlock_release(lock);
	MARCEL_LOG_RETURN(ret);
}

__tbx_inline__ static int __pmarcel_unlock(struct _marcel_fastlock *lock)
{
	return __marcel_unlock(lock);
}

__tbx_inline__ static int __lpt_unlock(struct _lpt_fastlock *lock)
{
	int ret;

	MARCEL_LOG_IN();
	lpt_fastlock_acquire(lock);
	ret = __lpt_unlock_spinlocked(lock);
	lpt_fastlock_release(lock);
	MARCEL_LOG_RETURN(ret);
}


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/
#endif /** __INLINEFUNCTIONS_MARCEL_FASTLOCK_H__ **/
