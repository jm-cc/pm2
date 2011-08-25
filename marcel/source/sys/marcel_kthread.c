
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


#if defined(MA__LWPS) && !defined(MARCEL_DONT_USE_POSIX_THREADS)

void marcel_kthread_join(marcel_kthread_t * pid)
{
	int cc;
	MARCEL_LOG_IN();

	do {
		cc = pthread_join(*pid, NULL);
	} while (cc == EINTR);
	MA_BUG_ON(cc);
	MARCEL_LOG_OUT();

}

void marcel_kthread_exit(void *retval)
{
	MARCEL_LOG_IN();
	pthread_exit(retval);
	MA_BUG();
}

marcel_kthread_t marcel_kthread_self(void)
{
	return pthread_self();
}

void marcel_kthread_kill(marcel_kthread_t pid, int sig)
{
	/** pthread_kill is not signal safe and take lock on
	    some systems (like FreeBSD) ... protect it */
	sigset_t set, oset;

	sigfillset(&set);
	marcel_kthread_sigmask(SIG_SETMASK, &set, &oset);
	int ret TBX_UNUSED = pthread_kill(pid, sig);
	MA_BUG_ON(ret);
	marcel_kthread_sigmask(SIG_SETMASK, &oset, NULL);
}

void marcel_kthread_mutex_init(marcel_kthread_mutex_t * lock)
{
	int ret TBX_UNUSED = pthread_mutex_init(lock, NULL);
	MA_BUG_ON(ret);
}

void marcel_kthread_mutex_lock(marcel_kthread_mutex_t * lock)
{
	int ret TBX_UNUSED = pthread_mutex_lock(lock);
	MA_BUG_ON(ret);
}

void marcel_kthread_mutex_unlock(marcel_kthread_mutex_t * lock)
{
	int ret TBX_UNUSED = pthread_mutex_unlock(lock);
	MA_BUG_ON(ret);
}

int marcel_kthread_mutex_trylock(marcel_kthread_mutex_t * lock)
{
	return pthread_mutex_trylock(lock);
}

void marcel_kthread_sem_init(marcel_kthread_sem_t * sem, int pshared, unsigned int value)
{
	int ret TBX_UNUSED = sem_init(sem, pshared, value);
	MA_BUG_ON(ret);
}

void marcel_kthread_sem_wait(marcel_kthread_sem_t * sem)
{
	while (sem_wait(sem) == -1 && errno == EINTR);
}

void marcel_kthread_sem_post(marcel_kthread_sem_t * sem)
{
	int ret TBX_UNUSED = sem_post(sem);
	MA_BUG_ON(ret);
}

int marcel_kthread_sem_trywait(marcel_kthread_sem_t * sem)
{
	return sem_trywait(sem);
}

void marcel_kthread_cond_init(marcel_kthread_cond_t * cond)
{
	int ret TBX_UNUSED = pthread_cond_init(cond, NULL);
	MA_BUG_ON(ret);
}

void marcel_kthread_cond_signal(marcel_kthread_cond_t * cond)
{
	int ret TBX_UNUSED = pthread_cond_signal(cond);
	MA_BUG_ON(ret);
}

void marcel_kthread_cond_broadcast(marcel_kthread_cond_t * cond)
{
	int ret TBX_UNUSED = pthread_cond_broadcast(cond);
	MA_BUG_ON(ret);
}

void marcel_kthread_cond_wait(marcel_kthread_cond_t * cond,
			      marcel_kthread_mutex_t * mutex)
{
	int ret TBX_UNUSED;
	while ((ret = pthread_cond_wait(cond, mutex)) == EINTR);
	MA_BUG_ON(ret);
}

void marcel_kthread_sigmask(int how, sigset_t * newmask, sigset_t * oldmask)
{
	int ret TBX_UNUSED;
	ret = pthread_sigmask(how, newmask, oldmask);
	MA_BUG_ON(ret);
}

#else

void marcel_kthread_sigmask(int how, sigset_t * newmask, sigset_t * oldmask)
{
	int ret TBX_UNUSED;
	ret = ma_sigprocmask(how, newmask, oldmask);
	MA_BUG_ON(ret);
}

#endif

