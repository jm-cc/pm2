
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


typedef struct blockcell_struct {
  marcel_t task;
  struct blockcell_struct *next;
  struct blockcell_struct *last; /* Valide uniquement dans la cellule de tête */
  boolean blocked;
} blockcell;

__tbx_inline__ static int __marcel_init_lock(struct _marcel_fastlock * lock)
{
  //LOG_IN();
  *lock = (struct _marcel_fastlock) MA_FASTLOCK_UNLOCKED;
  ////LOG_OUT();
  return 0;
}
#define __pmarcel_init_lock(lock) __marcel_init_lock(lock)

__tbx_inline__ static int __marcel_register_spinlocked(struct _marcel_fastlock * lock,
					       marcel_t self, blockcell*c)
{
  blockcell *first;

  //LOG_IN();
  c->next = NULL;
  c->blocked = TRUE;
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
			marcel_lock_release(&lock->__spinlock); unlock_task(),
			lock_task(); marcel_lock_acquire(&lock->__spinlock));
		marcel_lock_release(&lock->__spinlock);
		unlock_task();
		mdebug("unblocking %p (cell %p) in lock %p\n", self, &c, lock);
		
	} else { /* was free */
		lock->__status = 1;
		marcel_lock_release(&lock->__spinlock);
		unlock_task();
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
    ma_wmb();
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
  marcel_lock_acquire(&lock->__spinlock);
  ret=__marcel_lock_spinlocked(lock, self);
  //LOG_OUT();
  return ret;
}
__tbx_inline__ static int __pmarcel_lock(struct _marcel_fastlock * lock,
				 marcel_t self)
{
  lock_task();
  return __marcel_lock(lock, self);
}
#define __pmarcel_alt_lock(lock, self) __pmarcel_lock(lock, self)

__tbx_inline__ static int __marcel_trylock(struct _marcel_fastlock * lock)
{
  //LOG_IN();

  marcel_lock_acquire(&lock->__spinlock);

  if(lock->__status == 0) { /* free */
    lock->__status = 1;
    marcel_lock_release(&lock->__spinlock);
    unlock_task();
    //LOG_OUT();
    return 1;
  } else {
    marcel_lock_release(&lock->__spinlock);
    unlock_task();
    //LOG_OUT();
    return 0;
  }
}
__tbx_inline__ static int __pmarcel_trylock(struct _marcel_fastlock * lock)
{
  lock_task();
  return __marcel_trylock(lock);
}
#define __pmarcel_alt_trylock(lock) __pmarcel_trylock(lock)

__tbx_inline__ static int __marcel_unlock(struct _marcel_fastlock * lock)
{
  int ret;

  //LOG_IN();
  marcel_lock_acquire(&lock->__spinlock);
  ret=__marcel_unlock_spinlocked(lock);
  marcel_lock_release(&lock->__spinlock);
  unlock_task();
  //LOG_OUT();
  return ret;
}
__tbx_inline__ static int __pmarcel_unlock(struct _marcel_fastlock * lock)
{
  lock_task();
  return __marcel_unlock(lock);
}
#define __pmarcel_alt_unlock(lock) __pmarcel_unlock(lock)

