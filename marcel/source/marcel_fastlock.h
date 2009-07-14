
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

__tbx_inline__ static int lpt_lock_acquire(long int *lock)
{
	ma_bit_spin_lock(0, (unsigned long int*)lock);
	return 0;
}

__tbx_inline__ static int lpt_lock_release(long int *lock)
{
	ma_bit_spin_unlock(0, (unsigned long int*)lock);
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
  struct blockcell_struct *last; /* Valide uniquement dans la cellule de tête */
  volatile tbx_bool_t blocked;
};

/* `lpt_blockcell_t' objects must be 2-byte aligned.  See `struct
   _lpt_fastlock' for details.  */
typedef struct blockcell_struct blockcell
  __attribute__ ((__aligned__ ((2))));

/* `lpt_blockcell_t' objects must be 4-byte aligned.  See `struct
   _lpt_fastlock' for details.  */
typedef struct blockcell_struct lpt_blockcell_t
  __attribute__ ((__aligned__ ((4))));



__tbx_inline__ static int __marcel_init_lock(struct _marcel_fastlock * lock)
{
  //LOG_IN();
  *lock = (struct _marcel_fastlock) MA_MARCEL_FASTLOCK_UNLOCKED;
  //LOG_OUT();
  return 0;
}
#define __pmarcel_init_lock(lock) __marcel_init_lock(lock)
__tbx_inline__ static int __lpt_init_lock(struct _lpt_fastlock * lock)
{
  //LOG_IN();
  *lock = (struct _lpt_fastlock) MA_LPT_FASTLOCK_UNLOCKED;
  //LOG_OUT();
  return 0;
}

__tbx_inline__ static int __marcel_register_spinlocked(struct _marcel_fastlock * lock,
					       marcel_t self, blockcell*c)
{
  blockcell *first;

  //LOG_IN();
  c->next = NULL;
  c->blocked = tbx_true;
  c->task = (self)?:marcel_self();
  first=MA_MARCEL_FASTLOCK_WAIT_LIST(lock);
  if(first == NULL) {
    /* On est le premier à attendre */
    MA_MARCEL_FASTLOCK_SET_WAIT_LIST(lock, c);
    first=c;
  } else {
    first->last->next=c;
  }
  first->last=c;
  mdebug("registering %p (cell %p) in lock %p\n", self, c, lock);
  //LOG_OUT();
  return 0;
}
#define __pmarcel_register_spinlocked(l,s,c) __marcel_register_spinlocked(l,s,c)
__tbx_inline__ static int __lpt_register_spinlocked(struct _lpt_fastlock * lock,
					       marcel_t self, blockcell*c)
{
  lpt_blockcell_t *first;

  //LOG_IN();
  c->next = NULL;
  c->blocked = tbx_true;
  c->task = (self)?:marcel_self();
  first = MA_LPT_FASTLOCK_WAIT_LIST(lock);
  if(first == NULL) {
    /* On est le premier à attendre */
    MA_LPT_FASTLOCK_SET_WAIT_LIST(lock, c);
    first=c;
  } else {
    first->last->next=c;
  }
  first->last=c;
  mdebug("registering %p (cell %p) in lock %p\n", self, c, lock);
  //LOG_OUT();
  return 0;
}


__tbx_inline__ static int __marcel_unregister_spinlocked(struct _marcel_fastlock * lock,
						 blockcell *c)
{
  blockcell *first, *prev;

  //LOG_IN();
  mdebug("unregistering %p (cell %p) in lock %p\n", c->task, c, lock);
  first=NULL;
  first=MA_MARCEL_FASTLOCK_WAIT_LIST(lock);
  if (first == NULL) {
    /* Il n'y a rien */
  } else if (first==c) {
    /* On est en premier */
    MA_MARCEL_FASTLOCK_SET_WAIT_LIST(lock, first->next);
    if (first->next) {
      first->next->last=first->last;
    }
  } else {
    /* On doit être plus loin */
    prev=first;
    while (prev->next != c) {
      prev = prev->next;
      if (! prev) {
	//LOG_OUT();
	return 1;
      }
    }
    prev->next=c->next;
    if (first->last == c) {
      first->last=prev;
    }
  }
  //LOG_OUT();
  return 0;
}
#define __pmarcel_unregister_spinlocked(l,c) __marcel_unregister_spinlocked(l,c)
__tbx_inline__ static int __lpt_unregister_spinlocked(struct _lpt_fastlock * lock,
						 blockcell *c)
{
  lpt_blockcell_t *first, *prev;

  mdebug("unregistering %p (cell %p) in lock %p\n", c->task, c, lock);
  first = MA_LPT_FASTLOCK_WAIT_LIST(lock);
  if (first == NULL) {
    /* Nobody here.  */
  } else if (first==c) {
    /* C is the head of LOCK's wait list.  */
    MA_LPT_FASTLOCK_SET_WAIT_LIST(lock, first->next);
    if (first->next) {
      first->next->last=first->last;
    }
  } else {
    /* C must be somewhere down the list.  */
    prev=first;
    while (prev->next != c) {
      prev = prev->next;
      if (! prev) {
	return 1;
      }
    }
    prev->next=c->next;
    if (first->last == c) {
      first->last=prev;
    }
  }

  return 0;
}

__tbx_inline__ static int __marcel_lock_spinlocked(struct _marcel_fastlock * lock,
					   marcel_t self)
{
	int ret=0;

	//LOG_IN();
	if(MA_MARCEL_FASTLOCK_BUSY(lock)) {
		blockcell c;

		ret=__marcel_register_spinlocked(lock, self, &c);

		mdebug("blocking %p (cell %p) in lock %p\n", self, &c, lock);
		INTERRUPTIBLE_SLEEP_ON_CONDITION_RELEASING(
			c.blocked, 
			ma_fastlock_release(lock),
			ma_fastlock_acquire(lock));
		ma_fastlock_release(lock);
		mdebug("unblocking %p (cell %p) in lock %p\n", self, &c, lock);
		
	} else { /* was free */
		MA_MARCEL_FASTLOCK_SET_STATUS(lock, 1);
		ma_fastlock_release(lock);
	}
	mdebug("getting lock %p in task %p\n", lock, self);
	//LOG_OUT();
	return ret;
}
__tbx_inline__ static int __lpt_lock_spinlocked(struct _lpt_fastlock * lock,
					   marcel_t self)
{
	int ret=0;

	//LOG_IN();
	if (MA_LPT_FASTLOCK_TAKEN(lock)) {
		lpt_blockcell_t c;

		ret=__lpt_register_spinlocked(lock, self, &c);

		mdebug("blocking %p (cell %p) in lock %p\n", self, &c, lock);
		INTERRUPTIBLE_SLEEP_ON_CONDITION_RELEASING(
			c.blocked, 
			lpt_fastlock_release(lock),
			lpt_fastlock_acquire(lock));
		lpt_fastlock_release(lock);
		mdebug("unblocking %p (cell %p) in lock %p\n", self, &c, lock);
	} else { /* was free */
		MA_LPT_FASTLOCK_SET_STATUS(lock, 1);
		lpt_fastlock_release(lock);
	}
	mdebug("getting lock %p in task %p\n", lock, self);
	//LOG_OUT();
	return ret;
}

__tbx_inline__ static int __marcel_unlock_spinlocked(struct _marcel_fastlock * lock)
{
  int ret;
  blockcell *first;

  //LOG_IN();
  first=MA_MARCEL_FASTLOCK_WAIT_LIST(lock);
  if(first != 0) { /* waiting threads */
    MA_MARCEL_FASTLOCK_SET_WAIT_LIST(lock, first->next);
    if (first->next) {
      first->next->last=first->last;
    }
    mdebug("releasing lock %p in task %p to %p\n", lock, marcel_self(),
	   first->task);
    first->blocked=0;
    ma_smp_wmb();
    ma_wake_up_thread(first->task);
    ret=1;
  } else {
    mdebug("releasing lock %p in task %p\n", lock, marcel_self());
    MA_MARCEL_FASTLOCK_SET_STATUS(lock, 0); /* free */
    ret=0;
  }

  //LOG_OUT();
  return ret;
}
#define __pmarcel_unlock_spinlocked(lock) __marcel_unlock_spinlocked(lock)
__tbx_inline__ static int __lpt_unlock_spinlocked(struct _lpt_fastlock * lock)
{
  int ret;
  lpt_blockcell_t *first;

  first = MA_LPT_FASTLOCK_WAIT_LIST(lock);
  if(first != 0) { /* waiting threads */
    MA_LPT_FASTLOCK_SET_WAIT_LIST(lock, first->next);
    if (first->next) {
      first->next->last=first->last;
    }
    mdebug("releasing lock %p in task %p to %p\n", lock, marcel_self(),
	   first->task);
    first->blocked=0;
    ma_smp_wmb();
    ma_wake_up_thread(first->task);
    ret=1;
  } else {
    mdebug("releasing lock %p in task %p\n", lock, marcel_self());
    MA_LPT_FASTLOCK_SET_STATUS(lock, 0); /* free */
    ret=0;
  }

  return ret;
}

__tbx_inline__ static int __marcel_lock(struct _marcel_fastlock * lock,
				marcel_t self)
{
  int ret;

  //LOG_IN();
  ma_fastlock_acquire(lock);
  ret=__marcel_lock_spinlocked(lock, self);
  //LOG_OUT();
  return ret;
}
__tbx_inline__ static int __pmarcel_lock(struct _marcel_fastlock * lock,
				 marcel_t self)
{
  return __marcel_lock(lock, self);
}

__tbx_inline__ static int __lpt_lock(struct _lpt_fastlock * lock,
				marcel_t self)
{
  int ret;

  //LOG_IN();
  lpt_fastlock_acquire(lock);
  ret=__lpt_lock_spinlocked(lock, self);
  //LOG_OUT();
  return ret;
}

__tbx_inline__ static int __marcel_trylock(struct _marcel_fastlock * lock)
{
  //LOG_IN();
  ma_smp_mb();
  if(MA_MARCEL_FASTLOCK_BUSY(lock))
    return 0;

  ma_fastlock_acquire(lock);

  if(!MA_MARCEL_FASTLOCK_BUSY(lock)) {
    MA_MARCEL_FASTLOCK_SET_STATUS(lock, 1);
    ma_fastlock_release(lock);
    //LOG_OUT();
    return 1;
  } else {
    ma_fastlock_release(lock);
    //LOG_OUT();
    return 0;
  }
}
__tbx_inline__ static int __pmarcel_trylock(struct _marcel_fastlock * lock)
{
  return __marcel_trylock(lock);
}

__tbx_inline__ static int __lpt_trylock(struct _lpt_fastlock * lock)
{
  int taken;
  ma_smp_mb();
  if(MA_LPT_FASTLOCK_TAKEN(lock))
    return 0;

  lpt_fastlock_acquire(lock);

  if(!MA_LPT_FASTLOCK_TAKEN(lock)) {
    /* LOCK was free, take it.  */
    MA_LPT_FASTLOCK_SET_STATUS(lock, 1);
    taken = 1;
  } else {
    taken = 0;
  }

  lpt_fastlock_release(lock);

  return taken;
}

__tbx_inline__ static int __marcel_unlock(struct _marcel_fastlock * lock)
{
  int ret;

  //LOG_IN();
  ma_fastlock_acquire(lock);
  ret=__marcel_unlock_spinlocked(lock);
  ma_fastlock_release(lock);
  //LOG_OUT();
  return ret;
}
__tbx_inline__ static int __pmarcel_unlock(struct _marcel_fastlock * lock)
{
  return __marcel_unlock(lock);
}

__tbx_inline__ static int __lpt_unlock(struct _lpt_fastlock * lock)
{
  int ret;

  //LOG_IN();
  lpt_fastlock_acquire(lock);
  ret=__lpt_unlock_spinlocked(lock);
  lpt_fastlock_release(lock);
  //LOG_OUT();
  return ret;
}


/* Low-level sort-of condition variables à la `lll_futex_{wait,wake}()'.  */

/** \brief Wait on \param lock until a signal() or broadcast() call is
 * made.  */
extern void __lpt_lock_wait(struct _lpt_fastlock * lock);

/** \brief If \param value and \param expected_value are equal, wait on
 * \param lock until a signal() or broadcast() call is made or \param abstime
 * is reached.  In the latter case, return \e ETIMEDOUT; otherwise return \e
 * EWOULDBLOCK.  This is similar to the \e futex(2) \e FUTEX_WAIT
 * behavior.  */
extern int __lpt_lock_timed_wait(struct _lpt_fastlock *lock, struct timespec *abstime, long *value, long expected_value);

/** \brief Wake up every thread waiting on \param lock.  */
extern void __lpt_lock_broadcast(struct _lpt_fastlock * lock);

/** \brief Wake up one of the threads waiting on \param lock.  */
extern void __lpt_lock_signal(struct _lpt_fastlock * lock);
