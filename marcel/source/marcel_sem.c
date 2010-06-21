
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

#include "marcel.h"
#include <errno.h>

/* Portage libpthread:
 * -        sem_t fait 16 octets sur x86, 32 sur ia64,
 * - marcel_sem_t fait 16 octets sur x86, 24 sur ia64.
 *
 * donc �a tient.
 */

void marcel_sem_init(marcel_sem_t * s, int initial)
{
	/* The code assume that 'initial' is positive.
	 */
	MA_BUG_ON(initial < 0);

	s->value = initial;
	s->first = NULL;
	ma_spin_lock_init(&s->lock);
}

DEF_PMARCEL(int, sem_init, (pmarcel_sem_t *s, int pshared, unsigned int initial), (s,pshared, initial),
{
	MARCEL_LOG_IN();
	if (pshared) {
		fprintf(stderr, "PROCESS_PSHARED not supported\n");
		errno = ENOTSUP;
		MARCEL_LOG_RETURN(-1);
	}
	marcel_sem_init(s, initial);
	MARCEL_LOG_RETURN(0);
})
DEF_C(int, sem_init, (pmarcel_sem_t *s, int pshared, unsigned int initial), (s,pshared, initial))

DEF_PMARCEL(int, sem_destroy, (pmarcel_sem_t *s), (s),
{
	int res = 0;
	ma_spin_lock_bh(&s->lock);
	if (s->first) {
		errno = EBUSY;
		res = -1;
	}
	ma_spin_unlock_bh(&s->lock);
	return res;
})
DEF_C(int, sem_destroy, (pmarcel_sem_t *s), (s))

void marcel_sem_P(marcel_sem_t *s)
{
	semcell c;

	MARCEL_LOG_IN();
	MARCEL_LOG("semaphore %p\n", s);

	ma_spin_lock_bh(&s->lock);

	if (--(s->value) < 0) {
		c.next = NULL;
		c.blocked = tbx_true;
		c.task = ma_self();
		if (s->first == NULL)
			s->first = s->last = &c;
		else {
			s->last->next = &c;
			s->last = &c;
		}

		INTERRUPTIBLE_SLEEP_ON_CONDITION_RELEASING(c.blocked,
							   ma_spin_unlock_bh_no_resched(&s->lock), ma_spin_lock_bh(&s->lock));
	}
	ma_spin_unlock_bh(&s->lock);

	MARCEL_LOG_OUT();
}

DEF_PMARCEL(int, sem_wait, (pmarcel_sem_t *s), (s),
{
	semcell c;
	semcell **prev;

	MARCEL_LOG_IN();
	MARCEL_LOG("semaphore %p\n", s);

	ma_spin_lock_bh(&s->lock);

	if (--(s->value) < 0) {
		c.next = NULL;
		c.blocked = tbx_true;
		c.task = ma_self();
		if (s->first == NULL)
			s->first = s->last = &c;
		else {
			s->last->next = &c;
			s->last = &c;
		}

		MA_SET_INTERRUPTED(0);
		while (c.blocked) {
			ma_set_current_state(MA_TASK_INTERRUPTIBLE);
			ma_spin_unlock_bh_no_resched(&s->lock);
			ma_schedule();
			ma_spin_lock_bh(&s->lock);
			if (MA_GET_INTERRUPTED()
#ifdef MARCEL_DEVIATION_ENABLED
			    || c.task->canceled == MARCEL_IS_CANCELED
#endif
				) {
				for (prev = &s->first; *prev != &c;
				     prev = &(*prev)->next) ;
				*prev = c.next;
				s->value++;
				ma_spin_unlock_bh(&s->lock);
				errno = EINTR;
				MARCEL_LOG_RETURN(-1);
			}
		}
	}

	ma_spin_unlock_bh(&s->lock);

	MARCEL_LOG_RETURN(0);
})
DEF_C(int, sem_wait, (pmarcel_sem_t *s), (s))

int marcel_sem_try_P(marcel_sem_t *s)
{
	int result = 0;

	MARCEL_LOG_IN();
	MARCEL_LOG("semaphore %p\n", s);

	ma_spin_lock_bh(&s->lock);

	result = ((s->value - 1) < 0);

	if (result) {
		ma_spin_unlock_bh(&s->lock);
	} else {
		s->value--;
		ma_spin_unlock_bh(&s->lock);
	}

	MARCEL_LOG_RETURN(!result);
}

DEF_PMARCEL(int, sem_trywait, (pmarcel_sem_t *s), (s),
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
	unsigned long jiffies_timeout = 
		timeout ? (timeout * 1000 + marcel_gettimeslice() - 1) / marcel_gettimeslice() : 0;
	semcell c;
	semcell **prev;

	MARCEL_LOG_IN();
	MARCEL_LOG("semaphore %p\n", s);

	ma_spin_lock_bh(&s->lock);

	if (--(s->value) < 0) {
		if (jiffies_timeout == 0) {
			s->value++;
			ma_spin_unlock_bh(&s->lock);
			MARCEL_LOG_OUT();
			return 0;
		}
		c.next = NULL;
		c.blocked = tbx_true;
		c.task = ma_self();
		if (s->first == NULL)
			s->first = s->last = &c;
		else {
			s->last->next = &c;
			s->last = &c;
		}
		do {
			ma_set_current_state(MA_TASK_INTERRUPTIBLE);
			ma_spin_unlock_bh_no_resched(&s->lock);
			jiffies_timeout = ma_schedule_timeout(jiffies_timeout);
			ma_spin_lock_bh(&s->lock);
			if (c.blocked) {
				for (prev = &s->first; *prev != &c;
				     prev = &(*prev)->next) ;
				*prev = c.next;
				s->value++;
				ma_spin_unlock_bh(&s->lock);
				MARCEL_LOG_OUT();
				return 0;
			}
		} while (c.blocked);
	} else {
		ma_spin_unlock_bh(&s->lock);
	}

	MARCEL_LOG_OUT();
	return 1;
}

/*******************sem_timedwait**********************/
DEF_PMARCEL(int,sem_timedwait,(pmarcel_sem_t *__restrict s,
			       const struct timespec *__restrict abs_timeout),(sem,abs_timeout),
{
	struct timeval tv;
	long long timeout;	// usec
	unsigned long jiffies_timeout;
	semcell c;
	semcell **prev;

	MARCEL_LOG_IN();
	MARCEL_LOG("semaphore %p\n", s);

	if (abs_timeout->tv_nsec < 0 || abs_timeout->tv_nsec >= 1000000000) {
#ifdef MA__DEBUG
		fprintf(stderr,
			"pmarcel_semtimedwait : valeur nsec(%ld) invalide\n",
			abs_timeout->tv_nsec);
#endif
		errno = EINVAL;
		return -1;
	}

	gettimeofday(&tv, NULL);
	timeout = (abs_timeout->tv_sec - tv.tv_sec) * 1000000 + (abs_timeout->tv_nsec / 1000 - tv.tv_usec);
	if (timeout < 0)
		jiffies_timeout = 0;
	else
		jiffies_timeout = (timeout + marcel_gettimeslice() - 1) / marcel_gettimeslice();

	ma_spin_lock_bh(&s->lock);

	if (--(s->value) < 0) {
		if (jiffies_timeout == 0) {
			s->value++;
			ma_spin_unlock_bh(&s->lock);
			MARCEL_LOG_OUT();
			errno = ETIMEDOUT;
			return -1;
		}
		c.next = NULL;
		c.blocked = tbx_true;
		c.task = ma_self();
		if (s->first == NULL)
			s->first = s->last = &c;
		else {
			s->last->next = &c;
			s->last = &c;
		}
		MA_SET_INTERRUPTED(0);
		do {
			ma_set_current_state(MA_TASK_INTERRUPTIBLE);
			ma_spin_unlock_bh_no_resched(&s->lock);
			jiffies_timeout = ma_schedule_timeout(jiffies_timeout);
			ma_spin_lock_bh(&s->lock);
			if (jiffies_timeout == 0 || MA_GET_INTERRUPTED()) {
				for (prev = &s->first; *prev != &c;
				     prev = &(*prev)->next) ;
				*prev = c.next;
				s->value++;
				ma_spin_unlock_bh(&s->lock);
				MARCEL_LOG_OUT();
				errno = jiffies_timeout ? EINTR : ETIMEDOUT;
				return -1;
			}
		} while (c.blocked);
	} else {
		ma_spin_unlock_bh(&s->lock);
	}

	MARCEL_LOG_RETURN(0);
})
DEF_C(int,sem_timedwait,(sem_t *__restrict sem, const struct timespec *__restrict abs_timeout),(sem,abs_timeout))


void marcel_sem_V(marcel_sem_t *s)
{
	semcell *c;

	MARCEL_LOG_IN();
	MARCEL_LOG("semaphore %p\n", s);

	ma_spin_lock_bh(&s->lock);

	if (++(s->value) <= 0) {
		c = s->first;
		s->first = c->next;
		ma_spin_unlock_bh(&s->lock);
		c->blocked = tbx_false;
		ma_smp_wmb();
		ma_wake_up_thread(c->task);
	} else {
		ma_spin_unlock_bh(&s->lock);
	}

	MARCEL_LOG_OUT();
}

int marcel_sem_try_V(marcel_sem_t *s)
{
	semcell *c;

	MARCEL_LOG_IN();
	MARCEL_LOG("semaphore %p\n", s);

	if (!(ma_spin_trylock_bh(&s->lock)))
		MARCEL_LOG_RETURN(0);

	if (s->value < 0) {
		c = s->first;
		if (!(ma_wake_up_thread_async(c->task))) {
			ma_spin_unlock_bh(&s->lock);
			MARCEL_LOG_RETURN(0);
		}
		/* This is a bit late, but since we hold the lock, the woken up
		 * thread will spin waiting for us */
		c->blocked = tbx_false;
		s->first = c->next;
	}

	s->value++;
	ma_spin_unlock_bh(&s->lock);

	MARCEL_LOG_RETURN(1);
}

DEF_PMARCEL(int, sem_post, (pmarcel_sem_t *s), (s),
{
	// TODO: v�rifier tout overflow de value
	marcel_sem_V(s);
	return 0;
})
DEF_C(int, sem_post, (pmarcel_sem_t *s), (s))

DEF_MARCEL_PMARCEL(int, sem_getvalue, (pmarcel_sem_t * __restrict s, int * __restrict sval), (s, sval),
{
	ma_spin_lock_bh(&s->lock);
	*sval = s->value;
	ma_spin_unlock_bh(&s->lock);
	return 0;
})
DEF_C(int, sem_getvalue, (pmarcel_sem_t * __restrict s, int * __restrict sval), (s, sval))


/********************sem_close**************************/
DEF_PMARCEL(int,sem_close,(pmarcel_sem_t *sem TBX_UNUSED),(sem),
{
	MARCEL_LOG_IN();
	fprintf(stderr, "!! sem_close not implemented\n");
	errno = ENOTSUP;
	MARCEL_LOG_RETURN(-1);
})

DEF_C(int,sem_close,(sem_t *sem),(sem))
DEF___C(int,sem_close,(sem_t *sem),(sem))

/********************sem_open***************************/
DEF_PMARCEL(pmarcel_sem_t*,sem_open,(const char *name TBX_UNUSED, int flags TBX_UNUSED,...),(name,flags,...),
{
	MARCEL_LOG_IN();
	fprintf(stderr, "!! sem_open not implemented\n");
	errno = ENOTSUP;
	MARCEL_LOG_RETURN(PMARCEL_SEM_FAILED);
})

DEF_C(sem_t *,sem_open,(const char *name, int flags,...),(name,flags,...))
DEF___C(sem_t *,sem_open,(const char *name, int flags,...),(name,flags,...))

/*******************sem_unlink**************************/
DEF_PMARCEL(int,sem_unlink,(const char *name TBX_UNUSED),(name),
{
	MARCEL_LOG_IN();
	fprintf(stderr, "!! sem_unlink not implemented\n");
	errno = ENOTSUP;
	MARCEL_LOG_RETURN(-1);
})

DEF_C(int,sem_unlink,(const char *name),(name))
DEF___C(int,sem_unlink,(const char *name),(name))

