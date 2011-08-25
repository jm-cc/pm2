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


/* When calling external libraries, we have to disable preemption, to make sure
 * that they will not see a kernel thread change, in case they take a kernel
 * thread mutex for instance.
 */
void marcel_extlib_protect(void)
{
	ma_some_thread_preemption_disable(ma_self());
}

void marcel_extlib_unprotect(void)
{
	/* Release preemption after releasing the mutex, in case we'd try to
	 * immediately schedule a thread that calls marcel_extlib_protect(),
	 * thus requiring two useless context switches. */
	ma_some_thread_preemption_enable(ma_self());
}
