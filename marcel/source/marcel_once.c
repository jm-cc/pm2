
/* This file has been autogenerated from source/nptl_mutex.c.m4 */
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
#ifdef MARCEL_ONCE_ENABLED
#  include "marcel.h"
#  include <errno.h>
#  include "marcel_fastlock.h"
#  include <limits.h>

// XXX Vince, à corriger. On n'a pas lpt_mutex sur toutes les archis pour l'instant, donc j'ai mis un pmarcel_mutex pour que ça marchouille.
static marcel_mutex_t once_masterlock = MARCEL_MUTEX_INITIALIZER;
static marcel_cond_t once_finished = MARCEL_COND_INITIALIZER;

enum { NEVER = 0, IN_PROGRESS = 1, DONE = 2 };

#ifdef MARCEL_DEVIATION_ENABLED
/* If a thread is canceled while calling the init_routine out of
   marcel once, this handler will reset the once_control variable
   to the NEVER state. */

static void marcel_once_cancelhandler(void *arg)
{
	marcel_once_t *once_control = arg;

	marcel_mutex_lock(&once_masterlock);
	*once_control = NEVER;
	marcel_mutex_unlock(&once_masterlock);
	marcel_cond_broadcast(&once_finished);
}
#endif				/* MARCEL_DEVIATION_ENABLED */
int marcel_once(marcel_once_t * once_control, void (*init_routine) (void))
{
	MARCEL_LOG_IN();
	/* flag for doing the condition broadcast outside of mutex */
	int state_changed;

	/* Test without locking first for speed */
	if (*once_control == DONE) {
		ma_smp_rmb();
		MARCEL_LOG_RETURN(0);
	}
	/* Lock and test again */

	state_changed = 0;

	marcel_mutex_lock(&once_masterlock);

	/* If this object was left in an IN_PROGRESS state in a parent
	   process (indicated by stale generation field), reset it to NEVER. */
	if ((*once_control & 3) == IN_PROGRESS
	    && (*once_control & ~3) != (long) ma_fork_generation)
		*once_control = NEVER;

	/* If init_routine is being called from another routine, wait until
	   it completes. */
	while ((*once_control & 3) == IN_PROGRESS) {
		marcel_cond_wait(&once_finished, &once_masterlock);
	}
	/* Here *once_control is stable and either NEVER or DONE. */
	if (*once_control == NEVER) {
		*once_control = IN_PROGRESS | ma_fork_generation;
		marcel_mutex_unlock(&once_masterlock);
#ifdef MARCEL_DEVIATION_ENABLED
		marcel_cleanup_push(marcel_once_cancelhandler, once_control);
#endif				/* MARCEL_DEVIATION_ENABLED */
		init_routine();
#ifdef MARCEL_DEVIATION_ENABLED
		marcel_cleanup_pop(tbx_false);
#endif				/* MARCEL_DEVIATION_ENABLED */
		marcel_mutex_lock(&once_masterlock);
		ma_smp_wmb();
		*once_control = DONE;
		state_changed = 1;
	}
	marcel_mutex_unlock(&once_masterlock);

	if (state_changed)
		marcel_cond_broadcast(&once_finished);

	MARCEL_LOG_RETURN(0);
}

#  ifdef MA__IFACE_PMARCEL
int pmarcel_once(pmarcel_once_t * once_control, void (*init_routine) (void))
{
	MARCEL_LOG_IN();
	/* flag for doing the condition broadcast outside of mutex */
	int state_changed;

	/* Test without locking first for speed */
	if (*once_control == DONE) {
		ma_smp_rmb();
		MARCEL_LOG_RETURN(0);
	}
	/* Lock and test again */

	state_changed = 0;

	marcel_mutex_lock(&once_masterlock);

	/* If this object was left in an IN_PROGRESS state in a parent
	   process (indicated by stale generation field), reset it to NEVER. */
	if ((*once_control & 3) == IN_PROGRESS
	    && (*once_control & ~3) != (long) ma_fork_generation)
		*once_control = NEVER;

	/* If init_routine is being called from another routine, wait until
	   it completes. */
	while ((*once_control & 3) == IN_PROGRESS) {
		marcel_cond_wait(&once_finished, &once_masterlock);
	}
	/* Here *once_control is stable and either NEVER or DONE. */
	if (*once_control == NEVER) {
		*once_control = IN_PROGRESS | ma_fork_generation;
		marcel_mutex_unlock(&once_masterlock);
#ifdef MARCEL_DEVIATION_ENABLED
		marcel_cleanup_push(marcel_once_cancelhandler, once_control);
#endif				/* MARCEL_DEVIATION_ENABLED */
		init_routine();
#ifdef MARCEL_DEVIATION_ENABLED
		marcel_cleanup_pop(tbx_false);
#endif				/* MARCEL_DEVIATION_ENABLED */
		marcel_mutex_lock(&once_masterlock);
		ma_smp_wmb();
		*once_control = DONE;
		state_changed = 1;
	}
	marcel_mutex_unlock(&once_masterlock);

	if (state_changed)
		marcel_cond_broadcast(&once_finished);

	MARCEL_LOG_RETURN(0);
}
#  endif

#endif				/* MARCEL_ONCE_ENABLED */
