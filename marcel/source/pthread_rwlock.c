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

MA_VERIFY (sizeof (struct marcel_rwlockattr) <= sizeof (lpt_rwlockattr_t));
MA_VERIFY (__alignof (struct marcel_rwlockattr) <= sizeof (lpt_rwlockattr_t));


static const struct marcel_rwlockattr default_attr =
{
	.__lockkind = MARCEL_RWLOCK_DEFAULT_NP,
	.__pshared = MARCEL_PROCESS_PRIVATE
};


DEF_PTHREAD (int, rwlockattr_init, (marcel_rwlockattr_t *attr), (attr))
DEF_MARCEL_PMARCEL (int, rwlockattr_init, (marcel_rwlockattr_t *attr), (attr),
{
	attr->__lockkind = MARCEL_RWLOCK_DEFAULT_NP;
	attr->__pshared = MARCEL_PROCESS_PRIVATE;
	
	return 0;
})

DEF_PTHREAD (int, rwlockattr_destroy, (marcel_rwlockattr_t *attr TBX_UNUSED), (attr))
DEF_MARCEL_PMARCEL (int, rwlockattr_destroy, (marcel_rwlockattr_t *attr TBX_UNUSED), (attr),
{
	/* Nothing to do.  For now.  */
	return 0;
})

DEF_PTHREAD (int, rwlockattr_getkind_np,
	     (const marcel_rwlockattr_t *attr, int *pref),
	     (attr, pref))
DEF_MARCEL_PMARCEL (int, rwlockattr_getkind_np,
		    (const marcel_rwlockattr_t *attr, int *pref),
		    (attr, pref),
{
	*pref = ((const struct marcel_rwlockattr *) attr)->__lockkind;
	
	return 0;
})

DEF_PTHREAD (int, rwlockattr_getpshared,
	     (const marcel_rwlockattr_t *attr, int *pshared),
	     (attr, pshared))
DEF_MARCEL_PMARCEL (int, rwlockattr_getpshared,
		    (const marcel_rwlockattr_t *attr, int *pshared),
		    (attr, pshared),
{
	*pshared = ((const struct marcel_rwlockattr *) attr)->__pshared;
	
	return 0;
})

DEF_PTHREAD (int, rwlockattr_setkind_np,
	     (marcel_rwlockattr_t *attr, int pref),
	     (attr, pref))
DEF_MARCEL_PMARCEL (int, rwlockattr_setkind_np,
		    (marcel_rwlockattr_t *attr, int pref),
		    (attr, pref),
{
	/* FIXME: Check whether the MARCEL_RWLOCK* constants are equal to NPTL's
	   PTHREAD_RWLOCK* constants.  */

	if (pref != MARCEL_RWLOCK_PREFER_READER_NP
	    && pref != MARCEL_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP
	    && __builtin_expect  (pref != MARCEL_RWLOCK_PREFER_WRITER_NP, 0))
		return EINVAL;

	attr->__lockkind = pref;

	return 0;
})

DEF_PTHREAD (int, rwlockattr_setpshared,
	     (marcel_rwlockattr_t *attr, int pshared),
	     (attr, pshared))
DEF_MARCEL_PMARCEL (int, rwlockattr_setpshared,
		    (marcel_rwlockattr_t *attr, int pshared),
		    (attr, pshared),
{
	if (pshared != MARCEL_PROCESS_SHARED
	    && __builtin_expect (pshared != MARCEL_PROCESS_PRIVATE, 0))
		return EINVAL;

	if (pshared == MARCEL_PROCESS_SHARED)
		return ENOTSUP;
	attr = (struct marcel_rwlockattr *) attr;

	attr->__pshared = pshared;

	return 0;
})


/* RW locks.  */

/* Check whether rwlock prefers readers.   */
#define MARCEL_RWLOCK_PREFER_READER_P(rwlock)	\
	((rwlock)->__data.__flags == 0)


/* A "generation number" is associated with each RW lock.  This is used in
   the `timed{rd,wr}lock' variants so that we can know whether someone
   unlocked the RWLOCK by the time we actually start waiting, in which case
   we just start again the timed lock operation.  */

/* Return the generation number of RWLOCK.  */
#define MA_RWLOCK_GENERATION_NUMBER(_rwlock)	\
	((_rwlock)->__data.__lock & ~1UL)

/* Increment the generation number of RWLOCK, assuming it's already
   locked.  */
#define MA_RWLOCK_INCREMENT_GENERATION_NUMBER(_rwlock)			\
	do								\
	{								\
		/* XXX: We're assuming that `lpt_lock_acquire ()' uses a bit-spinlock \
		   on bit 0.  */					\
		MA_BUG_ON (!ma_bit_spin_is_locked			\
			   (0, (unsigned long *) &(_rwlock)->__data.__lock)); \
		(_rwlock)->__data.__lock =				\
			(MA_RWLOCK_GENERATION_NUMBER (_rwlock) + 2) | 1UL; \
	}								\
	while (0)


DEF_PTHREAD (int, rwlock_init,
	     (lpt_rwlock_t *rwlock, const marcel_rwlockattr_t *attr),
	     (rwlock, attr))
DEF_MARCEL_PMARCEL (int, rwlock_init,
		    (lpt_rwlock_t *rwlock, const marcel_rwlockattr_t *attr),
		    (rwlock, attr),
{
	const struct marcel_rwlockattr *iattr;
	struct _lpt_fastlock fastlock_unlocked = MA_LPT_FASTLOCK_UNLOCKED;

	iattr = attr ?: &default_attr;

	rwlock->__data.__lock = 0;
	rwlock->__data.__nr_readers = 0;
	rwlock->__data.__readers_wakeup = fastlock_unlocked;
	rwlock->__data.__writer_wakeup = fastlock_unlocked;
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

#if 0
	/* There is no strict need to initialize them, and we actually need the extra
	 * room on some architectures to cope with marcel_t and _lpt_fastlock being
	 * 64bits.  */
	rwlock->__data.__pad1 = 0;
	rwlock->__data.__pad2 = 0;
#endif

	return 0;
})

DEF_PTHREAD (int, rwlock_destroy, (lpt_rwlock_t *rwlock TBX_UNUSED), (rwlock))
DEF_MARCEL_PMARCEL (int, rwlock_destroy, (lpt_rwlock_t *rwlock TBX_UNUSED), (rwlock),
{
	/* Nothing to be done.  For now.  */
	return 0;
})

/* Acquire read lock for RWLOCK.  */
DEF_PTHREAD (int, rwlock_rdlock, (lpt_rwlock_t *rwlock), (rwlock))
DEF_MARCEL_PMARCEL (int, rwlock_rdlock, (lpt_rwlock_t *rwlock), (rwlock),
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
				      == ma_self (), 0))
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
})

/* Try to acquire read lock for RWLOCK or return after specfied time.  */
DEF_PTHREAD (int, rwlock_timedrdlock,
	     (lpt_rwlock_t *rwlock, const struct timespec *abstime),
	     (rwlock, abstime))
DEF_MARCEL_PMARCEL (int, rwlock_timedrdlock,
		    (lpt_rwlock_t *rwlock, const struct timespec *abstime),
		    (rwlock, abstime),
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
				      == ma_self (), 0))
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

		int waitval = MA_RWLOCK_GENERATION_NUMBER (rwlock);

		/* Free the lock.  */
		lpt_lock_release (&rwlock->__data.__lock);

		/* Wait for the writer to finish.  */
		err = __lpt_lock_timed_wait (&rwlock->__data.__readers_wakeup, &rt,
					     &rwlock->__data.__lock, waitval);

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
})

/* Try to acquire write lock for RWLOCK or return after specfied time.	*/
DEF_PTHREAD (int, rwlock_timedwrlock,
	     (lpt_rwlock_t *rwlock, const struct timespec *abstime),
	     (rwlock, abstime))
DEF_MARCEL_PMARCEL (int, rwlock_timedwrlock,
		    (lpt_rwlock_t *rwlock, const struct timespec *abstime),
		    (rwlock, abstime),
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
			rwlock->__data.__writer = ma_self ();
			break;
		}

		/* Make sure we are not holding the rwlock as a writer.  This is
		   a deadlock situation we recognize and report.  */
		if (__builtin_expect (rwlock->__data.__writer
				      == ma_self (), 0))
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

		int waitval = MA_RWLOCK_GENERATION_NUMBER (rwlock);

		/* Free the lock.  */
		lpt_lock_release (&rwlock->__data.__lock);

		/* Wait for the writer or reader(s) to finish.  */
		err = __lpt_lock_timed_wait (&rwlock->__data.__writer_wakeup, &rt,
					     &rwlock->__data.__lock, waitval);

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
})

DEF_PTHREAD (int, rwlock_tryrdlock, (lpt_rwlock_t *rwlock), (rwlock))
DEF_MARCEL_PMARCEL (int, rwlock_tryrdlock, (lpt_rwlock_t *rwlock), (rwlock),
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
})

DEF_PTHREAD (int, rwlock_trywrlock, (lpt_rwlock_t *rwlock), (rwlock))
DEF_MARCEL_PMARCEL (int, rwlock_trywrlock, (lpt_rwlock_t *rwlock), (rwlock),
{
	int result = EBUSY;

	lpt_lock_acquire (&rwlock->__data.__lock);

	if (rwlock->__data.__writer == 0 && rwlock->__data.__nr_readers == 0)
	{
		rwlock->__data.__writer = ma_self ();
		result = 0;
	}

	lpt_lock_release (&rwlock->__data.__lock);

	return result;
})

/* Unlock RWLOCK.  */
DEF_PTHREAD (int, rwlock_unlock, (lpt_rwlock_t *rwlock), (rwlock))
DEF_MARCEL_PMARCEL (int, rwlock_unlock, (lpt_rwlock_t *rwlock), (rwlock),
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
			MA_RWLOCK_INCREMENT_GENERATION_NUMBER (rwlock);
			lpt_lock_release (&rwlock->__data.__lock);
			__lpt_lock_signal (&rwlock->__data.__writer_wakeup);
			return 0;
		}
		else if (rwlock->__data.__nr_readers_queued)
		{
			MA_RWLOCK_INCREMENT_GENERATION_NUMBER (rwlock);
			lpt_lock_release (&rwlock->__data.__lock);
			__lpt_lock_broadcast (&rwlock->__data.__readers_wakeup);
			return 0;
		}
	}
	lpt_lock_release (&rwlock->__data.__lock);
	return 0;
})


/* Acquire write lock for RWLOCK.  */
DEF_PTHREAD (int, rwlock_wrlock, (lpt_rwlock_t *rwlock),  (rwlock))
DEF_MARCEL_PMARCEL (int, rwlock_wrlock, (lpt_rwlock_t *rwlock),  (rwlock),
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
			rwlock->__data.__writer = ma_self ();
			break;
		}

		/* Make sure we are not holding the rwlock as a writer.  This is
		   a deadlock situation we recognize and report.  */
		if (__builtin_expect (rwlock->__data.__writer
				      == ma_self (), 0))
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
})
