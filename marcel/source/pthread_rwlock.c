/* Copyright 2009 INRIA
   Modified for Marcel by Ludovic Courtès <ludovic.courtes@inria.fr>.

   Copyright (C) 2002, 2007 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2002.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

/* Taken from version 2.7 of the GNU C Library.  This must be in sync with
   the data structures in <nptl_types.h>.  */

#include <marcel.h>
#include "marcel_fastlock.h"

#include <asm/nptl_types.h>
#include <errno.h>



/* Attributes.  */

/* We use `struct marcel_rwlockattr' as the internal structure that underlies
   `lpt_rwlockattr_t'.  Make sure they are compatible.  */
static __inline__ void
lpt_check_rwlockattr_type (void)
{
  char test[sizeof (struct marcel_rwlockattr) > sizeof (lpt_rwlockattr_t)
	    || __alignof (struct marcel_rwlockattr) > sizeof (lpt_rwlockattr_t)
	    ? -1 : 1]
    TBX_UNUSED;
}

static const struct marcel_rwlockattr default_attr =
  {
    .__lockkind = MARCEL_RWLOCK_DEFAULT_NP,
    .__pshared = MARCEL_PROCESS_PRIVATE
  };


int
lpt_rwlockattr_init (attr)
     lpt_rwlockattr_t *attr;
{
  struct marcel_rwlockattr *iattr;

  iattr = (struct marcel_rwlockattr *) attr;

  iattr->__lockkind = MARCEL_RWLOCK_DEFAULT_NP;
  iattr->__pshared = MARCEL_PROCESS_PRIVATE;

  return 0;
}

int
lpt_rwlockattr_destroy (attr)
     lpt_rwlockattr_t *attr;
{
  /* Nothing to do.  For now.  */

  return 0;
}

int
lpt_rwlockattr_getkind_np (attr, pref)
     const lpt_rwlockattr_t *attr;
     int *pref;
{
  *pref = ((const struct marcel_rwlockattr *) attr)->__lockkind;

  return 0;
}

int
lpt_rwlockattr_getpshared (attr, pshared)
     const lpt_rwlockattr_t *attr;
     int *pshared;
{
  *pshared = ((const struct marcel_rwlockattr *) attr)->__pshared;

  return 0;
}

int
lpt_rwlockattr_setkind_np (attr, pref)
     lpt_rwlockattr_t *attr;
     int pref;
{
  struct marcel_rwlockattr *iattr;

  /* FIXME: Check whether the MARCEL_RWLOCK* constants are equal to NPTL's
     PTHREAD_RWLOCK* constants.  */

  if (pref != MARCEL_RWLOCK_PREFER_READER_NP
      && pref != MARCEL_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP
      && __builtin_expect  (pref != MARCEL_RWLOCK_PREFER_WRITER_NP, 0))
    return EINVAL;

  iattr = (struct marcel_rwlockattr *) attr;

  iattr->__lockkind = pref;

  return 0;
}

int
lpt_rwlockattr_setpshared (attr, pshared)
     lpt_rwlockattr_t *attr;
     int pshared;
{
  struct marcel_rwlockattr *iattr;

  if (pshared != MARCEL_PROCESS_SHARED
      && __builtin_expect (pshared != MARCEL_PROCESS_PRIVATE, 0))
    return EINVAL;

  iattr = (struct marcel_rwlockattr *) attr;

  iattr->__pshared = pshared;

  return 0;
}


/* RW locks.  */

/* Check whether rwlock prefers readers.   */
#define MARCEL_RWLOCK_PREFER_READER_P(rwlock) \
  ((rwlock)->__data.__flags == 0)


int
lpt_rwlock_init (rwlock, attr)
     lpt_rwlock_t *rwlock;
     const lpt_rwlockattr_t *attr;
{
  const struct marcel_rwlockattr *iattr;

  iattr = ((const struct marcel_rwlockattr *) attr) ?: &default_attr;

  rwlock->__data.__lock = 0;
  rwlock->__data.__nr_readers = 0;
  rwlock->__data.__readers_wakeup.__status = 0;
  rwlock->__data.__writer_wakeup.__status = 0;
  rwlock->__data.__nr_readers_queued = 0;
  rwlock->__data.__nr_writers_queued = 0;
  rwlock->__data.__writer = 0;

  rwlock->__data.__flags
    = iattr->__lockkind == MARCEL_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP;

#if 0 /* XXX: We don't honor the `__shared' field.  */

  /* The __SHARED field is computed to minimize the work that needs to
     be done while handling the futex.  There are two inputs: the
     availability of private futexes and whether the rwlock is shared
     or private.  Unfortunately the value of a private rwlock is
     fixed: it must be zero.  The PRIVATE_FUTEX flag has the value
     0x80 in case private futexes are available and zero otherwise.
     This leads to the following table:

                 |     pshared     |     result
                 | shared  private | shared  private |
     ------------+-----------------+-----------------+
     !avail 0    |     0       0   |     0       0   |
      avail 0x80 |  0x80       0   |     0    0x80   |

     If the pshared value is in locking functions XORed with avail
     we get the expected result.  */
#ifdef __ASSUME_PRIVATE_FUTEX
  rwlock->__data.__shared = (iattr->pshared == MARCEL_PROCESS_PRIVATE
			     ? 0 : FUTEX_PRIVATE_FLAG);
#else
  rwlock->__data.__shared = (iattr->pshared == MARCEL_PROCESS_PRIVATE
			     ? 0
			     : THREAD_GETMEM (THREAD_SELF,
					      header.private_futex));
#endif
#endif

  rwlock->__data.__pad1 = 0;
  rwlock->__data.__pad2 = 0;

  return 0;
}

int
lpt_rwlock_destroy (rwlock)
     lpt_rwlock_t *rwlock;
{
  /* Nothing to be done.  For now.  */
  return 0;
}

/* Acquire read lock for RWLOCK.  */
int
lpt_rwlock_rdlock (rwlock)
     lpt_rwlock_t *rwlock;
{
  int result = 0;

  /* Make sure we are along.  */
  lpt_lock_acquire (&rwlock->__data.__lock);

  while (1)
    {
      /* Get the rwlock if there is no writer...  */
      if (rwlock->__data.__writer == 0
	  /* ...and if either no writer is waiting or we prefer readers.  */
	  && (!rwlock->__data.__nr_writers_queued
	      || MARCEL_RWLOCK_PREFER_READER_P (rwlock)))
	{
	  /* Increment the reader counter.  Avoid overflow.  */
	  if (__builtin_expect (++rwlock->__data.__nr_readers == 0, 0))
	    {
	      /* Overflow on number of readers.	 */
	      --rwlock->__data.__nr_readers;
	      result = EAGAIN;
	    }

	  break;
	}

      /* Make sure we are not holding the rwlock as a writer.  This is
	 a deadlock situation we recognize and report.  */
      if (__builtin_expect (rwlock->__data.__writer
			    == marcel_self (), 0))
	{
	  result = EDEADLK;
	  break;
	}

      /* Remember that we are a reader.  */
      if (__builtin_expect (++rwlock->__data.__nr_readers_queued == 0, 0))
	{
	  /* Overflow on number of queued readers.  */
	  --rwlock->__data.__nr_readers_queued;
	  result = EAGAIN;
	  break;
	}

      /* Free the lock.  */
      lpt_lock_release (&rwlock->__data.__lock);

      /* Wait for the writer to finish.  */
      __lpt_lock_wait (&rwlock->__data.__readers_wakeup);

      /* Get the lock.  */
      lpt_lock_acquire (&rwlock->__data.__lock);

      --rwlock->__data.__nr_readers_queued;
    }

  /* We are done, free the lock.  */
  lpt_lock_release (&rwlock->__data.__lock);

  return result;
}

/* Try to acquire read lock for RWLOCK or return after specfied time.  */
int
lpt_rwlock_timedrdlock (rwlock, abstime)
     lpt_rwlock_t *rwlock;
     const struct timespec *abstime;
{
  int result = 0;

  /* Make sure we are alone.  */
  lpt_lock_acquire(&rwlock->__data.__lock);

  while (1)
    {
      int err;

      /* Get the rwlock if there is no writer...  */
      if (rwlock->__data.__writer == 0
	  /* ...and if either no writer is waiting or we prefer readers.  */
	  && (!rwlock->__data.__nr_writers_queued
	      || MARCEL_RWLOCK_PREFER_READER_P (rwlock)))
	{
	  /* Increment the reader counter.  Avoid overflow.  */
	  if (++rwlock->__data.__nr_readers == 0)
	    {
	      /* Overflow on number of readers.	 */
	      --rwlock->__data.__nr_readers;
	      result = EAGAIN;
	    }

	  break;
	}

      /* Make sure we are not holding the rwlock as a writer.  This is
	 a deadlock situation we recognize and report.  */
      if (__builtin_expect (rwlock->__data.__writer
			    == marcel_self (), 0))
	{
	  result = EDEADLK;
	  break;
	}

      /* Make sure the passed in timeout value is valid.  Ideally this
	 test would be executed once.  But since it must not be
	 performed if we would not block at all simply moving the test
	 to the front is no option.  Replicating all the code is
	 costly while this test is not.  */
      if (__builtin_expect (abstime->tv_nsec >= 1000000000
                            || abstime->tv_nsec < 0, 0))
	{
	  result = EINVAL;
	  break;
	}

      /* Get the current time.  So far we support only one clock.  */
      struct timeval tv;
      (void) gettimeofday (&tv, NULL);

      /* Convert the absolute timeout value to a relative timeout.  */
      struct timespec rt;
      rt.tv_sec = abstime->tv_sec - tv.tv_sec;
      rt.tv_nsec = abstime->tv_nsec - tv.tv_usec * 1000;
      if (rt.tv_nsec < 0)
	{
	  rt.tv_nsec += 1000000000;
	  --rt.tv_sec;
	}
      /* Did we already time out?  */
      if (rt.tv_sec < 0)
	{
	  /* Yep, return with an appropriate error.  */
	  result = ETIMEDOUT;
	  break;
	}

      /* Remember that we are a reader.  */
      if (++rwlock->__data.__nr_readers_queued == 0)
	{
	  /* Overflow on number of queued readers.  */
	  --rwlock->__data.__nr_readers_queued;
	  result = EAGAIN;
	  break;
	}

      /* Free the lock.  */
      lpt_lock_release (&rwlock->__data.__lock);

      /* Wait for the writer to finish.  */
      err = __lpt_lock_timed_wait (&rwlock->__data.__readers_wakeup, &rt);

      /* Get the lock.  */
      lpt_lock_acquire (&rwlock->__data.__lock);

      --rwlock->__data.__nr_readers_queued;

      /* Did the futex call time out?  */
      if (err == ETIMEDOUT)
	{
	  /* Yep, report it.  */
	  result = ETIMEDOUT;
	  break;
	}
    }

  /* We are done, free the lock.  */
  lpt_lock_release (&rwlock->__data.__lock);

  return result;
}

/* Try to acquire write lock for RWLOCK or return after specfied time.	*/
int
lpt_rwlock_timedwrlock (rwlock, abstime)
     lpt_rwlock_t *rwlock;
     const struct timespec *abstime;
{
  int result = 0;

  /* Make sure we are along.  */
  lpt_lock_acquire (&rwlock->__data.__lock);

  while (1)
    {
      int err;

      /* Get the rwlock if there is no writer and no reader.  */
      if (rwlock->__data.__writer == 0 && rwlock->__data.__nr_readers == 0)
	{
	  /* Mark self as writer.  */
	  rwlock->__data.__writer = marcel_self ();
	  break;
	}

      /* Make sure we are not holding the rwlock as a writer.  This is
	 a deadlock situation we recognize and report.  */
      if (__builtin_expect (rwlock->__data.__writer
			    == marcel_self (), 0))
	{
	  result = EDEADLK;
	  break;
	}

      /* Make sure the passed in timeout value is valid.  Ideally this
	 test would be executed once.  But since it must not be
	 performed if we would not block at all simply moving the test
	 to the front is no option.  Replicating all the code is
	 costly while this test is not.  */
      if (__builtin_expect (abstime->tv_nsec >= 1000000000
                            || abstime->tv_nsec < 0, 0))
	{
	  result = EINVAL;
	  break;
	}

      /* Get the current time.  So far we support only one clock.  */
      struct timeval tv;
      (void) gettimeofday (&tv, NULL);

      /* Convert the absolute timeout value to a relative timeout.  */
      struct timespec rt;
      rt.tv_sec = abstime->tv_sec - tv.tv_sec;
      rt.tv_nsec = abstime->tv_nsec - tv.tv_usec * 1000;
      if (rt.tv_nsec < 0)
	{
	  rt.tv_nsec += 1000000000;
	  --rt.tv_sec;
	}
      /* Did we already time out?  */
      if (rt.tv_sec < 0)
	{
	  result = ETIMEDOUT;
	  break;
	}

      /* Remember that we are a writer.  */
      if (++rwlock->__data.__nr_writers_queued == 0)
	{
	  /* Overflow on number of queued writers.  */
	  --rwlock->__data.__nr_writers_queued;
	  result = EAGAIN;
	  break;
	}

      /* Free the lock.  */
      lpt_lock_release (&rwlock->__data.__lock);

      /* Wait for the writer or reader(s) to finish.  */
      err = __lpt_lock_timed_wait (&rwlock->__data.__writer_wakeup, &rt);

      /* Get the lock.  */
      lpt_lock_acquire (&rwlock->__data.__lock);

      /* To start over again, remove the thread from the writer list.  */
      --rwlock->__data.__nr_writers_queued;

      /* Did the futex call time out?  */
      if (err == ETIMEDOUT)
	{
	  result = ETIMEDOUT;
	  break;
	}
    }

  /* We are done, free the lock.  */
  lpt_lock_release (&rwlock->__data.__lock);

  return result;
}

int
lpt_rwlock_tryrdlock (rwlock)
     lpt_rwlock_t *rwlock;
{
  int result = EBUSY;

  lpt_lock_acquire (&rwlock->__data.__lock);

  if (rwlock->__data.__writer == 0
      && (rwlock->__data.__nr_writers_queued == 0
	  || MARCEL_RWLOCK_PREFER_READER_P (rwlock)))
    {
      if (__builtin_expect (++rwlock->__data.__nr_readers == 0, 0))
	{
	  --rwlock->__data.__nr_readers;
	  result = EAGAIN;
	}
      else
	result = 0;
    }

  lpt_lock_release (&rwlock->__data.__lock);

  return result;
}

int
lpt_rwlock_trywrlock (rwlock)
     lpt_rwlock_t *rwlock;
{
  int result = EBUSY;

  lpt_lock_acquire (&rwlock->__data.__lock);

  if (rwlock->__data.__writer == 0 && rwlock->__data.__nr_readers == 0)
    {
      rwlock->__data.__writer = marcel_self ();
      result = 0;
    }

  lpt_lock_release (&rwlock->__data.__lock);

  return result;
}

/* Unlock RWLOCK.  */
int
lpt_rwlock_unlock (rwlock)
     lpt_rwlock_t *rwlock;
{
  lpt_lock_acquire (&rwlock->__data.__lock);
  if (rwlock->__data.__writer)
    rwlock->__data.__writer = 0;
  else
    --rwlock->__data.__nr_readers;
  if (rwlock->__data.__nr_readers == 0)
    {
      if (rwlock->__data.__nr_writers_queued)
	{
	  /* ++rwlock->__data.__writer_wakeup; */
	  lpt_lock_release (&rwlock->__data.__lock);
	  __lpt_lock_signal (&rwlock->__data.__writer_wakeup);
	  return 0;
	}
      else if (rwlock->__data.__nr_readers_queued)
	{
	  /* ++rwlock->__data.__readers_wakeup; */
	  lpt_lock_release (&rwlock->__data.__lock);
	  __lpt_lock_broadcast (&rwlock->__data.__readers_wakeup);
	  return 0;
	}
    }
  lpt_lock_release (&rwlock->__data.__lock);
  return 0;
}


/* Acquire write lock for RWLOCK.  */
int
lpt_rwlock_wrlock (rwlock)
     lpt_rwlock_t *rwlock;
{
  int result = 0;

  /* Make sure we are along.  */
  lpt_lock_acquire (&rwlock->__data.__lock);

  while (1)
    {
      /* Get the rwlock if there is no writer and no reader.  */
      if (rwlock->__data.__writer == 0 && rwlock->__data.__nr_readers == 0)
	{
	  /* Mark self as writer.  */
	  rwlock->__data.__writer = marcel_self ();
	  break;
	}

      /* Make sure we are not holding the rwlock as a writer.  This is
	 a deadlock situation we recognize and report.  */
      if (__builtin_expect (rwlock->__data.__writer
			    == marcel_self (), 0))
	{
	  result = EDEADLK;
	  break;
	}

      /* Remember that we are a writer.  */
      if (++rwlock->__data.__nr_writers_queued == 0)
	{
	  /* Overflow on number of queued writers.  */
	  --rwlock->__data.__nr_writers_queued;
	  result = EAGAIN;
	  break;
	}

      /* Free the lock.  */
      lpt_lock_release (&rwlock->__data.__lock);

      /* Wait for the writer or reader(s) to finish.  */
      __lpt_lock_wait (&rwlock->__data.__writer_wakeup);

      /* Get the lock.  */
      lpt_lock_acquire (&rwlock->__data.__lock);

      /* To start over again, remove the thread from the writer list.  */
      --rwlock->__data.__nr_writers_queued;
    }

  /* We are done, free the lock.  */
  lpt_lock_release (&rwlock->__data.__lock);

  return result;
}


/* Aliases.  */

strong_alias (lpt_rwlockattr_init, pthread_rwlockattr_init)
strong_alias (lpt_rwlockattr_destroy, pthread_rwlockattr_destroy)
strong_alias (lpt_rwlockattr_getkind_np, pthread_rwlockattr_getkind_np)
strong_alias (lpt_rwlockattr_getpshared, pthread_rwlockattr_getpshared)
strong_alias (lpt_rwlockattr_setkind_np, pthread_rwlockattr_setkind_np)
strong_alias (lpt_rwlockattr_setpshared, pthread_rwlockattr_setpshared)

strong_alias (lpt_rwlock_init, pthread_rwlock_init)
strong_alias (lpt_rwlock_destroy, pthread_rwlock_destroy)
strong_alias (lpt_rwlock_rdlock, pthread_rwlock_rdlock)
strong_alias (lpt_rwlock_timedrdlock, pthread_rwlock_timedrdlock)
strong_alias (lpt_rwlock_timedwrlock, pthread_rwlock_timedwrlock)
strong_alias (lpt_rwlock_tryrdlock, pthread_rwlock_tryrdlock)
strong_alias (lpt_rwlock_trywrlock, pthread_rwlock_trywrlock)
strong_alias (lpt_rwlock_unlock, pthread_rwlock_unlock)
strong_alias (lpt_rwlock_wrlock, pthread_rwlock_wrlock)

/*
   Local Variables:
   tab-width: 8
   c-style: "gnu"
   End:
 */
