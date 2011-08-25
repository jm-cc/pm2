
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

void marcel_kthread_create(marcel_kthread_t * pid, void *sp,
			   void *stack_base, marcel_kthread_func_t func, void *arg)
{
	pthread_attr_t attr;
	int err;
	size_t stack_size;

	MARCEL_LOG_IN();
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
	/* Contraintes d'alignement (16 pour l'IA64) */
	stack_base = (void *) (((unsigned long int) stack_base + 15) & (~0xF));
	sp = (void *) (((unsigned long int) sp) & (~0xF));
	/* On ne peut pas savoir combien la libpthread veut que cela soit
	 * aligné, donc on aligne au maximum */
	stack_size = 1 << (ma_fls((char *)sp - (char *)stack_base) - 1);
	if ((err = pthread_attr_setstackaddr(&attr, stack_base))
	    || (err = pthread_attr_setstacksize(&attr, stack_size)))
	{
		char *s;
		s = strerror(err);
		MA_WARN_USER("error, pthread_attr_setstack(%p, %p-%p (%#zx)):"
			     " (%d) %s\n", &attr, stack_base, sp, stack_size, err, s);
#ifdef PTHREAD_STACK_MIN
		MA_WARN_USER("PTHREAD_STACK_MIN: %#x\n", PTHREAD_STACK_MIN);
#endif
		abort();
	}
	err = pthread_create(pid, &attr, func, arg);
	MA_BUG_ON(err);
	MARCEL_LOG_OUT();
}

#endif /** LWPS && !DONT_USE_POSIX_THREADS **/
