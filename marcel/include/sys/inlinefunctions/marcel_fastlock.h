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

static __tbx_inline__ int ma_fastlock_is_taken(struct _marcel_fastlock *fastlock)
{
	return ma_spin_is_locked(&fastlock->__spinlock);
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
	struct __blockcell *next;
	struct __blockcell *prev;
	tbx_bool_t blocked;
} blockcell;


__attribute__((nonnull(1, 2, 3)))
static __tbx_inline__ 
void __marcel_register_lock_spinlocked(struct _marcel_fastlock *lock, marcel_t self, blockcell * c)
{
	blockcell *first; 

	MARCEL_LOG_IN();

	c->blocked = tbx_true;
	c->task = self;
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

static __tbx_inline__ void
__marcel_unregister_lock_spinlocked(struct _marcel_fastlock *lock, blockcell * c)
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
	MARCEL_LOG_OUT();
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

	MARCEL_LOG("blocking %p (cell %p) in lock %p\n", self, c, lock);

	INTERRUPTIBLE_SLEEP_ON_CONDITION_RELEASING(
		ma_access_once(c->blocked) == tbx_true,
		(((flags & MA_CHECK_INTR) && MA_GET_INTERRUPTED())
#if defined(MARCEL_DEVIATION_ENABLED) && defined(MA__IFACE_PMARCEL)
		 || ((flags & MA_CHECK_CANCEL) && self->canceled == MARCEL_IS_CANCELED)
#endif
		),
		ma_fastlock_release(lock),
		ma_fastlock_acquire(lock));

	if (tbx_unlikely(tbx_true == ma_access_once(c->blocked)))
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
		ma_fastlock_release(lock),
		ma_fastlock_acquire(lock),
		timeout);

	if (tbx_unlikely(ma_access_once(c->blocked) == tbx_true)) {
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

static __tbx_inline__ void __marcel_lock(struct _marcel_fastlock *lock, marcel_t self)
{
	MARCEL_LOG_IN();

	if (tbx_unlikely(ma_atomic_xchg(0, 1, &lock->__status))) { // try to take the lock
		ma_fastlock_acquire(lock);
		if (tbx_unlikely(ma_atomic_xchg(0, 1, &lock->__status)))
			__marcel_lock_wait(lock, self, 0);
		ma_fastlock_release(lock);
	}

	MARCEL_LOG("getting lock %p in task %p\n", lock, self);
	MARCEL_LOG_OUT();
}

static __tbx_inline__ void __lpt_lock(struct _lpt_fastlock *lock, marcel_t self)
{
	if (tbx_unlikely(NULL == LPT_FASTLOCK_2_MA_FASTLOCK(lock)))
		__lpt_init_lock(lock);
	__marcel_lock(LPT_FASTLOCK_2_MA_FASTLOCK(lock), self);
}


static __tbx_inline__ int __marcel_lock_signal(struct _marcel_fastlock *lock)
{
	blockcell *first;

	first = MA_MARCEL_FASTLOCK_WAIT_LIST(lock);
	if (first == NULL) {
		ma_atomic_set(&lock->__status, 0); /* free */
		ma_smp_wmb();
		return 0;
	}

#ifdef MA__LWPS
	blockcell *curr;
	ma_holder_t *h;
	ma_lwp_t lwp;

	/** try to find a task that can be scheduled now */
	curr = first;
	do {
		if (MA_LWP_SELF == ma_get_task_lwp(curr->task)) {
			first = curr;
			break;
		}

		h = ma_task_holder_lock_softirq_async(curr->task);
		lwp = ma_find_lwp_for_task(curr->task, h);
		if (tbx_likely(lwp)) {
			__marcel_unregister_lock_spinlocked(lock, curr);
			curr->blocked = tbx_false;
			ma_smp_wmb();
			ma_awake_task(curr->task, 
				      MA_TASK_INTERRUPTIBLE|MA_TASK_UNINTERRUPTIBLE, &h);
			ma_holder_try_to_wake_up_and_unlock_softirq(h);
			ma_resched_task(ma_per_lwp(current_thread, lwp), lwp);

			return 1;
		}
		ma_task_holder_unlock_softirq(h);
                        
		curr = curr->next;
		if (! curr)
			break;

#ifdef MARCEL_CHECK_PRIO_ON_LOCKS
		/** check priority constraint */
		if (first->task->as_entity.prio != curr->task->as_entity.prio)
			break;
#endif
	} while (1);
#endif /* MA__LWPS **/

	__marcel_unregister_lock_spinlocked(lock, first);
	first->blocked = tbx_false;
	ma_smp_wmb();
	ma_wake_up_thread(first->task);

	return 1;
}

static __tbx_inline__ int __lpt_lock_signal(struct _lpt_fastlock *lock)
{
	return __marcel_lock_signal(LPT_FASTLOCK_2_MA_FASTLOCK(lock));
}

static __tbx_inline__ void __marcel_lock_broadcast(struct _marcel_fastlock *lock)
{
	blockcell *cell;

	cell = MA_MARCEL_FASTLOCK_WAIT_LIST(lock);
	while (cell) {
		cell->blocked = tbx_false;
		cell = cell->next;
	}
	ma_smp_wmb();

	cell = MA_MARCEL_FASTLOCK_WAIT_LIST(lock);
	while (cell) {
		ma_wake_up_thread(cell->task);
		cell = cell->next;
	}

	ma_atomic_set(&lock->__status, 0);
	MA_MARCEL_FASTLOCK_SET_WAIT_LIST(lock, NULL);
	ma_smp_wmb();
}

static __tbx_inline__ void __lpt_lock_broadcast(struct _lpt_fastlock *lock)
{
	__marcel_lock_broadcast(LPT_FASTLOCK_2_MA_FASTLOCK(lock));
}

static __tbx_inline__ int __marcel_unlock(struct _marcel_fastlock *lock)
{
	int ret;

	MARCEL_LOG_IN();

	ma_fastlock_acquire(lock);
	ret = __marcel_lock_signal(lock);
	ma_fastlock_release(lock);

	MARCEL_LOG_RETURN(ret);
}

static __tbx_inline__ int __lpt_unlock(struct _lpt_fastlock *lock)
{
	if (tbx_unlikely(NULL == LPT_FASTLOCK_2_MA_FASTLOCK(lock)))
		__lpt_init_lock(lock);
	return __marcel_unlock(LPT_FASTLOCK_2_MA_FASTLOCK(lock));
}

static __tbx_inline__ int __marcel_trylock(struct _marcel_fastlock *lock)
{
	MARCEL_LOG_IN();
	if (tbx_unlikely(0 == ma_atomic_xchg(0, 1, &lock->__status)))
		MARCEL_LOG_RETURN(1); // lock taken
	MARCEL_LOG_RETURN(0);
}

static __tbx_inline__ int __lpt_trylock(struct _lpt_fastlock *lock)
{
	if (tbx_unlikely(NULL == LPT_FASTLOCK_2_MA_FASTLOCK(lock)))
		__lpt_init_lock(lock);
	return __marcel_trylock(LPT_FASTLOCK_2_MA_FASTLOCK(lock));
}


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __INLINEFUNCTIONS_SYS_MARCEL_FASTLOCK_H__ **/
