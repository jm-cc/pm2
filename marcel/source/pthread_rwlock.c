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

#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
//VD: #include "internals.h"
//VD: #include "queue.h"
//VD: #include "spinlock.h"
//VD: #include "restart.h"
#include "marcel_rwlock.h" //VD:
#include "marcel_fastlock.h" //VD:
#include "pthread_queue.h" //VD:

#ifdef MA__LIBPTHREAD
#  ifdef PM2_DEV
#    warning FIXME: Need to use lpt_rwlock_t instead of Marcel s type
#  endif
#endif

#if 0 //VD:
/* Function called by marcel_cancel to remove the thread from
   waiting inside marcel_rwlock_timedrdlock or marcel_rwlock_timedwrlock. */

static int rwlock_rd_extricate_func(void *obj, marcel_descr th)
{
  marcel_rwlock_t *rwlock = obj;
  int did_remove = 0;

        __marcel_lock(&rwlock->__rw_lock, NULL);
  did_remove = remove_from_queue(&rwlock->__rw_read_waiting, th);
        __marcel_unlock(&rwlock->__rw_lock);

  return did_remove;
}

static int rwlock_wr_extricate_func(void *obj, marcel_descr th)
{
  marcel_rwlock_t *rwlock = obj;
  int did_remove = 0;

        __marcel_lock(&rwlock->__rw_lock, NULL);
  did_remove = remove_from_queue(&rwlock->__rw_write_waiting, th);
        __marcel_unlock(&rwlock->__rw_lock);

  return did_remove;
}
#endif //VD:

/*
 * Check whether the calling thread already owns one or more read locks on the
 * specified lock. If so, return a pointer to the read lock info structure
 * corresponding to that lock.
 */

static marcel_readlock_info *rwlock_is_in_list(marcel_descr self,
    marcel_rwlock_t * rwlock)
{
	marcel_readlock_info *info;

	for (info = THREAD_GETMEM(self, p_readlock_list); info != NULL;
	    info = info->pr_next) {
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

static marcel_readlock_info *rwlock_add_to_list(marcel_descr self,
    marcel_rwlock_t * rwlock)
{
	marcel_readlock_info *info = THREAD_GETMEM(self, p_readlock_free);

	if (info != NULL)
		THREAD_SETMEM(self, p_readlock_free, info->pr_next);
	else
		info = malloc(sizeof *info);

	if (info == NULL)
		return NULL;

	info->pr_lock_count = 1;
	info->pr_lock = rwlock;
	info->pr_next = THREAD_GETMEM(self, p_readlock_list);
	THREAD_SETMEM(self, p_readlock_list, info);

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

static marcel_readlock_info *rwlock_remove_from_list(marcel_descr self,
    marcel_rwlock_t * rwlock)
{
	marcel_readlock_info **pinfo;

	for (pinfo = &self->p_readlock_list; *pinfo != NULL;
	    pinfo = &(*pinfo)->pr_next) {
		if ((*pinfo)->pr_lock == rwlock) {
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

static int rwlock_can_rdlock(marcel_rwlock_t * rwlock, int have_lock_already)
{
	/* Can't readlock; it is write locked. */
	if (rwlock->__rw_writer != NULL)
		return 0;

	/* Lock prefers readers; get it. */
	if (rwlock->__rw_kind == MARCEL_RWLOCK_PREFER_READER_NP)
		return 1;

	/* Lock prefers writers, but none are waiting. */
	//sauf si la priorité du reader est plus grande que celle du writer !

	/* attention ! c'est bien la priorité minimale qui l'emporte ??? */
	if (queue_is_empty((marcel_t *) & rwlock->__rw_write_waiting))
		return 1;

	{
		marcel_t cthread = marcel_self();
		marcel_t waiting_writer = rwlock->__rw_write_waiting;
		int policy;
		struct marcel_sched_param paramr;
		struct marcel_sched_param paramw;
		marcel_getschedparam(cthread, &policy, &paramr);
		marcel_getschedparam(waiting_writer, &policy, &paramw);
		int prior = paramr.__sched_priority;
		int priow = paramw.__sched_priority;
		if (prior < priow)
			return 1;
	}

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
rwlock_have_already(marcel_descr * pself, marcel_rwlock_t * rwlock,
    marcel_readlock_info ** pexisting, int *pout_of_mem)
{
	marcel_readlock_info *existing = NULL;
	int out_of_mem = 0, have_lock_already = 0;
	marcel_descr self = *pself;

	if (rwlock->__rw_kind == MARCEL_RWLOCK_PREFER_WRITER_NP) {
		if (!self)
			*pself = self = __thread_self();

		existing = rwlock_is_in_list(self, rwlock);

		if (existing != NULL
		    || THREAD_GETMEM(self, p_untracked_readlock_count) > 0)
			have_lock_already = 1;
		else {
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
	LOG_IN();
	__marcel_init_lock(&rwlock->__rw_lock);
	rwlock->__rw_readers = 0;
	rwlock->__rw_writer = NULL;
	rwlock->__rw_read_waiting = NULL;
	rwlock->__rw_write_waiting = NULL;

	if (attr == NULL) {
		rwlock->__rw_kind = MARCEL_RWLOCK_DEFAULT_NP;
		rwlock->__rw_pshared = MARCEL_PROCESS_PRIVATE;
	} else {
		rwlock->__rw_kind = attr->__lockkind;
		rwlock->__rw_pshared = attr->__pshared;
	}
	LOG_RETURN(0);
})
DEF_PTHREAD(int, rwlock_init, (pthread_rwlock_t * __restrict rwlock,
			       __const pthread_rwlockattr_t * __restrict attr),
		(rwlock, attr))
DEF___PTHREAD(int, rwlock_init, (pthread_rwlock_t * __restrict rwlock,
			       __const pthread_rwlockattr_t * __restrict attr),
		(rwlock, attr))


DEF_MARCEL_POSIX(int,
		 rwlock_destroy, (marcel_rwlock_t *rwlock), (rwlock),
{
	LOG_IN();
	int readers;
	marcel_descr writer;

	__marcel_lock(&rwlock->__rw_lock, NULL);
	readers = rwlock->__rw_readers;
	writer = rwlock->__rw_writer;
	__marcel_unlock(&rwlock->__rw_lock);

	if (readers > 0 || writer != NULL)
		LOG_RETURN(EBUSY);

	LOG_RETURN(0);
})
DEF_PTHREAD(int, rwlock_destroy, (pthread_rwlock_t *rwlock), (rwlock))
DEF___PTHREAD(int, rwlock_destroy, (pthread_rwlock_t *rwlock), (rwlock))

DEF_MARCEL_POSIX(int,
		 rwlock_rdlock, (marcel_rwlock_t *rwlock), (rwlock),
{
	LOG_IN();

	marcel_descr self = NULL;
	marcel_readlock_info *existing;
	int out_of_mem;
	int have_lock_already;

	have_lock_already = rwlock_have_already(&self, rwlock,
	    &existing, &out_of_mem);

	if (self == NULL)
		self = __thread_self();

	for (;;) {
		__marcel_lock(&rwlock->__rw_lock, self);
		if (rwlock_can_rdlock(rwlock, have_lock_already))
			break;
		enqueue((marcel_t *) & rwlock->__rw_read_waiting, self);
		__marcel_unlock(&rwlock->__rw_lock);
		suspend(self);	/* This is not a cancellation point */
	}

	++rwlock->__rw_readers;
	__marcel_unlock(&rwlock->__rw_lock);

	if (have_lock_already || out_of_mem) {
		if (existing != NULL)
			++existing->pr_lock_count;
		else
			++self->p_untracked_readlock_count;
	}

	LOG_RETURN(0);
})
DEF_PTHREAD(int, rwlock_rdlock, (pthread_rwlock_t *rwlock), (rwlock))
DEF___PTHREAD(int, rwlock_rdlock, (pthread_rwlock_t *rwlock), (rwlock))


static int marcel_rwlock_suspend(const struct timespec *abstime)
{
        struct timeval now, tv;
        unsigned long int timeout;
        tv.tv_sec = abstime->tv_sec;
        tv.tv_usec = (abstime->tv_nsec + 999) / 1000;
        do {
                gettimeofday(&now, NULL);
                if(timercmp(&tv, &now, <=))
                        return ETIMEDOUT;

                timeout = (((tv.tv_sec*1e6 + tv.tv_usec) -
                        (now.tv_sec*1e6 + now.tv_usec)) + 999)/1000;

                /* This is not a cancellation point */
        } while(timed_suspend(MARCEL_SELF,timeout));
        return 0;
}


DEF_MARCEL_POSIX(int, rwlock_timedrdlock, (marcel_rwlock_t * __restrict rwlock,
				      const struct timespec * __restrict abstime),
		 (rwlock, abstime),
{
	LOG_IN();
	marcel_t self = NULL;
	marcel_readlock_info *existing;
	int out_of_mem;
	int have_lock_already;

	if (abstime->tv_nsec < 0 || abstime->tv_nsec >= 1000000000) {
		mdebug
		    ("(p)marcel_rwlock_timedrdlock : valeur nsec(%ld) invalide\n",
		    abstime->tv_nsec);
		LOG_RETURN(EINVAL);
	}

	have_lock_already = rwlock_have_already(&self, rwlock,
	    &existing, &out_of_mem);

	if (self == NULL)
		self = marcel_self();

	for (;;) {
		int ret;
		__marcel_lock(&rwlock->__rw_lock, self);
		if (rwlock_can_rdlock(rwlock, have_lock_already))
			break;
		enqueue(&rwlock->__rw_read_waiting, self);
		__marcel_unlock(&rwlock->__rw_lock);
		ret = marcel_rwlock_suspend(abstime);
		if (ret)
			LOG_RETURN(ETIMEDOUT);
	}

	++rwlock->__rw_readers;
	__marcel_unlock(&rwlock->__rw_lock);

	if (have_lock_already || out_of_mem) {
		if (existing != NULL)
			++existing->pr_lock_count;
		else
			++self->p_untracked_readlock_count;
	}
	LOG_RETURN(0);
})

DEF_PTHREAD(int, rwlock_timedrdlock, (pthread_rwlock_t *rwlock), (rwlock))
DEF___PTHREAD(int, rwlock_timedrdlock, (pthread_rwlock_t *rwlock), (rwlock))


DEF_MARCEL_POSIX(int,
		 rwlock_tryrdlock, (marcel_rwlock_t *rwlock), (rwlock),
{
	LOG_IN();
	marcel_descr self = __thread_self();
	marcel_readlock_info *existing;
	int out_of_mem;
	int have_lock_already;
	int retval = EBUSY;

	have_lock_already = rwlock_have_already(&self, rwlock,
	    &existing, &out_of_mem);

	__marcel_lock(&rwlock->__rw_lock, self);

	/* 0 is passed to here instead of have_lock_already.
	   This is to meet Single Unix Spec requirements:
	   if writers are waiting, marcel_rwlock_tryrdlock
	   does not acquire a read lock, even if the caller has
	   one or more read locks already. */

	if (rwlock_can_rdlock(rwlock, 0)) {
		++rwlock->__rw_readers;
		retval = 0;
	}

	__marcel_unlock(&rwlock->__rw_lock);

	if (retval == 0) {
		if (have_lock_already || out_of_mem) {
			if (existing != NULL)
				++existing->pr_lock_count;
			else
				++self->p_untracked_readlock_count;
		}
	}
	LOG_RETURN(retval);
})
DEF_PTHREAD(int, rwlock_tryrdlock, (pthread_rwlock_t *rwlock), (rwlock))
DEF___PTHREAD(int, rwlock_tryrdlock, (pthread_rwlock_t *rwlock), (rwlock))


DEF_MARCEL_POSIX(int,
		 rwlock_wrlock, (marcel_rwlock_t *rwlock), (rwlock),
{
	LOG_IN();
	marcel_descr self = __thread_self();
	/* TODO: EDEADLCK */
	while (1) {
		__marcel_lock(&rwlock->__rw_lock, self);
		if (rwlock->__rw_readers == 0 && rwlock->__rw_writer == NULL) {
			rwlock->__rw_writer = (marcel_descr) self;
			__marcel_unlock(&rwlock->__rw_lock);
			LOG_RETURN(0);
		}
		/* Suspend ourselves, then try again */
		enqueue((marcel_t *) & rwlock->__rw_write_waiting, self);
		__marcel_unlock(&rwlock->__rw_lock);
		suspend(self);	/* This is not a cancellation point */
	}
})
DEF_PTHREAD(int, rwlock_wrlock, (pthread_rwlock_t *rwlock), (rwlock))
DEF___PTHREAD(int, rwlock_wrlock, (pthread_rwlock_t *rwlock), (rwlock))



DEF_MARCEL_POSIX(int,rwlock_timedwrlock,(marcel_rwlock_t * __restrict rwlock,
  const struct timespec * __restrict abstime),(rwlock,abstime),
{
	LOG_IN();
	marcel_t self;

	if (abstime->tv_nsec < 0 || abstime->tv_nsec >= 1000000000) {
		mdebug
		    ("pthread_rwlock_timewrdlock : valeur nsec(%ld) invalide\n",
		    abstime->tv_nsec);
		LOG_RETURN(EINVAL);
	}
	self = marcel_self();

	while (1) {
		__marcel_lock(&rwlock->__rw_lock, self);

		if (rwlock->__rw_readers == 0 && rwlock->__rw_writer == NULL) {
			rwlock->__rw_writer = self;
			__marcel_unlock(&rwlock->__rw_lock);
			LOG_RETURN(0);
		}

		/* Suspend ourselves, then try again */
		enqueue(&rwlock->__rw_write_waiting, self);
		__marcel_unlock(&rwlock->__rw_lock);

		int ret = marcel_rwlock_suspend(abstime);
		if (ret)
			LOG_RETURN(ETIMEDOUT);
	}
})

DEF_PTHREAD(int, rwlock_timedwrlock, (pthread_rwlock_t *rwlock), (rwlock))
DEF___PTHREAD(int, rwlock_timedwrlock, (pthread_rwlock_t *rwlock), (rwlock))


DEF_MARCEL_POSIX(int,
		 rwlock_trywrlock, (marcel_rwlock_t *rwlock), (rwlock),
{
	LOG_IN();
	int result = EBUSY;

	__marcel_lock(&rwlock->__rw_lock, NULL);
	if (rwlock->__rw_readers == 0 && rwlock->__rw_writer == NULL) {
		rwlock->__rw_writer = (marcel_descr) __thread_self();
		result = 0;
	}
	__marcel_unlock(&rwlock->__rw_lock);

	LOG_RETURN(result);
})
DEF_PTHREAD(int, rwlock_trywrlock, (pthread_rwlock_t *rwlock), (rwlock))
DEF___PTHREAD(int, rwlock_trywrlock, (pthread_rwlock_t *rwlock), (rwlock))


DEF_MARCEL_POSIX(int,
		 rwlock_unlock, (marcel_rwlock_t *rwlock), (rwlock),
{
	LOG_IN();
	if ((!rwlock->__rw_writer) && (!rwlock->__rw_readers)) {
		mdebug("pthread_rwlock_unlock : rwlock not locked\n");
		LOG_RETURN(EINVAL);
	}

	marcel_descr torestart;
	marcel_descr th;
	__marcel_lock(&rwlock->__rw_lock, NULL);

	/* TODO : prendre celui qui à la plus grande priority avec pthread_getschedparam, en cas de priority égale, le writer l'emporte  */

	if (rwlock->__rw_writer != NULL) {
		/* Unlocking a write lock.  */
		if (rwlock->__rw_writer != (marcel_descr) __thread_self()) {
			__marcel_unlock(&rwlock->__rw_lock);
			LOG_RETURN(EPERM);
		}
		rwlock->__rw_writer = NULL;

		if ((rwlock->__rw_kind == MARCEL_RWLOCK_PREFER_READER_NP
			&& !queue_is_empty((marcel_t *) & rwlock->
			    __rw_read_waiting))
		    || (th =
			dequeue((marcel_t *) & rwlock->__rw_write_waiting)) ==
		    NULL) {
			/* Restart all waiting readers.  */
			torestart = (marcel_descr) rwlock->__rw_read_waiting;
			rwlock->__rw_read_waiting = NULL;
			__marcel_unlock(&rwlock->__rw_lock);
			while ((th = dequeue(&torestart)) != NULL) {
				restart(th);
			}
		} else {
			/* Restart one waiting writer.  */
			__marcel_unlock(&rwlock->__rw_lock);
			restart(th);
		}
	} else {
		/* Unlocking a read lock.  */
		if (rwlock->__rw_readers == 0) {
			__marcel_unlock(&rwlock->__rw_lock);
			LOG_RETURN(EPERM);
		}

		--rwlock->__rw_readers;
		if (rwlock->__rw_readers == 0)
			/* Restart one waiting writer, if any.  */
			th = dequeue((marcel_t *) & rwlock->__rw_write_waiting);
		else
			th = NULL;

		__marcel_unlock(&rwlock->__rw_lock);
		if (th != NULL)
			restart(th);

		/* Recursive lock fixup */
		if (rwlock->__rw_kind == MARCEL_RWLOCK_PREFER_WRITER_NP) {
			marcel_descr self = __thread_self();
			marcel_readlock_info *victim =
			    rwlock_remove_from_list(self, rwlock);

			if (victim != NULL) {
				if (victim->pr_lock_count == 0) {
					victim->pr_next =
					    THREAD_GETMEM(self,
					    p_readlock_free);
					THREAD_SETMEM(self, p_readlock_free,
					    victim);
				}
			} else {
				int val =
				    THREAD_GETMEM(self,
				    p_untracked_readlock_count);
				if (val > 0)
					THREAD_SETMEM(self,
					    p_untracked_readlock_count,
					    val - 1);
			}
		}
	}
	LOG_RETURN(0);
})

DEF_PTHREAD(int, rwlock_unlock, (pthread_rwlock_t *rwlock), (rwlock))
DEF___PTHREAD(int, rwlock_unlock, (pthread_rwlock_t *rwlock), (rwlock))



DEF_MARCEL_POSIX(int,
		 rwlockattr_init, (marcel_rwlockattr_t *attr), (attr),
{
	LOG_IN();
	attr->__lockkind = 0;
	attr->__pshared = MARCEL_PROCESS_PRIVATE;
	LOG_RETURN(0);
})

DEF_PTHREAD(int, rwlockattr_init, (pthread_rwlockattr_t *attr), (attr))

DEF_MARCEL_POSIX(int,
		 rwlockattr_destroy, (marcel_rwlockattr_t *attr TBX_UNUSED), (attr),
{
        LOG_IN();
        LOG_RETURN(0);
})
DEF_PTHREAD(int, rwlockattr_destroy, (pthread_rwlockattr_t *attr), (attr))
DEF___PTHREAD(int, rwlockattr_destroy, (pthread_rwlockattr_t *attr), (attr))


DEF_MARCEL_POSIX(int,
		 rwlockattr_getpshared,
		 (const marcel_rwlockattr_t * __restrict attr,
		  int * __restrict pshared),
		 (attr, pshared),
{
	LOG_IN();
	*pshared = attr->__pshared;
	LOG_RETURN(0);
})

DEF_PTHREAD(int, rwlockattr_getpshared,
		 (const pthread_rwlockattr_t * __restrict attr,
		  int * __restrict pshared),
		 (attr, pshared))

DEF_MARCEL_POSIX(int,
		 rwlockattr_setpshared,
		 (marcel_rwlockattr_t *attr, int pshared), (attr, pshared),
{
	LOG_IN();
	if (pshared != MARCEL_PROCESS_PRIVATE
	    && pshared != MARCEL_PROCESS_SHARED) {
		mdebug
		    ("pthread_rwlockattr_setpshared : valeur pshared(%d)  invalide\n",
		    pshared);
		LOG_RETURN(EINVAL);
	}
	/* For now it is not possible to shared a conditional variable.  */
	if (pshared != MARCEL_PROCESS_PRIVATE) {
		fprintf(stderr,
		    "pthread_rwlockattr : PROCESS_SHARED non supporté\n");
		LOG_RETURN(ENOTSUP);
	}
	attr->__pshared = pshared;
	LOG_RETURN(0);
})

DEF_PTHREAD(int, rwlockattr_setpshared,
		 (pthread_rwlockattr_t *attr, int pshared), (attr, pshared))

DEF_MARCEL_POSIX(int,
		 rwlockattr_getkind_np,
		 (const marcel_rwlockattr_t *attr, int *pref), (attr, pref),
{
	LOG_IN();
	*pref = attr->__lockkind;
	LOG_RETURN(0);
})

DEF_PTHREAD(int, rwlockattr_getkind_np,
		 (const pthread_rwlockattr_t *attr, int *pref), (attr, pref))

DEF_MARCEL_POSIX(int,
		 rwlockattr_setkind_np, (marcel_rwlockattr_t *attr, int pref),
		 (attr, pref),
{
	LOG_IN();
	if (pref != MARCEL_RWLOCK_PREFER_READER_NP
	    && pref != MARCEL_RWLOCK_PREFER_WRITER_NP
	    && pref != MARCEL_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP
	    && pref != MARCEL_RWLOCK_DEFAULT_NP) {
		mdebug
		    ("(p)marcel_rwlockattr_setkind_np : valeur pref(%d) invalide\n",
		    pref);
		LOG_RETURN(EINVAL);
	}
	attr->__lockkind = pref;
	LOG_RETURN(0);
})

DEF_PTHREAD(int, rwlockattr_setkind_np, (pthread_rwlockattr_t *attr, int pref),
		 (attr, pref))
