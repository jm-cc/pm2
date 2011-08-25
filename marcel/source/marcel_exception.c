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
#include "tbx.h"
#include <stdio.h>
#include <stdlib.h>


#ifdef MARCEL_EXCEPTIONS_ENABLED


/* Message definitions */
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


/* ================== Gestion des exceptions : ================== */
int _marcel_raise_exception(marcel_exception_t ex)
{
	marcel_t cur = ma_self();

	if (ex == NULL)
		ex = cur->cur_exception;

	if (cur->cur_excep_blk == NULL) {
		ma_preempt_disable();
		MA_WARN_USER("Unhandled exception %s in task %d on LWP(%d)"
			     "\nFILE : %s, LINE : %u\n",
			     ex, cur->number, ma_vpnum(MA_LWP_SELF), cur->exfile, cur->exline);
		MA_BUG();
		exit(1);
	} else {
		cur->cur_exception = ex;
		ma_ST_FLUSH_WINDOWS();
		marcel_ctx_longjmp(cur->cur_excep_blk->ctx, 1);
	}
}


#endif				/* MARCEL_EXCEPTIONS_ENABLED */
