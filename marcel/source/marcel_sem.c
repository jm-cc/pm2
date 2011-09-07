
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


#include "tbx.h"
#include "marcel.h"
#include "marcel_pmarcel.h"
#include <errno.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/mman.h>


static tbx_htable_t shm_sems;
static ma_spinlock_t shm_sems_lock = MA_SPIN_LOCK_UNLOCKED;


/** helpers to (un)register semcell item from the waiting queue */
static __tbx_inline__ void ma_register_semcell(marcel_sem_t *s, semcell *c)
{
	c->task = ma_self();
	c->next = NULL;
	if (s->list) {
		c->prev = s->list->prev;
		c->prev->next = c;
		s->list->prev = c;
	} else {
		s->list = c;
		c->prev = c;
	}
}

static __tbx_inline__ marcel_t ma_unregister_semcell(marcel_sem_t *s, semcell *c)
{
	marcel_t task;

	if (tbx_likely(c == s->list)) {
		s->list = c->next;
		if (s->list)
			s->list->prev = c->prev;
	} else {
		c->prev->next = c->next;
		if (c->next)
			c->next->prev = c->prev;
		else
			s->list->prev = c->prev;
	}

	task = c->task;
	c->task = NULL;
	return task;
}


/* libpthread:
 * -        sem_t: 16 bytes on x86, 32 on ia64,
 * - marcel_sem_t: 16 bytes on x86, 24 on ia64.
 * Good !!!
 */
DEF_MARCEL(void, sem_init, (marcel_sem_t * s, int initial), (s, initial),
{
	s->value = initial;
	s->list = NULL;
	ma_spin_lock_init(&s->lock);
})
DEF_PMARCEL(int, sem_init, (pmarcel_sem_t *s, int pshared, unsigned int initial), (s,pshared, initial),
{
	MARCEL_LOG_IN();
	if (pshared) {
		MA_NOT_SUPPORTED("pshared semaphore");
		errno = ENOTSUP;
		MARCEL_LOG_RETURN(-1);
	}
	marcel_sem_init(s, initial);
	MARCEL_LOG_RETURN(0);
})
DEF_C(int, sem_init, (pmarcel_sem_t *s, int pshared, unsigned int initial), (s,pshared, initial))

DEF_MARCEL_PMARCEL(int, sem_destroy, (marcel_sem_t *s), (s),
{
	int res = 0;
	ma_spin_lock(&s->lock);
	if (s->list) {
		errno = EBUSY;
		res = -1;
	}
	ma_spin_unlock(&s->lock);
	return res;
})
DEF_C(int, sem_destroy, (pmarcel_sem_t *s), (s))

void marcel_sem_P(marcel_sem_t *s)
{
	semcell c;

	MARCEL_LOG_IN();
	MARCEL_LOG("semaphore %p\n", s);

	ma_spin_lock(&s->lock);
	if (tbx_unlikely(--(s->value) < 0)) {
		ma_register_semcell(s, &c);
		SLEEP_ON_CONDITION_RELEASING(ma_access_once(c.task),
					     ma_spin_unlock_no_resched(&s->lock), 
					     ma_spin_lock(&s->lock));
	}
	ma_spin_unlock(&s->lock);

	MARCEL_LOG_OUT();
}

DEF_MARCEL_PMARCEL(int, sem_wait, (marcel_sem_t *s), (s),
{
	int ret;
	semcell c;

	MARCEL_LOG_IN();
	MARCEL_LOG("semaphore %p\n", s);

	ret = 0;

	ma_spin_lock(&s->lock);
	if (--(s->value) < 0) {
		ma_register_semcell(s, &c);

		MA_SET_INTERRUPTED(0);
		INTERRUPTIBLE_SLEEP_ON_CONDITION_RELEASING(
			ma_access_once(c.task),
			MA_GET_INTERRUPTED()
#if defined(MARCEL_DEVIATION_ENABLED) && defined(MA__IFACE_PMARCEL)
			|| c.task->canceled == MARCEL_IS_CANCELED
#endif
			, ma_spin_unlock_no_resched(&s->lock), 
			ma_spin_lock(&s->lock));

		/** task was interrupted */
		if (tbx_unlikely(ma_access_once(c.task))) {
			ma_unregister_semcell(s, &c);
			s->value++;
			errno = EINTR;
			ret = -1;
		}
	}
	ma_spin_unlock(&s->lock);

	MARCEL_LOG_RETURN(ret);
})
DEF_C(int, sem_wait, (pmarcel_sem_t *s), (s))

int marcel_sem_try_P(marcel_sem_t *s)
{
	int ret;

	MARCEL_LOG_IN();
	MARCEL_LOG("semaphore %p\n", s);

	ma_spin_lock(&s->lock);
	if ((s->value - 1) >= 0) {
		s->value--;
		ret = 1;
	} else
		ret = 0;
	ma_spin_unlock(&s->lock);

	MARCEL_LOG_RETURN(ret);
}

DEF_MARCEL_PMARCEL(int, sem_trywait, (marcel_sem_t *s), (s),
{
	if (!marcel_sem_try_P(s)) {
		errno = EAGAIN;
		return -1;
	}
	return 0;
})
DEF_C(int, sem_trywait, (pmarcel_sem_t *s), (s))

/** \brief Attempt to do a timeout bounded P operation on the semaphore
 *
 *  \return 1 if the P operation succeeded before the timeout, 0
 *  otherwise
 */
int marcel_sem_timed_P(marcel_sem_t *s, unsigned long timeout)
{
	int ret;
	unsigned long jiffies_timeout;
	semcell c;

	MARCEL_LOG_IN();
	MARCEL_LOG("semaphore %p\n", s);

	ret = 1;
	jiffies_timeout = ma_jiffies_from_us(timeout * 1000);

	ma_spin_lock(&s->lock);
	if (tbx_unlikely(--(s->value) < 0)) {
		if (0 == jiffies_timeout) {
			s->value++;
			errno = ETIMEDOUT;
			ma_spin_unlock(&s->lock);
			MARCEL_LOG_RETURN(0);
		}

		ma_register_semcell(s, &c);

		INTERRUPTIBLE_TIMED_SLEEP_ON_CONDITION_RELEASING(
			ma_access_once(c.task),
			0,
			ma_spin_unlock_no_resched(&s->lock),
			ma_spin_lock(&s->lock),
			jiffies_timeout);

		/** task was interrupted */
		if (tbx_unlikely(ma_access_once(c.task))) {
			ma_unregister_semcell(s, &c);
			s->value++;
			ret = 0;
		}
	}
	ma_spin_unlock(&s->lock);

	MARCEL_LOG_RETURN(ret);
}

/*******************sem_timedwait**********************/
DEF_MARCEL_PMARCEL(int, sem_timedwait,(marcel_sem_t *__restrict s,
				      const struct timespec *__restrict abs_timeout),(s,abs_timeout),
{
	struct timeval tv;
	long long timeout;	// usec
	unsigned long jiffies_timeout;
	semcell c;
	int ret;

	MARCEL_LOG_IN();
	MARCEL_LOG("semaphore %p\n", s);

	if (abs_timeout->tv_nsec < 0 || abs_timeout->tv_nsec >= 1000000000) {
		MARCEL_LOG("pmarcel_semtimedwait : valeur nsec(%ld) invalide\n",
			   abs_timeout->tv_nsec);
		errno = EINVAL;
		return -1;
	}

	gettimeofday(&tv, NULL);
	timeout = (abs_timeout->tv_sec - tv.tv_sec) * 1000000 
		+ (abs_timeout->tv_nsec / 1000 - tv.tv_usec);
	if (timeout <= 0)
		jiffies_timeout = 0;
	else
		jiffies_timeout = ma_jiffies_from_us(timeout);

	ret = 0;
	ma_spin_lock(&s->lock);
	if (tbx_unlikely(--(s->value) < 0)) {
		if (jiffies_timeout == 0) {
			s->value++;
			errno = ETIMEDOUT;
			ma_spin_unlock(&s->lock);
			MARCEL_LOG_RETURN(-1);
		}

		ma_register_semcell(s, &c);

		MA_SET_INTERRUPTED(0);
		INTERRUPTIBLE_TIMED_SLEEP_ON_CONDITION_RELEASING(
			ma_access_once(c.task),
			MA_GET_INTERRUPTED(),
			ma_spin_unlock_no_resched(&s->lock),
			ma_spin_lock(&s->lock),
			jiffies_timeout);

		/** task was interrupted */
		if (tbx_unlikely(ma_access_once(c.task))) {
			ma_unregister_semcell(s, &c);
			s->value++;
			errno = MA_GET_INTERRUPTED() ? EINTR : ETIMEDOUT;
			ret = -1;
		}
	}
	ma_spin_unlock(&s->lock);

	MARCEL_LOG_RETURN(ret);
})
DEF_C(int,sem_timedwait,(sem_t *__restrict sem, const struct timespec *__restrict abs_timeout),(sem,abs_timeout))

void marcel_sem_V(marcel_sem_t *s)
{
	marcel_t task;

	MARCEL_LOG_IN();

	ma_spin_lock(&s->lock);
	if (++(s->value) <= 0) {
		task = ma_unregister_semcell(s, s->list);
		ma_wake_up_thread(task);
	}
	ma_spin_unlock(&s->lock);
	MARCEL_LOG_OUT();
}

DEF_MARCEL_PMARCEL(int, sem_post, (marcel_sem_t *s), (s),
{
	// TODO: vérifier tout overflow de value
	marcel_sem_V(s);
	return 0;
})
DEF_C(int, sem_post, (pmarcel_sem_t *s), (s))

DEF_MARCEL_PMARCEL(int, sem_getvalue, (marcel_sem_t * __restrict s, int * __restrict sval), (s, sval),
{
	ma_spin_lock(&s->lock);
	*sval = s->value;
	ma_spin_unlock(&s->lock);
	return 0;
})
DEF_C(int, sem_getvalue, (pmarcel_sem_t * __restrict s, int * __restrict sval), (s, sval))


/********************sem_close**************************/
DEF_MARCEL_PMARCEL(int,sem_close,(marcel_sem_t *sem),(sem),
{
	char *semname;

	MARCEL_LOG_IN();

	if (! sem) {
		errno = EINVAL;
		MARCEL_LOG_RETURN(-1);
	}

	/** unmap semaphore memory region if it is unused: update ref counter */
	semname = (char *)sem + sizeof(*sem) + sizeof(int);
	ma_spin_lock(&shm_sems_lock);
	if (0 == (-- *((int *)(sem + 1)))) {
		if (sem != tbx_htable_get(&shm_sems, semname))
			marcel_munmap(sem, sizeof(*sem) + sizeof(int) + strlen(semname));
	}
	ma_spin_unlock(&shm_sems_lock);

	MARCEL_LOG_RETURN(0);
})
DEF_C(int,sem_close,(sem_t *sem),(sem))
DEF___C(int,sem_close,(sem_t *sem),(sem))


/********************sem_open***************************/
DEF_MARCEL(marcel_sem_t*,sem_open,(const char *name, int flags,...),(name,flags,...),
{
	int fd;
	int mode;
	va_list args;
	unsigned int namesz;
	unsigned int value;
	marcel_sem_t *sem;

	MARCEL_LOG_IN();
	
	value = mode = 0;
	if ((flags & O_CREAT)) {
		/** retrieve init semaphore value and access rights **/
		va_start(args, flags);
		mode = va_arg(args, int);
		value = va_arg(args, unsigned int);
		va_end(args);

		if (SEM_VALUE_MAX < value) {
			errno = EINVAL;
			MARCEL_LOG_RETURN(MARCEL_SEM_FAILED);
		}
	}

	/** check semfile permissions & existence **/
	fd = shm_open(name, flags | O_RDWR | O_EXCL, mode);
	if (-1 == fd && !(flags & O_EXCL) && errno == EEXIST) {
		/* case of unmapped and linked semaphore */
		mode = -1; // mode == -1 + O_CREAT => ignore value, mode, init
		fd = shm_open(name, flags | O_RDWR, 0);
	}
	if (-1 == fd)
		MARCEL_LOG_RETURN(MARCEL_SEM_FAILED);

	/** check if there is a semaphore previously created with this key **/
	ma_spin_lock(&shm_sems_lock);
	sem = tbx_htable_get(&shm_sems, name);
	if (sem) {
		++ *((int *)(sem + 1));
		close(fd);
		ma_spin_unlock(&shm_sems_lock);
		MARCEL_LOG_RETURN(sem);
	}
			
	/** map file: | semaphore || ref counter || name_size || name | **/
	namesz = strlen(name);
	if ((flags & O_CREAT) && -1 != mode && 
	    ftruncate(fd, sizeof(*sem) + sizeof(int) + namesz)) {
		ma_spin_unlock(&shm_sems_lock);
		close(fd);
		errno = ENOSPC;
		MARCEL_LOG_RETURN(MARCEL_SEM_FAILED);
	}

#ifdef DARWIN_SYS
	/** NB: mmap should be MAP_PRIVATE because we doesn't support inter-process locks but on
	 *  Darwin systems, mmap return -1 & errno = EINVAL if MAP_PRIVATE is specified */
#  define SEM_MMAP_FLAG MAP_SHARED
#else
#  define SEM_MMAP_FLAG MAP_PRIVATE
#endif
	if ((void *)-1 == (sem = (marcel_sem_t *)marcel_mmap(NULL, sizeof(*sem) + sizeof(int) + namesz,
							     PROT_READ | PROT_WRITE, SEM_MMAP_FLAG,
							     fd, 0))) {
		ma_spin_unlock(&shm_sems_lock);
		close(fd);
		errno = ENOSPC;
		MARCEL_LOG_RETURN(MARCEL_SEM_FAILED);
	}

	/** initialize and register the semaphore **/
	close(fd);
	tbx_htable_add(&shm_sems, name, sem);
	*((int *)(sem + 1)) = 1;
	if ((flags & O_CREAT) && -1 != mode) {
		marcel_sem_init(sem, value);
		strcpy((char *)sem + sizeof(*sem) + sizeof(int), name);
	}
	ma_spin_unlock(&shm_sems_lock);

	MA_WARN_USER("semaphore: does not support process shared lock\n");
	MARCEL_LOG_RETURN(sem);
})
DEF_PMARCEL(marcel_sem_t*,sem_open,(const char *name, int flags,...),(name,flags,...),
{
	if ((flags & O_CREAT)) {
		int perms;
		unsigned int value;
		va_list args;

		va_start(args, flags);
		perms = va_arg(args, int);
		value = va_arg(args, unsigned int);
		va_end(args);

		return marcel_sem_open(name, flags, perms, value);
	}

	return marcel_sem_open(name, flags);
})
DEF_C(sem_t *,sem_open,(const char *name, int flags,...),(name,flags,...))
DEF___C(sem_t *,sem_open,(const char *name, int flags,...),(name,flags,...))

/*******************sem_unlink**************************/
DEF_MARCEL_PMARCEL(int,sem_unlink,(const char *name),(name),
{
	marcel_sem_t *sem;

	if (! name) {
		errno = ENOENT;
		return -1;
	}

	ma_spin_lock(&shm_sems_lock);
	sem = tbx_htable_get(&shm_sems, name);
	if (sem && (*((int *)(sem + 1)) - 1 == 0))
		tbx_htable_extract(&shm_sems, name);
	ma_spin_unlock(&shm_sems_lock);

	return shm_unlink(name);
})
DEF_C(int,sem_unlink,(const char *name),(name))
DEF___C(int,sem_unlink,(const char *name),(name))



static void sem_data_init(void)
{
	ma_spin_lock_init(&shm_sems_lock);
	tbx_htable_init(&shm_sems, 10);
}
__ma_initfunc(sem_data_init, MA_INIT_SEM_DATA, "Semaphore static data");
