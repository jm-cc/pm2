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


#ifndef __INLINEFUNCTIONS_SYS_MARCEL_FASTLOCK_H__
#define __INLINEFUNCTIONS_SYS_MARCEL_FASTLOCK_H__


#include "scheduler/inlinefunctions/linux_sched.h"
#include "scheduler/inlinefunctions/marcel_holder.h"
#include "sys/marcel_sched_generic.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/**
 * Low level lock functions
 *
 **/
static __tbx_inline__ void __marcel_init_lock(struct _marcel_fastlock *lock)
{
	struct _marcel_fastlock initlock = MA_MARCEL_FASTLOCK_UNLOCKED; 
	*lock = initlock;
}

static __tbx_inline__ void __marcel_destroy_lock(struct _marcel_fastlock *lock TBX_UNUSED)
{
}

static __tbx_inline__ void __lpt_init_lock(struct _lpt_fastlock *lock)
{
	struct _marcel_fastlock *l;
	static ma_spinlock_t s = MA_SPIN_LOCK_UNLOCKED;

	ma_spin_lock(&s);
	if (tbx_unlikely(NULL == LPT_FASTLOCK_2_MA_FASTLOCK(lock))) {
		l = marcel_malloc(sizeof(*l));
		__marcel_init_lock(l);
		LPT_FASTLOCK_2_MA_FASTLOCK(lock) = l;
	}
	ma_spin_unlock(&s);
}

static __tbx_inline__ void __lpt_destroy_lock(struct _lpt_fastlock *lock)
{
	marcel_free(LPT_FASTLOCK_2_MA_FASTLOCK(lock));
	LPT_FASTLOCK_2_MA_FASTLOCK(lock) = NULL;
}

static __tbx_inline__ void ma_fastlock_acquire(struct _marcel_fastlock *fastlock)
{
	ma_spin_lock(&fastlock->__spinlock);
}

static __tbx_inline__ void ma_fastlock_release(struct _marcel_fastlock *fastlock)
{
	ma_spin_unlock(&fastlock->__spinlock);
}

static __tbx_inline__ void lpt_fastlock_acquire(struct _lpt_fastlock *fastlock)
{
	if (tbx_unlikely(NULL == LPT_FASTLOCK_2_MA_FASTLOCK(fastlock)))
		__lpt_init_lock(fastlock);
	ma_fastlock_acquire(LPT_FASTLOCK_2_MA_FASTLOCK(fastlock));
}

static __tbx_inline__ void lpt_fastlock_release(struct _lpt_fastlock *fastlock)
{
	ma_fastlock_release(LPT_FASTLOCK_2_MA_FASTLOCK(fastlock));
}


/**
 * Lock wait queue management functions
 *
 **/
typedef struct __blockcell {
	marcel_t task;
	tbx_bool_t blocked;
	struct __blockcell *next;
	struct __blockcell *prev;
} blockcell;


__attribute__((nonnull(1, 2, 3)))
static __tbx_inline__ 
void __marcel_register_lock_spinlocked(struct _marcel_fastlock *lock, marcel_t self, blockcell * c)
{
	blockcell *first; 

	MARCEL_LOG_IN();

	c->task = self;
	c->blocked = tbx_true;
	first = MA_MARCEL_FASTLOCK_WAIT_LIST(lock);

#ifdef MARCEL_CHECK_PRIO_ON_LOCKS
	blockcell *curr;

	if (first == NULL) {
		c->prev = c;
		c->next = NULL;
		MA_MARCEL_FASTLOCK_SET_WAIT_LIST(lock, c);
		MARCEL_LOG_OUT();
		return;
	}

	if (first->task->as_entity.prio > c->task->as_entity.prio) {
		c->prev = first->prev;
		c->next = first;
		first->prev = c;
		MA_MARCEL_FASTLOCK_SET_WAIT_LIST(lock, c);
		MARCEL_LOG_OUT();
		return;
	}
		
	curr = first->prev;
	while (curr->task->as_entity.prio > c->task->as_entity.prio)
		curr = curr->prev;

	c->prev = curr;
	c->next = curr->next;
	curr->next = c;
	if (c->next)
		c->next->prev = c;
	else
		first->prev = c;
#else
	c->next = NULL;
	if (first) {
		first->prev->next = c;
		c->prev = first->prev;
		first->prev = c;
	} else {
		c->prev = c;
		MA_MARCEL_FASTLOCK_SET_WAIT_LIST(lock, c);
	}
#endif /* MARCEL_CHECK_PRIO_ON_LOCKS */

	MARCEL_LOG("registering %p (cell %p) in lock %p\n", self, c, lock);
	MARCEL_LOG_OUT();
}

__attribute__((nonnull(1, 2, 3)))
static __tbx_inline__ 
void __lpt_register_lock_spinlocked(struct _lpt_fastlock *lock, marcel_t self, blockcell *c)
{
	__marcel_register_lock_spinlocked(LPT_FASTLOCK_2_MA_FASTLOCK(lock), self, c);
}

static __tbx_inline__ 
void __marcel_unregister_lock_spinlocked(struct _marcel_fastlock *lock, blockcell * c)
{
	blockcell *first;

	MARCEL_LOG_IN();
	MARCEL_LOG("unregistering %p (cell %p) in lock %p\n", c->task, c, lock);

	first = MA_MARCEL_FASTLOCK_WAIT_LIST(lock);

	if (tbx_likely(first == c)) {
		/* On est en premier */
		MA_MARCEL_FASTLOCK_SET_WAIT_LIST(lock, first->next);
		if (first->next)
			first->next->prev = first->prev;
	} else {
		c->prev->next = c->next;
		if (c->next)
			c->next->prev = c->prev;
		else
			first->prev = c->prev;
	}
}

static __tbx_inline__ 
void __lpt_unregister_lock_spinlocked(struct _lpt_fastlock *lock, blockcell * c)
{
	__marcel_unregister_lock_spinlocked(LPT_FASTLOCK_2_MA_FASTLOCK(lock), c);
}



/**
 * Lock operation functions 
 *
 **/
static __tbx_inline__ 
void __marcel_lock_wait(struct _marcel_fastlock *lock, marcel_t self, unsigned int flags)
{
	/* thread could be cancelled during SLEEP_ON_CONDITION loop:
	 * ex: ma_schedule -> deviate_work -> *_spin_unlock() */
	blockcell *c = ma_obj_alloc(marcel_lockcell_allocator, NULL);

	__marcel_register_lock_spinlocked(lock, self, c);
	ma_fastlock_release(lock);

	MARCEL_LOG("blocking %p (cell %p) in lock %p\n", self, c, lock);
	INTERRUPTIBLE_SLEEP_ON_CONDITION_RELEASING(
		ma_access_once(c->blocked) == tbx_true,
		(((flags & MA_CHECK_INTR) && MA_GET_INTERRUPTED())
#if defined(MARCEL_DEVIATION_ENABLED) && defined(MA__IFACE_PMARCEL)
		 || ((flags & MA_CHECK_CANCEL) && self->canceled == MARCEL_IS_CANCELED)
#endif
		));

	ma_fastlock_acquire(lock);
	if (tbx_unlikely(c->blocked))
		__marcel_unregister_lock_spinlocked(lock, c);

	/* list (waiting threads) was updated before by 
	 * __marcel_unlock (__marcel_unlock_spinlocked) call
	 * c can be detroyed now */
	ma_obj_free(marcel_lockcell_allocator, c, NULL);
}

static __tbx_inline__ 
void __lpt_lock_wait(struct _lpt_fastlock *lock, marcel_t self, unsigned int flags)
{
	__marcel_lock_wait(LPT_FASTLOCK_2_MA_FASTLOCK(lock), self, flags);
}

static __tbx_inline__ 
int __marcel_lock_timed_wait(struct _marcel_fastlock *lock, marcel_t self, 
			     unsigned long timeout, unsigned int flags)
{
	int ret;

	/* thread could be cancelled during SLEEP_ON_CONDITION loop:
	 * ex: ma_schedule -> deviate_work -> *_spin_unlock() */
	blockcell *c = ma_obj_alloc(marcel_lockcell_allocator, NULL);

	__marcel_register_lock_spinlocked(lock, self, c);
	ma_fastlock_release(lock);

	/* Loop until we're unblocked or time is up.  */
	MARCEL_LOG("blocking %p (cell %p) in lock %p\n", self, c, lock);

	timeout = ma_jiffies_from_us(timeout);
	INTERRUPTIBLE_TIMED_SLEEP_ON_CONDITION_RELEASING(
		ma_access_once(c->blocked) == tbx_true,
		(((flags & MA_CHECK_INTR) && MA_GET_INTERRUPTED())
#if defined(MARCEL_DEVIATION_ENABLED) && defined(MA__IFACE_PMARCEL)
		 || ((flags & MA_CHECK_CANCEL) && self->canceled == MARCEL_IS_CANCELED)
#endif
		),
		timeout);

	ma_fastlock_acquire(lock);
	if (tbx_unlikely(c->blocked)) {
		ret = (timeout) ? EINTR : ETIMEDOUT;
		__marcel_unregister_lock_spinlocked(lock, c);
	} else
		ret = 0;

	/* list (waiting threads) was updated before by 
	 * __marcel_unlock (__marcel_unlock_spinlocked) call
	 * c can be detroyed now */
	ma_obj_free(marcel_lockcell_allocator, c, NULL);

	return ret;
}

static __tbx_inline__ 
int __lpt_lock_timed_wait(struct _lpt_fastlock *lock, marcel_t self, 
			  unsigned timeout, unsigned int flags)
{
	return __marcel_lock_timed_wait(LPT_FASTLOCK_2_MA_FASTLOCK(lock), self, timeout, flags);
}

static __tbx_inline__ void __marcel_lock_signal(struct _marcel_fastlock *lock)
{
	blockcell *first;

	first = MA_MARCEL_FASTLOCK_WAIT_LIST(lock);
	if (tbx_likely(first)) {
		first->blocked = tbx_false;
		__marcel_unregister_lock_spinlocked(lock, first);
		ma_wake_up_thread(first->task);
	}
}

static __tbx_inline__ void __lpt_lock_signal(struct _lpt_fastlock *lock)
{
	__marcel_lock_signal(LPT_FASTLOCK_2_MA_FASTLOCK(lock));
}

/** signal at least n task */
static __tbx_inline__ 
int __marcel_lock_broadcast(struct _marcel_fastlock *lock, int n)
{
	blockcell *c;
	int wokenup;

	wokenup = 0;
	c = MA_MARCEL_FASTLOCK_WAIT_LIST(lock);
	while (c && n) {
		c->blocked = tbx_false;
		__marcel_unregister_lock_spinlocked(lock, c);
		wokenup += ma_wake_up_thread(c->task);
		c = c->next;
		n--;
	}

	return wokenup;
}

static __tbx_inline__ void __lpt_lock_broadcast(struct _lpt_fastlock *lock, int n)
{
	__marcel_lock_broadcast(LPT_FASTLOCK_2_MA_FASTLOCK(lock), n);
}


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __INLINEFUNCTIONS_SYS_MARCEL_FASTLOCK_H__ **/
