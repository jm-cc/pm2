/* Read-write lock implementation.
   Copyright (C) 1998, 2000 Free Software Foundation, Inc.
   This file comes from the GNU C Library.
   Contributed by Xavier Leroy <Xavier.Leroy@inria.fr>
   and Ulrich Drepper <drepper@cygnus.com>, 1998.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include "marcel.h" //VD:
#include "marcel_for_pthread.h"

#ifdef __USE_UNIX98 // AD:

#ifdef LINUX_SYS // AD:
#include <bits/libc-lock.h>
#endif
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
//VD: #include "internals.h"
//VD: #include "queue.h"
//VD: #include "spinlock.h"
//VD: #include "restart.h"
#include "marcel_rwlock.h" //VD:
#include "marcel_fastlock.h" //VD:
#include "pthread_queue.h" //VD:

#if 0 //VD:
/* Function called by marcel_cancel to remove the thread from
   waiting inside marcel_rwlock_timedrdlock or marcel_rwlock_timedwrlock. */

static int rwlock_rd_extricate_func(void *obj, marcel_descr th)
{
  marcel_rwlock_t *rwlock = obj;
  int did_remove = 0;

  __pmarcel_lock(&rwlock->__rw_lock, NULL);
  did_remove = remove_from_queue(&rwlock->__rw_read_waiting, th);
  __pmarcel_unlock(&rwlock->__rw_lock);

  return did_remove;
}

static int rwlock_wr_extricate_func(void *obj, marcel_descr th)
{
  marcel_rwlock_t *rwlock = obj;
  int did_remove = 0;

  __pmarcel_lock(&rwlock->__rw_lock, NULL);
  did_remove = remove_from_queue(&rwlock->__rw_write_waiting, th);
  __pmarcel_unlock(&rwlock->__rw_lock);

  return did_remove;
}
#endif //VD:

/*
 * Check whether the calling thread already owns one or more read locks on the
 * specified lock. If so, return a pointer to the read lock info structure
 * corresponding to that lock.
 */

static marcel_readlock_info *
rwlock_is_in_list(marcel_descr self, marcel_rwlock_t *rwlock)
{
  marcel_readlock_info *info;

  for (info = THREAD_GETMEM (self, p_readlock_list); info != NULL;
       info = info->pr_next)
    {
      if (info->pr_lock == rwlock)
	return info;
    }

  return NULL;
}

/*
 * Add a new lock to the thread's list of locks for which it has a read lock.
 * A new info node must be allocated for this, which is taken from the thread's
 * free list, or by calling malloc. If malloc fails, a null pointer is
 * returned. Otherwise the lock info structure is initialized and pushed
 * onto the thread's list.
 */

static marcel_readlock_info *
rwlock_add_to_list(marcel_descr self, marcel_rwlock_t *rwlock)
{
  marcel_readlock_info *info = THREAD_GETMEM (self, p_readlock_free);

  if (info != NULL)
    THREAD_SETMEM (self, p_readlock_free, info->pr_next);
  else
    info = malloc(sizeof *info);

  if (info == NULL)
    return NULL;

  info->pr_lock_count = 1;
  info->pr_lock = rwlock;
  info->pr_next = THREAD_GETMEM (self, p_readlock_list);
  THREAD_SETMEM (self, p_readlock_list, info);

  return info;
}

/*
 * If the thread owns a read lock over the given marcel_rwlock_t,
 * and this read lock is tracked in the thread's lock list,
 * this function returns a pointer to the info node in that list.
 * It also decrements the lock count within that node, and if
 * it reaches zero, it removes the node from the list.
 * If nothing is found, it returns a null pointer.
 */

static marcel_readlock_info *
rwlock_remove_from_list(marcel_descr self, marcel_rwlock_t *rwlock)
{
  marcel_readlock_info **pinfo;

  for (pinfo = &self->p_readlock_list; *pinfo != NULL; pinfo = &(*pinfo)->pr_next)
    {
      if ((*pinfo)->pr_lock == rwlock)
	{
	  marcel_readlock_info *info = *pinfo;
	  if (--info->pr_lock_count == 0)
	    *pinfo = info->pr_next;
	  return info;
	}
    }

  return NULL;
}

/*
 * This function checks whether the conditions are right to place a read lock.
 * It returns 1 if so, otherwise zero. The rwlock's internal lock must be
 * locked upon entry.
 */

static int
rwlock_can_rdlock(marcel_rwlock_t *rwlock, int have_lock_already)
{
  /* Can't readlock; it is write locked. */
  if (rwlock->__rw_writer != NULL)
    return 0;

  /* Lock prefers readers; get it. */
  if (rwlock->__rw_kind == MARCEL_RWLOCK_PREFER_READER_NP)
    return 1;

  /* Lock prefers writers, but none are waiting. */
  if (queue_is_empty((marcel_t*)&rwlock->__rw_write_waiting))
    return 1;

  /* Writers are waiting, but this thread already has a read lock */
  if (have_lock_already)
    return 1;

  /* Writers are waiting, and this is a new lock */
  return 0;
}

/*
 * This function helps support brain-damaged recursive read locking
 * semantics required by Unix 98, while maintaining write priority.
 * This basically determines whether this thread already holds a read lock
 * already. It returns 1 if so, otherwise it returns 0.
 *
 * If the thread has any ``untracked read locks'' then it just assumes
 * that this lock is among them, just to be safe, and returns 1.
 *
 * Also, if it finds the thread's lock in the list, it sets the pointer
 * referenced by pexisting to refer to the list entry.
 *
 * If the thread has no untracked locks, and the lock is not found
 * in its list, then it is added to the list. If this fails,
 * then *pout_of_mem is set to 1.
 */

static int
rwlock_have_already(marcel_descr *pself, marcel_rwlock_t *rwlock,
    marcel_readlock_info **pexisting, int *pout_of_mem)
{
  marcel_readlock_info *existing = NULL;
  int out_of_mem = 0, have_lock_already = 0;
  marcel_descr self = *pself;

  if (rwlock->__rw_kind == MARCEL_RWLOCK_PREFER_WRITER_NP)
    {
      if (!self)
	*pself = self = thread_self();

      existing = rwlock_is_in_list(self, rwlock);

      if (existing != NULL
	  || THREAD_GETMEM (self, p_untracked_readlock_count) > 0)
	have_lock_already = 1;
      else
	{
	  existing = rwlock_add_to_list(self, rwlock);
	  if (existing == NULL)
	    out_of_mem = 1;
	}
    }

  *pout_of_mem = out_of_mem;
  *pexisting = existing;

  return have_lock_already;
}

DEF_MARCEL_POSIX(int,
		 rwlock_init, (marcel_rwlock_t * __restrict rwlock,
			       const marcel_rwlockattr_t * __restrict attr),
		 (rwlock, attr),
{
  __marcel_init_lock(&rwlock->__rw_lock);
  rwlock->__rw_readers = 0;
  rwlock->__rw_writer = NULL;
  rwlock->__rw_read_waiting = NULL;
  rwlock->__rw_write_waiting = NULL;

  if (attr == NULL)
    {
      rwlock->__rw_kind = MARCEL_RWLOCK_DEFAULT_NP;
      rwlock->__rw_pshared = MARCEL_PROCESS_PRIVATE;
    }
  else
    {
      rwlock->__rw_kind = attr->__lockkind;
      rwlock->__rw_pshared = attr->__pshared;
    }

  return 0;
})
//VD:strong_alias (__marcel_rwlock_init, marcel_rwlock_init)
DEF_PTHREAD(int, rwlock_init, (pthread_rwlock_t * __restrict rwlock,
			       const pthread_rwlockattr_t * __restrict attr),
		(rwlock, attr))
DEF___PTHREAD(int, rwlock_init, (pthread_rwlock_t * __restrict rwlock,
			       const pthread_rwlockattr_t * __restrict attr),
		(rwlock, attr))


DEF_MARCEL_POSIX(int,
		 rwlock_destroy, (marcel_rwlock_t *rwlock), (rwlock),
{
  int readers;
  marcel_descr writer;

  __pmarcel_lock (&rwlock->__rw_lock, NULL);
  readers = rwlock->__rw_readers;
  writer = rwlock->__rw_writer;
  __pmarcel_unlock (&rwlock->__rw_lock);

  if (readers > 0 || writer != NULL)
    return EBUSY;

  return 0;
})
//VD:strong_alias (__marcel_rwlock_destroy, marcel_rwlock_destroy)
DEF_PTHREAD(int, rwlock_destroy, (pthread_rwlock_t *rwlock), (rwlock))
DEF___PTHREAD(int, rwlock_destroy, (pthread_rwlock_t *rwlock), (rwlock))

DEF_MARCEL_POSIX(int,
		 rwlock_rdlock, (marcel_rwlock_t *rwlock), (rwlock),
{
  marcel_descr self = NULL;
  marcel_readlock_info *existing;
  int out_of_mem, have_lock_already;

  have_lock_already = rwlock_have_already(&self, rwlock,
					  &existing, &out_of_mem);

  if (self == NULL)
    self = thread_self ();

  for (;;)
    {
      __pmarcel_lock (&rwlock->__rw_lock, self);

      if (rwlock_can_rdlock(rwlock, have_lock_already))
	break;

      enqueue ((marcel_t*)&rwlock->__rw_read_waiting, self);
      __pmarcel_unlock (&rwlock->__rw_lock);
      suspend (self); /* This is not a cancellation point */
    }

  ++rwlock->__rw_readers;
  __pmarcel_unlock (&rwlock->__rw_lock);

  if (have_lock_already || out_of_mem)
    {
      if (existing != NULL)
	++existing->pr_lock_count;
      else
	++self->p_untracked_readlock_count;
    }

  return 0;
})
//VD:strong_alias (__marcel_rwlock_rdlock, marcel_rwlock_rdlock)
DEF_PTHREAD(int, rwlock_rdlock, (pthread_rwlock_t *rwlock), (rwlock))
DEF___PTHREAD(int, rwlock_rdlock, (pthread_rwlock_t *rwlock), (rwlock))

#if 0 //VD:
//VD: A lire et corriger
DEF_MARCEL_POSIX(int,
		 rwlock_timedrdlock, (marcel_rwlock_t * __restrict rwlock,
				      const struct timespec * __restrict abstime),
		 (rwlock, abstime),
{
  marcel_descr self = NULL;
  marcel_readlock_info *existing;
  int out_of_mem, have_lock_already;
  //VD:marcel_extricate_if extr;

  if (abstime->tv_nsec < 0 || abstime->tv_nsec >= 1000000000)
    return EINVAL;

  have_lock_already = rwlock_have_already(&self, rwlock,
					  &existing, &out_of_mem);

  if (self == NULL)
    self = thread_self ();

  /* Set up extrication interface */
  //VD:extr.pu_object = rwlock;
  //VD:extr.pu_extricate_func = rwlock_rd_extricate_func;

  /* Register extrication interface */
  //VD:__marcel_set_own_extricate_if (self, &extr);

  for (;;)
    {
      __pmarcel_lock (&rwlock->__rw_lock, self);

      if (rwlock_can_rdlock(rwlock, have_lock_already))
	break;

      enqueue (&rwlock->__rw_read_waiting, self);
      __pmarcel_unlock (&rwlock->__rw_lock);
      /* This is not a cancellation point */
      if (timedsuspend (self, abstime) == 0)
	{
	  int was_on_queue;

	  __pmarcel_lock (&rwlock->__rw_lock, self);
	  was_on_queue = remove_from_queue (&rwlock->__rw_read_waiting, self);
	  __pmarcel_unlock (&rwlock->__rw_lock);

	  if (was_on_queue)
	    {
	      //VD:__marcel_set_own_extricate_if (self, 0);
	      return ETIMEDOUT;
	    }

	  /* Eat the outstanding restart() from the signaller */
	  suspend (self);
	}
    }

  //VD:__marcel_set_own_extricate_if (self, 0);

  ++rwlock->__rw_readers;
  __pmarcel_unlock (&rwlock->__rw_lock);

  if (have_lock_already || out_of_mem)
    {
      if (existing != NULL)
	++existing->pr_lock_count;
      else
	++self->p_untracked_readlock_count;
    }

  return 0;
})
//VD:strong_alias (__marcel_rwlock_timedrdlock, marcel_rwlock_timedrdlock)
#endif

DEF_MARCEL_POSIX(int,
		 rwlock_tryrdlock, (marcel_rwlock_t *rwlock), (rwlock),
{
  marcel_descr self = thread_self();
  marcel_readlock_info *existing;
  int out_of_mem, have_lock_already;
  int retval = EBUSY;

  have_lock_already = rwlock_have_already(&self, rwlock,
      &existing, &out_of_mem);

  __pmarcel_lock (&rwlock->__rw_lock, self);

  /* 0 is passed to here instead of have_lock_already.
     This is to meet Single Unix Spec requirements:
     if writers are waiting, marcel_rwlock_tryrdlock
     does not acquire a read lock, even if the caller has
     one or more read locks already. */

  if (rwlock_can_rdlock(rwlock, 0))
    {
      ++rwlock->__rw_readers;
      retval = 0;
    }

  __pmarcel_unlock (&rwlock->__rw_lock);

  if (retval == 0)
    {
      if (have_lock_already || out_of_mem)
	{
	  if (existing != NULL)
	    ++existing->pr_lock_count;
	  else
	    ++self->p_untracked_readlock_count;
	}
    }

  return retval;
})
//VD:strong_alias (__marcel_rwlock_tryrdlock, marcel_rwlock_tryrdlock)
DEF_PTHREAD(int, rwlock_tryrdlock, (pthread_rwlock_t *rwlock), (rwlock))
DEF___PTHREAD(int, rwlock_tryrdlock, (pthread_rwlock_t *rwlock), (rwlock))


DEF_MARCEL_POSIX(int,
		 rwlock_wrlock, (marcel_rwlock_t *rwlock), (rwlock),
{
  marcel_descr self = thread_self ();

  while(1)
    {
      __pmarcel_lock (&rwlock->__rw_lock, self);
      if (rwlock->__rw_readers == 0 && rwlock->__rw_writer == NULL)
	{
	  rwlock->__rw_writer = (marcel_descr)self;
	  __pmarcel_unlock (&rwlock->__rw_lock);
	  return 0;
	}

      /* Suspend ourselves, then try again */
      enqueue ((marcel_t*)&rwlock->__rw_write_waiting, self);
      __pmarcel_unlock (&rwlock->__rw_lock);
      suspend (self); /* This is not a cancellation point */
    }
})
//VD:strong_alias (__marcel_rwlock_wrlock, marcel_rwlock_wrlock)
DEF_PTHREAD(int, rwlock_wrlock, (pthread_rwlock_t *rwlock), (rwlock))
DEF___PTHREAD(int, rwlock_wrlock, (pthread_rwlock_t *rwlock), (rwlock))


#if 0 //VD:
//VD: A lire et corriger
int
__marcel_rwlock_timedwrlock (marcel_rwlock_t * __restrict rwlock,
			      const struct timespec * __restrict abstime)
{
  marcel_descr self;
  //VD:marcel_extricate_if extr;

  if (abstime->tv_nsec < 0 || abstime->tv_nsec >= 1000000000)
    return EINVAL;

  self = thread_self ();

  /* Set up extrication interface */
  //VD:extr.pu_object = rwlock;
  //VD:extr.pu_extricate_func =  rwlock_wr_extricate_func;

  /* Register extrication interface */
  //VD:__marcel_set_own_extricate_if (self, &extr);

  while(1)
    {
      __pmarcel_lock (&rwlock->__rw_lock, self);

      if (rwlock->__rw_readers == 0 && rwlock->__rw_writer == NULL)
	{
	  rwlock->__rw_writer = self;
	  //VD:__marcel_set_own_extricate_if (self, 0);
	  __pmarcel_unlock (&rwlock->__rw_lock);
	  return 0;
	}

      /* Suspend ourselves, then try again */
      enqueue (&rwlock->__rw_write_waiting, self);
      __pmarcel_unlock (&rwlock->__rw_lock);
      /* This is not a cancellation point */
      if (timedsuspend (self, abstime) == 0)
	{
	  int was_on_queue;

	  __pmarcel_lock (&rwlock->__rw_lock, self);
	  was_on_queue = remove_from_queue (&rwlock->__rw_write_waiting, self);
	  __pmarcel_unlock (&rwlock->__rw_lock);

	  if (was_on_queue)
	    {
	      //VD:__marcel_set_own_extricate_if (self, 0);
	      return ETIMEDOUT;
	    }

	  /* Eat the outstanding restart() from the signaller */
	  suspend (self);
	}
    }
}
strong_alias (__marcel_rwlock_timedwrlock, marcel_rwlock_timedwrlock)
#endif

DEF_MARCEL_POSIX(int,
		 rwlock_trywrlock, (marcel_rwlock_t *rwlock), (rwlock),
{
  int result = EBUSY;

  __pmarcel_lock (&rwlock->__rw_lock, NULL);
  if (rwlock->__rw_readers == 0 && rwlock->__rw_writer == NULL)
    {
      rwlock->__rw_writer = (marcel_descr)thread_self ();
      result = 0;
    }
  __pmarcel_unlock (&rwlock->__rw_lock);

  return result;
})
//VD:strong_alias (__marcel_rwlock_trywrlock, marcel_rwlock_trywrlock)
DEF_PTHREAD(int, rwlock_trywrlock, (pthread_rwlock_t *rwlock), (rwlock))
DEF___PTHREAD(int, rwlock_trywrlock, (pthread_rwlock_t *rwlock), (rwlock))


DEF_MARCEL_POSIX(int,
		 rwlock_unlock, (marcel_rwlock_t *rwlock), (rwlock),
{
  marcel_descr torestart;
  marcel_descr th;

  __pmarcel_lock (&rwlock->__rw_lock, NULL);
  if (rwlock->__rw_writer != NULL)
    {
      /* Unlocking a write lock.  */
      if (rwlock->__rw_writer != (marcel_descr)thread_self ())
	{
	  __pmarcel_unlock (&rwlock->__rw_lock);
	  return EPERM;
	}
      rwlock->__rw_writer = NULL;

      if ((rwlock->__rw_kind == MARCEL_RWLOCK_PREFER_READER_NP
	   && !queue_is_empty((marcel_t*)&rwlock->__rw_read_waiting))
	  || (th = dequeue((marcel_t*)&rwlock->__rw_write_waiting)) == NULL)
	{
	  /* Restart all waiting readers.  */
	  torestart = (marcel_descr)rwlock->__rw_read_waiting;
	  rwlock->__rw_read_waiting = NULL;
	  __pmarcel_unlock (&rwlock->__rw_lock);
	  while ((th = dequeue (&torestart)) != NULL)
	    restart (th);
	}
      else
	{
	  /* Restart one waiting writer.  */
	  __pmarcel_unlock (&rwlock->__rw_lock);
	  restart (th);
	}
    }
  else
    {
      /* Unlocking a read lock.  */
      if (rwlock->__rw_readers == 0)
	{
	  __pmarcel_unlock (&rwlock->__rw_lock);
	  return EPERM;
	}

      --rwlock->__rw_readers;
      if (rwlock->__rw_readers == 0)
	/* Restart one waiting writer, if any.  */
	th = dequeue ((marcel_t*)&rwlock->__rw_write_waiting);
      else
	th = NULL;

      __pmarcel_unlock (&rwlock->__rw_lock);
      if (th != NULL)
	restart (th);

      /* Recursive lock fixup */

      if (rwlock->__rw_kind == MARCEL_RWLOCK_PREFER_WRITER_NP)
	{
	  marcel_descr self = thread_self();
	  marcel_readlock_info *victim = rwlock_remove_from_list(self, rwlock);

	  if (victim != NULL)
	    {
	      if (victim->pr_lock_count == 0)
		{
		  victim->pr_next = THREAD_GETMEM (self, p_readlock_free);
		  THREAD_SETMEM (self, p_readlock_free, victim);
		}
	    }
	  else
	    {
	      int val = THREAD_GETMEM (self, p_untracked_readlock_count);
	      if (val > 0)
		THREAD_SETMEM (self, p_untracked_readlock_count, val - 1);
	    }
	}
    }

  return 0;
})
//VD:strong_alias (__marcel_rwlock_unlock, marcel_rwlock_unlock)
DEF_PTHREAD(int, rwlock_unlock, (pthread_rwlock_t *rwlock), (rwlock))
DEF___PTHREAD(int, rwlock_unlock, (pthread_rwlock_t *rwlock), (rwlock))



DEF_MARCEL_POSIX(int,
		 rwlockattr_init, (marcel_rwlockattr_t *attr), (attr),
{
  attr->__lockkind = 0;
  attr->__pshared = MARCEL_PROCESS_PRIVATE;

  return 0;
})
DEF_PTHREAD(int, rwlockattr_init, (pthread_rwlockattr_t *attr), (attr))


DEF_MARCEL_POSIX(int,
		 rwlockattr_destroy, (marcel_rwlockattr_t *attr), (attr),
{
  return 0;
})
//VD:strong_alias (__marcel_rwlockattr_destroy, marcel_rwlockattr_destroy)
DEF_PTHREAD(int, rwlockattr_destroy, (pthread_rwlockattr_t *attr), (attr))
DEF___PTHREAD(int, rwlockattr_destroy, (pthread_rwlockattr_t *attr), (attr))


DEF_MARCEL_POSIX(int,
		 rwlockattr_getpshared,
		 (const marcel_rwlockattr_t * __restrict attr,
		  int * __restrict pshared),
		 (attr, pshared),
{
  *pshared = attr->__pshared;
  return 0;
})
DEF_PTHREAD(int, rwlockattr_getpshared,
		 (const pthread_rwlockattr_t * __restrict attr,
		  int * __restrict pshared),
		 (attr, pshared))


DEF_MARCEL_POSIX(int,
		 rwlockattr_setpshared,
		 (marcel_rwlockattr_t *attr, int pshared), (attr, pshared),
{
  if (pshared != MARCEL_PROCESS_PRIVATE && pshared != MARCEL_PROCESS_SHARED)
    return EINVAL;

  /* For now it is not possible to shared a conditional variable.  */
  if (pshared != MARCEL_PROCESS_PRIVATE)
    return ENOSYS;

  attr->__pshared = pshared;

  return 0;
})
DEF_PTHREAD(int, rwlockattr_setpshared,
		 (pthread_rwlockattr_t *attr, int pshared), (attr, pshared))


DEF_MARCEL_POSIX(int,
		 rwlockattr_getkind_np,
		 (const marcel_rwlockattr_t *attr, int *pref), (attr, pref),
{
  *pref = attr->__lockkind;
  return 0;
})
DEF_PTHREAD(int, rwlockattr_getkind_np,
		 (const pthread_rwlockattr_t *attr, int *pref), (attr, pref))


DEF_MARCEL_POSIX(int,
		 rwlockattr_setkind_np, (marcel_rwlockattr_t *attr, int pref),
		 (attr, pref),
{
  if (pref != MARCEL_RWLOCK_PREFER_READER_NP
      && pref != MARCEL_RWLOCK_PREFER_WRITER_NP
      && pref != MARCEL_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP
      && pref != MARCEL_RWLOCK_DEFAULT_NP)
    return EINVAL;

  attr->__lockkind = pref;

  return 0;
})
DEF_PTHREAD(int, rwlockattr_setkind_np, (pthread_rwlockattr_t *attr, int pref),
		 (attr, pref))


#endif // __USE_UNIX98 (AD:)
