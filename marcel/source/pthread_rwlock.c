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


#include "marcel.h"
#include "asm/nptl_types.h"
#include "errno.h"
#include "marcel_pmarcel.h"


/* Attributes.  */

/* We use `struct marcel_rwlockattr' as the internal structure that underlies
   `lpt_rwlockattr_t'.  Make sure they are compatible.  */

MA_VERIFY (sizeof (struct marcel_rwlockattr) <= sizeof (lpt_rwlockattr_t));
MA_VERIFY (__alignof (struct marcel_rwlockattr) <= sizeof (lpt_rwlockattr_t));

static const struct marcel_rwlockattr default_attr = {
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
	    && pshared != MARCEL_PROCESS_PRIVATE)
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


DEF_PTHREAD (int, rwlock_init,
	     (lpt_rwlock_t *rwlock, const marcel_rwlockattr_t *attr),
	     (rwlock, attr))
DEF_MARCEL_PMARCEL (int, rwlock_init,
		    (lpt_rwlock_t *rwlock, const marcel_rwlockattr_t *attr),
		    (rwlock, attr),
{
	const struct _lpt_fastlock initlock = MA_LPT_FASTLOCK_UNLOCKED;
	const struct marcel_rwlockattr *iattr;

	rwlock->__data.__nr_readers = 0;
	rwlock->__data.__nr_readers_queued = 0;
	rwlock->__data.__nr_writers_queued = 0;
	rwlock->__data.__writer = 0;
	rwlock->__data.__lock = initlock;
	rwlock->__data.__readers_wakeup = initlock;
	rwlock->__data.__writer_wakeup = initlock;

	if (attr)
		iattr = attr;
	else
		iattr = &default_attr;

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
	__lpt_destroy_lock(&rwlock->__data.__readers_wakeup);
	__lpt_destroy_lock(&rwlock->__data.__writer_wakeup);
	__lpt_destroy_lock(&rwlock->__data.__lock);

	return 0;
})


static tbx_bool_t ma_rwlock_unlock_reader(lpt_rwlock_t *rwlock)
{
	/* Get the rwlock if there is no writer...  */
	if (rwlock->__data.__writer == 0) {
#ifdef MARCEL_CHECK_PRIO_ON_LOCKS
		blockcell *writer;
		
		/* If there is a writer, check prio */
		if (rwlock->__data.__nr_writers_queued) {
			writer = MA_LPT_FASTLOCK_WAIT_LIST(&rwlock->__data.__writer_wakeup);
			if (SELF_GETMEM(as_entity.prio) >= writer->task->as_entity.prio)
				return tbx_false;
		}
		return tbx_true;
#else
		/* ...and if either no writer is waiting or we prefer readers.  */
		if (!rwlock->__data.__nr_writers_queued || MARCEL_RWLOCK_PREFER_READER_P (rwlock))
			return tbx_true;
#endif /* MARCEL_CHECK_PRIO_ON_LOCKS */
	}

	return tbx_false;
}

/* Acquire read lock for RWLOCK.  */
DEF_PTHREAD (int, rwlock_rdlock, (lpt_rwlock_t *rwlock), (rwlock))
DEF_MARCEL_PMARCEL (int, rwlock_rdlock, (lpt_rwlock_t *rwlock), (rwlock),
{
	int result;

	/* Make sure we are along.  */
	__lpt_lock(&rwlock->__data.__lock, ma_self());
	result = 0;
	do {
		/* check if the reader can take the lock */
		if (tbx_true == ma_rwlock_unlock_reader(rwlock)) {
			/* Increment the reader counter.  Avoid overflow.  */
			rwlock->__data.__nr_readers++;
#ifdef MA__DEBUG
			if (tbx_unlikely(0 == rwlock->__data.__nr_readers)) {
				/* Overflow on number of readers.	 */
				--rwlock->__data.__nr_readers;
				result = EAGAIN;
			}
#endif
       			break;
		}

		/* Make sure we are not holding the rwlock as a writer.  This is
		   a deadlock situation we recognize and report.  */
		if (tbx_unlikely(rwlock->__data.__writer == ma_self())) {
			result = EDEADLK;
			break;
		}

		/* Remember that we are a reader.  */
		rwlock->__data.__nr_readers_queued++;
#ifdef MA__DEBUG
		if (tbx_unlikely(rwlock->__data.__nr_readers_queued == 0)) {
			/* Overflow on number of queued readers.  */
			--rwlock->__data.__nr_readers_queued;
			result = EAGAIN;
			break;
		}
#endif

		/* Free the lock.  */
		lpt_fastlock_acquire(&rwlock->__data.__readers_wakeup);
		__lpt_unlock(&rwlock->__data.__lock);

		/* Wait for the writer to finish.  */
		__lpt_lock_wait(&rwlock->__data.__readers_wakeup, ma_self(), 0);

		/* Get the lock.  */
		lpt_fastlock_release(&rwlock->__data.__readers_wakeup);
		__lpt_lock(&rwlock->__data.__lock, ma_self());
	       
		--rwlock->__data.__nr_readers_queued;
	} while (1);

	/* We are done, free the lock.  */
	__lpt_unlock(&rwlock->__data.__lock);

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
	int result;

	/* Make sure we are alone.  */
	__lpt_lock(&rwlock->__data.__lock, ma_self());

	result = 0;
	while (1)
	{
		/* check if the reader can take the lock */
		if (tbx_true == ma_rwlock_unlock_reader(rwlock)) {
			++rwlock->__data.__nr_readers;
#ifdef MA__DEBUG
			/* Increment the reader counter.  Avoid overflow.  */
			if (__builtin_expect (rwlock->__data.__nr_readers == 0, 0)) {
				/* Overflow on number of readers.	 */
				--rwlock->__data.__nr_readers;
				result = EAGAIN;
			}
#endif		
			break;
		}

		/* Make sure we are not holding the rwlock as a writer.  This is
		   a deadlock situation we recognize and report.  */
		if (__builtin_expect (rwlock->__data.__writer == ma_self (), 0)) {
			result = EDEADLK;
			break;
		}

		/* Make sure the passed in timeout value is valid.  Ideally this
		   test would be executed once.  But since it must not be
		   performed if we would not block at all simply moving the test
		   to the front is no option.  Replicating all the code is
		   costly while this test is not.  */
		if (__builtin_expect (abstime->tv_nsec >= 1000000000 || abstime->tv_nsec < 0, 0)) {
			result = EINVAL;
			break;
		}

		/* Get the current time.  So far we support only one clock.  */
		struct timeval tv;
		gettimeofday (&tv, NULL);

		/* Convert the absolute timeout value to a relative timeout.  */
		struct timespec rt;
		rt.tv_sec = abstime->tv_sec - tv.tv_sec;
		rt.tv_nsec = abstime->tv_nsec - tv.tv_usec * 1000;
		if (rt.tv_nsec < 0) {
			rt.tv_nsec += 1000000000;
			--rt.tv_sec;
		}
		/* Did we already time out?  */
		if (rt.tv_sec < 0) {
			/* Yep, return with an appropriate error.  */
			result = ETIMEDOUT;
			break;
		}

		/* Remember that we are a reader.  */
		rwlock->__data.__nr_readers_queued++;
#ifdef MA__DEBUG
		if (rwlock->__data.__nr_readers_queued == 0) {
			/* Overflow on number of queued readers.  */
			--rwlock->__data.__nr_readers_queued;
			result = EAGAIN;
			break;
		}
#endif

		/* Free the lock.  */
		lpt_fastlock_acquire(&rwlock->__data.__readers_wakeup);
		__lpt_unlock(&rwlock->__data.__lock);

		/* Wait for the writer to finish.  */
		result = __lpt_lock_timed_wait (&rwlock->__data.__readers_wakeup, ma_self(), 
						MA_TIMESPEC_TO_USEC(&rt), 0);

		/* Get the lock.  */
		lpt_fastlock_release(&rwlock->__data.__readers_wakeup);
		__lpt_lock(&rwlock->__data.__lock, ma_self());

		--rwlock->__data.__nr_readers_queued;

		/* Did the futex call time out?  */
		if (result == ETIMEDOUT)
			break;
	}

	/* We are done, free the lock.  */
	__lpt_unlock(&rwlock->__data.__lock);
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
	__lpt_lock(&rwlock->__data.__lock, ma_self());

	while (1) {
		/* Get the rwlock if there is no writer and no reader.  */
		if (rwlock->__data.__writer == 0 && rwlock->__data.__nr_readers == 0) {
			/* Mark self as writer.  */
			rwlock->__data.__writer = ma_self ();
			break;
		}

		/* Make sure we are not holding the rwlock as a writer.  This is
		   a deadlock situation we recognize and report.  */
		if (__builtin_expect (rwlock->__data.__writer == ma_self (), 0)) {
			result = EDEADLK;
			break;
		}

		/* Make sure the passed in timeout value is valid.  Ideally this
		   test would be executed once.  But since it must not be
		   performed if we would not block at all simply moving the test
		   to the front is no option.  Replicating all the code is
		   costly while this test is not.  */
		if (__builtin_expect (abstime->tv_nsec >= 1000000000 || abstime->tv_nsec < 0, 0)) {
			result = EINVAL;
			break;
		}

		/* Get the current time.  So far we support only one clock.  */
		struct timeval tv;
		gettimeofday (&tv, NULL);

		/* Convert the absolute timeout value to a relative timeout.  */
		struct timespec rt;
		rt.tv_sec = abstime->tv_sec - tv.tv_sec;
		rt.tv_nsec = abstime->tv_nsec - tv.tv_usec * 1000;
		if (rt.tv_nsec < 0) {
			rt.tv_nsec += 1000000000;
			--rt.tv_sec;
		}
		/* Did we already time out?  */
		if (rt.tv_sec < 0) {
			result = ETIMEDOUT;
			break;
		}

		/* Remember that we are a writer.  */
		rwlock->__data.__nr_writers_queued++;
#ifdef MA__DEBUG
		if (rwlock->__data.__nr_writers_queued == 0) {
			/* Overflow on number of queued writers.  */
			--rwlock->__data.__nr_writers_queued;
			result = EAGAIN;
			break;
		}
#endif

		/* Free the lock.  */
		lpt_fastlock_acquire(&rwlock->__data.__writer_wakeup);
		__lpt_unlock(&rwlock->__data.__lock);

		/* Wait for the writer or reader(s) to finish.  */
		result = __lpt_lock_timed_wait (&rwlock->__data.__writer_wakeup, ma_self(), 
						MA_TIMESPEC_TO_USEC(&rt), 0);

		/* Get the lock.  */
		lpt_fastlock_release(&rwlock->__data.__writer_wakeup);
		__lpt_lock(&rwlock->__data.__lock, ma_self());

		/* To start over again, remove the thread from the writer list.  */
		--rwlock->__data.__nr_writers_queued;

		/* Did the futex call time out?  */
		if (result == ETIMEDOUT)
			break;
	}

	/* We are done, free the lock.  */
	__lpt_unlock(&rwlock->__data.__lock);

	return result;
})

DEF_PTHREAD (int, rwlock_tryrdlock, (lpt_rwlock_t *rwlock), (rwlock))
DEF_MARCEL_PMARCEL (int, rwlock_tryrdlock, (lpt_rwlock_t *rwlock), (rwlock),
{
	int result = EBUSY;

	__lpt_lock(&rwlock->__data.__lock, ma_self());

	/* check if the reader can take the lock */
	if (tbx_true == ma_rwlock_unlock_reader(rwlock)) {
		/* Increment the reader counter.  Avoid overflow.  */
		rwlock->__data.__nr_readers ++;
#ifdef MA__DEBUG
		if (__builtin_expect (rwlock->__data.__nr_readers == 0, 0)) {
			/* Overflow on number of readers.	 */
			--rwlock->__data.__nr_readers;
			result = EAGAIN;
		}
#endif
		
		result = 0;
	}

	__lpt_unlock(&rwlock->__data.__lock);

	return result;
})

DEF_PTHREAD (int, rwlock_trywrlock, (lpt_rwlock_t *rwlock), (rwlock))
DEF_MARCEL_PMARCEL (int, rwlock_trywrlock, (lpt_rwlock_t *rwlock), (rwlock),
{
	int result = EBUSY;

	__lpt_lock(&rwlock->__data.__lock, ma_self());
	
	if (rwlock->__data.__writer == 0 && rwlock->__data.__nr_readers == 0) {
		rwlock->__data.__writer = ma_self ();
		result = 0;
	}

	__lpt_unlock(&rwlock->__data.__lock);

	return result;
})

/* Unlock RWLOCK.  */
DEF_PTHREAD (int, rwlock_unlock, (lpt_rwlock_t *rwlock), (rwlock))
DEF_MARCEL_PMARCEL (int, rwlock_unlock, (lpt_rwlock_t *rwlock), (rwlock),
{
	__lpt_lock(&rwlock->__data.__lock, ma_self());

	if (rwlock->__data.__writer)
		rwlock->__data.__writer = 0;
	else
		--rwlock->__data.__nr_readers;

	if (rwlock->__data.__nr_readers == 0) {
#ifdef MARCEL_CHECK_PRIO_ON_LOCKS
		/* check queued writers and queued readers priority */
		if (rwlock->__data.__nr_writers_queued && rwlock->__data.__nr_readers_queued) {
			blockcell *writer;
			blockcell *reader;

			writer = MA_LPT_FASTLOCK_WAIT_LIST(&rwlock->__data.__writer_wakeup);
			reader = MA_LPT_FASTLOCK_WAIT_LIST(&rwlock->__data.__readers_wakeup);
			if (reader->task->as_entity.prio < writer->task->as_entity.prio) {
				lpt_fastlock_acquire(&rwlock->__data.__readers_wakeup);
				__lpt_unlock(&rwlock->__data.__lock);
				__lpt_lock_signal(&rwlock->__data.__readers_wakeup);
				lpt_fastlock_release(&rwlock->__data.__readers_wakeup);
				return 0;
			}
		}
#endif /* MARCEL_CHECK_PRIO_ON_LOCKS */
		
		if (rwlock->__data.__nr_writers_queued) {
			lpt_fastlock_acquire(&rwlock->__data.__writer_wakeup);
			__lpt_unlock(&rwlock->__data.__lock);
			__lpt_lock_signal (&rwlock->__data.__writer_wakeup);
			lpt_fastlock_release(&rwlock->__data.__writer_wakeup);
			return 0;
		}
		if (rwlock->__data.__nr_readers_queued) {
			lpt_fastlock_acquire(&rwlock->__data.__readers_wakeup);
			__lpt_unlock(&rwlock->__data.__lock);
			__lpt_lock_broadcast (&rwlock->__data.__readers_wakeup, -1);
			lpt_fastlock_release(&rwlock->__data.__readers_wakeup);
			return 0;
		}
	}
	__lpt_unlock(&rwlock->__data.__lock);

	return 0;
})


/* Acquire write lock for RWLOCK.  */
DEF_PTHREAD (int, rwlock_wrlock, (lpt_rwlock_t *rwlock),  (rwlock))
DEF_MARCEL_PMARCEL (int, rwlock_wrlock, (lpt_rwlock_t *rwlock),  (rwlock),
{
	int result = 0;

	/* Make sure we are along.  */
	__lpt_lock(&rwlock->__data.__lock, ma_self());

	while (1)
	{
		/* Get the rwlock if there is no writer and no reader.  */
		if (rwlock->__data.__writer == 0 && rwlock->__data.__nr_readers == 0) {
			/* Mark self as writer.  */
			rwlock->__data.__writer = ma_self ();
			break;
		}

		/* Make sure we are not holding the rwlock as a writer.  This is
		   a deadlock situation we recognize and report.  */
		if (__builtin_expect (rwlock->__data.__writer == ma_self (), 0)) {
			result = EDEADLK;
			break;
		}

		/* Remember that we are a writer.  */
		++rwlock->__data.__nr_writers_queued;
#ifdef MA__DEBUG
		if (rwlock->__data.__nr_writers_queued == 0) {
			/* Overflow on number of queued writers.  */
			--rwlock->__data.__nr_writers_queued;
			result = EAGAIN;
			break;
		}
#endif

		/* Free the lock.  */
		lpt_fastlock_acquire(&rwlock->__data.__writer_wakeup);
		__lpt_unlock(&rwlock->__data.__lock);

		/* Wait for the writer or reader(s) to finish.  */
		__lpt_lock_wait(&rwlock->__data.__writer_wakeup, ma_self(), 0);

		/* Get the lock.  */
		lpt_fastlock_release(&rwlock->__data.__writer_wakeup);
		__lpt_lock(&rwlock->__data.__lock, ma_self());

		/* To start over again, remove the thread from the writer list.  */
		--rwlock->__data.__nr_writers_queued;
	}

	/* We are done, free the lock.  */
	__lpt_unlock(&rwlock->__data.__lock);

	return result;
})
