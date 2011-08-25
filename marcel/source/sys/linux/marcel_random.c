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


static int rand_lwp_init(ma_lwp_t lwp)
{
	srand48_r(ma_vpnum(lwp), &ma_per_lwp(random_buffer, lwp));
	return 0;
}

static int rand_lwp_start(ma_lwp_t lwp TBX_UNUSED)
{
	return 0;
}

MA_DEFINE_LWP_NOTIFIER_START(random_lwp, "Initialisation generateur aleatoire",
			     rand_lwp_init, "Initialisation generateur",
			     rand_lwp_start, "");
MA_LWP_NOTIFIER_CALL_UP_PREPARE(random_lwp, MA_INIT_MAIN_LWP);


long marcel_random(void)
{
	long res;
	ma_local_bh_disable();
	ma_preempt_disable();
	lrand48_r(&__ma_get_lwp_var(random_buffer), &res);
	ma_preempt_enable_no_resched();
	ma_local_bh_enable();
	return res;
}

