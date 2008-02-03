
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
	ma_preempt_disable();
#ifdef MA__LWPS
	ma_bit_spin_lock(1, (unsigned long int*)lock);
#endif
	return 0;
}

__tbx_inline__ static int lpt_lock_release(long int *lock)
{
#ifdef MA__LWPS
	ma_bit_spin_unlock(1, (unsigned long int*)lock);
#endif
	ma_preempt_enable();
	return 0;
}

#define marcel_lock_acquire(lock)		ma_spin_lock(lock)
#define marcel_lock_release(lock)		ma_spin_unlock(lock)
#define pmarcel_lock_acquire(lock)		ma_spin_lock(lock)
#define pmarcel_lock_release(lock)		ma_spin_unlock(lock)

typedef struct blockcell_struct {
  marcel_t task;
  struct blockcell_struct *next;
  struct blockcell_struct *last; /* Valide uniquement dans la cellule de tête */
  volatile tbx_bool_t blocked;
} blockcell;

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
  first=(blockcell*)(lock->__status & ~1);
  if(first == NULL) {
    /* On est le premier à attendre */
    lock->__status = 1 | ((unsigned long)c);
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
  blockcell *first;

  //LOG_IN();
  c->next = NULL;
  c->blocked = tbx_true;
  c->task = (self)?:marcel_self();
  first=(blockcell*)(lock->__status & ~1);
  if(first == NULL) {
    /* On est le premier à attendre */
    lock->__status = 1 | ((unsigned long)c);
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
  first=(blockcell*)(lock->__status & ~1);
  if (first == NULL) {
    /* Il n'y a rien */
  } else if (first==c) {
    /* On est en premier */
    lock->__status= (unsigned long)first->next|1;
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
  blockcell *first, *prev;

  //LOG_IN();
  mdebug("unregistering %p (cell %p) in lock %p\n", c->task, c, lock);
  first=NULL;
  first=(blockcell*)(lock->__status & ~1);
  if (first == NULL) {
    /* Il n'y a rien */
  } else if (first==c) {
    /* On est en premier */
    lock->__status= (unsigned long)first->next|1;
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

__tbx_inline__ static int __marcel_lock_spinlocked(struct _marcel_fastlock * lock,
					   marcel_t self)
{
	int ret=0;

	//LOG_IN();
	if(lock->__status != 0) { /* busy */
		blockcell c;

		ret=__marcel_register_spinlocked(lock, self, &c);

		mdebug("blocking %p (cell %p) in lock %p\n", self, &c, lock);
		INTERRUPTIBLE_SLEEP_ON_CONDITION_RELEASING(
			c.blocked, 
			ma_spin_unlock(&lock->__spinlock),
			ma_spin_lock(&lock->__spinlock));
		ma_spin_unlock(&lock->__spinlock);
		mdebug("unblocking %p (cell %p) in lock %p\n", self, &c, lock);
		
	} else { /* was free */
		lock->__status = 1;
		ma_spin_unlock(&lock->__spinlock);
	}
	mdebug("getting lock %p in lock %p\n", self, lock);
	//LOG_OUT();
	return ret;
}
__tbx_inline__ static int __lpt_lock_spinlocked(struct _lpt_fastlock * lock,
					   marcel_t self)
{
	int ret=0;

	//LOG_IN();
	if(lock->__status != 0) { /* busy */
		blockcell c;

		ret=__lpt_register_spinlocked(lock, self, &c);

		mdebug("blocking %p (cell %p) in lock %p\n", self, &c, lock);
		INTERRUPTIBLE_SLEEP_ON_CONDITION_RELEASING(
			c.blocked, 
			lpt_lock_release(&lock->__spinlock),
			lpt_lock_acquire(&lock->__spinlock));
		lpt_lock_release(&lock->__spinlock);
		mdebug("unblocking %p (cell %p) in lock %p\n", self, &c, lock);
		
	} else { /* was free */
		lock->__status = 1;
		lpt_lock_release(&lock->__spinlock);
	}
	mdebug("getting lock %p in lock %p\n", self, lock);
	//LOG_OUT();
	return ret;
}

__tbx_inline__ static int __marcel_unlock_spinlocked(struct _marcel_fastlock * lock)
{
  int ret;
  blockcell *first;

  //LOG_IN();
  first=(blockcell*)(lock->__status & ~1);
  if(first != 0) { /* waiting threads */
    lock->__status= (unsigned long)first->next|1;
    if (first->next) {
      first->next->last=first->last;
    }
    mdebug("releasing lock %p in lock %p to %p\n", marcel_self(), lock, 
	   first->task);
    first->blocked=0;
    ma_smp_wmb();
    ma_wake_up_thread(first->task);
    ret=1;
  } else {
    mdebug("releasing lock %p in lock %p\n", marcel_self(), lock);
    lock->__status = 0; /* free */
    ret=0;
  }

  //LOG_OUT();
  return ret;
}
#define __pmarcel_unlock_spinlocked(lock) __marcel_unlock_spinlocked(lock)
__tbx_inline__ static int __lpt_unlock_spinlocked(struct _lpt_fastlock * lock)
{
  int ret;
  blockcell *first;

  //LOG_IN();
  first=(blockcell*)(lock->__status & ~1);
  if(first != 0) { /* waiting threads */
    lock->__status= (unsigned long)first->next|1;
    if (first->next) {
      first->next->last=first->last;
    }
    mdebug("releasing lock %p in lock %p to %p\n", marcel_self(), lock, 
	   first->task);
    first->blocked=0;
    ma_smp_wmb();
    ma_wake_up_thread(first->task);
    ret=1;
  } else {
    mdebug("releasing lock %p in lock %p\n", marcel_self(), lock);
    lock->__status = 0; /* free */
    ret=0;
  }

  //LOG_OUT();
  return ret;
}

__tbx_inline__ static int __marcel_lock(struct _marcel_fastlock * lock,
				marcel_t self)
{
  int ret;

  //LOG_IN();
  ma_spin_lock(&lock->__spinlock);
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
  lpt_lock_acquire(&lock->__spinlock);
  ret=__lpt_lock_spinlocked(lock, self);
  //LOG_OUT();
  return ret;
}

__tbx_inline__ static int __marcel_trylock(struct _marcel_fastlock * lock)
{
  //LOG_IN();

  ma_spin_lock(&lock->__spinlock);

  if(lock->__status == 0) { /* free */
    lock->__status = 1;
    ma_spin_unlock(&lock->__spinlock);
    //LOG_OUT();
    return 1;
  } else {
    ma_spin_unlock(&lock->__spinlock);
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
  //LOG_IN();

  lpt_lock_acquire(&lock->__spinlock);

  if(lock->__status == 0) { /* free */
    lock->__status = 1;
    lpt_lock_release(&lock->__spinlock);
    //LOG_OUT();
    return 1;
  } else {
    lpt_lock_release(&lock->__spinlock);
    //LOG_OUT();
    return 0;
  }
}

__tbx_inline__ static int __marcel_unlock(struct _marcel_fastlock * lock)
{
  int ret;

  //LOG_IN();
  ma_spin_lock(&lock->__spinlock);
  ret=__marcel_unlock_spinlocked(lock);
  ma_spin_unlock(&lock->__spinlock);
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
  lpt_lock_acquire(&lock->__spinlock);
  ret=__lpt_unlock_spinlocked(lock);
  lpt_lock_release(&lock->__spinlock);
  //LOG_OUT();
  return ret;
}
