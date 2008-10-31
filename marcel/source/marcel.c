
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
#include "marcel_alloc.h"
#include "sys/marcel_work.h"

#include "pm2_common.h"
#include "tbx.h"
#include "pm2_profile.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>

#ifdef UNICOS_SYS
#include <sys/mman.h>
#endif

/* Miscellaneous functions */

#ifdef RS6K_ARCH
int _jmg(int r)
{
	if (r != 0)
		ma_preempt_enable();
	return r;
}

#undef longjmp
void LONGJMP(jmp_buf buf, int val)
{
	static jmp_buf _buf;
	static int _val;

	ma_preempt_disable();
	memcpy(_buf, buf, sizeof(jmp_buf));
	_val = val;
#ifdef PM2_DEV
#warning set_sp() should not be directly used
#endif
	set_sp(SP_FIELD(buf) - 256);
	longjmp(_buf, _val);
}

#define longjmp(buf, v)	LONGJMP(buf, v)
#endif

#ifdef MARCEL_EXCEPTIONS_ENABLED
marcel_exception_t
    MARCEL_TASKING_ERROR =
    "MARCEL_TASKING_ERROR: A non-handled exception occurred in a task",
    MARCEL_DEADLOCK_ERROR =
    "MARCEL_DEADLOCK_ERROR: Global Blocking Situation Detected",
    MARCEL_STORAGE_ERROR =
    "MARCEL_STORAGE_ERROR: No space left on the heap", MARCEL_CONSTRAINT_ERROR =
    "MARCEL_CONSTRAINT_ERROR", MARCEL_PROGRAM_ERROR =
    "MARCEL_PROGRAM_ERROR", MARCEL_STACK_ERROR =
    "MARCEL_STACK_ERROR: Stack Overflow", MARCEL_TIME_OUT =
    "TIME OUT while being blocked on a semaphor", MARCEL_NOT_IMPLEMENTED =
    "MARCEL_NOT_IMPLEMENTED (sorry)", MARCEL_USE_ERROR =
    "MARCEL_USE_ERROR: Marcel was not compiled to enable this functionality";
#endif /* MARCEL_EXCEPTIONS_ENABLED */

#ifdef MA__DEBUG
/* C'EST ICI QU'IL EST PRATIQUE DE METTRE UN POINT D'ARRET
   LORSQUE L'ON VEUT EXECUTER PAS A PAS... */
void marcel_breakpoint()
{
	/* Lieu ideal ;-) pour un point d'arret */
}
#endif

/* returns the amount of mem between the base of the current thread stack and
   its stack pointer */
unsigned long marcel_usablestack(void)
{
	return (unsigned long) get_sp() -
	    (unsigned long) marcel_self()->stack_base;
}

/* ================== Gestion des exceptions : ================== */

#ifdef MARCEL_EXCEPTIONS_ENABLED
int _marcel_raise_exception(marcel_exception_t ex)
{
	marcel_t cur = marcel_self();

	if (ex == NULL)
		ex = cur->cur_exception;

	if (cur->cur_excep_blk == NULL) {
		ma_preempt_disable();
		fprintf(stderr, "\nUnhandled exception %s in task %d on LWP(%d)"
		    "\nFILE : %s, LINE : %d\n",
		    ex, cur->number, ma_vpnum(MA_LWP_SELF), cur->exfile,
		    cur->exline);
		*(int*)0 = -1;	/* To generate a core file */
		exit(1);
	} else {
		cur->cur_exception = ex;
		call_ST_FLUSH_WINDOWS();
		marcel_ctx_longjmp(cur->cur_excep_blk->ctx, 1);
	}
}
#endif /* MARCEL_EXCEPTIONS_ENABLED */

/* When calling external libraries, we have to disable preemption, to make sure
 * that they will not see a kernel thread change, in case they take a kernel
 * thread mutex for instance.
 *
 * When we do not use posix threads to create our kernel threads (i.e. we
 * directly use clone) and do not provide the libpthread interface, external
 * libraries aren't even aware that there is concurrency, so we have to protect
 * them ourselves.
 */
#if defined(MARCEL_DONT_USE_POSIX_THREADS) && !defined(MA__LIBPTHREAD)
static marcel_mutex_t ma_extlib_lock = MARCEL_MUTEX_INITIALIZER;
#endif
int marcel_extlib_protect(void)
{
#if defined(MARCEL_DONT_USE_POSIX_THREADS) && !defined(MA__LIBPTHREAD)
	marcel_mutex_lock(&ma_extlib_lock);
#endif
	ma_local_bh_disable();
	return 0;
}

int marcel_extlib_unprotect(void)
{
#if defined(MARCEL_DONT_USE_POSIX_THREADS) && !defined(MA__LIBPTHREAD)
	marcel_mutex_unlock(&ma_extlib_lock);
#endif
	/* Release preemption after releasing the mutex, in case we'd try to
	 * immediately schedule a thread that calls marcel_extlib_protect(),
	 * thus requiring two useless context switches. */
	ma_local_bh_enable();
	return 0;
}

void marcel_start_playing(void) {
	PROF_EVENT(fut_start_playing);
}

#if defined(LINUX_SYS) || defined(GNU_SYS)
static int rand_lwp_init(ma_lwp_t lwp) {
	srand48_r(ma_vpnum(lwp), &ma_per_lwp(random_buffer, lwp));
	return 0;
}

static int rand_lwp_start(ma_lwp_t lwp) {
	return 0;
}

MA_DEFINE_LWP_NOTIFIER_START(random_lwp, "Initialisation générateur aléatoire",
		rand_lwp_init, "Initialisation générateur",
		rand_lwp_start, "");

MA_LWP_NOTIFIER_CALL_UP_PREPARE(random_lwp, MA_INIT_MAIN_LWP);

long marcel_random(void) {
	long res;
	ma_local_bh_disable();
	ma_preempt_disable();
	lrand48_r(&__ma_get_lwp_var(random_buffer), &res);
	ma_preempt_enable_no_resched();
	ma_local_bh_enable();
	return res;
}
#endif
