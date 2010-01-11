
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

/* disable.c */

#include "marcel.h"

#define N 5
#define DELAY 5

int marcel_main(int argc, char *argv[])
{
	int i;
	marcel_vpset_t vpset = MARCEL_VPSET_ZERO;

	marcel_init(&argc, argv);

	for (i = 2; i < marcel_nbvps(); i++)
		if (marcel_random() % 2)
			marcel_vpset_set(&vpset, i);

	/* Force at least some enabled and disabled */
#ifdef FIXME
	/* For now VP0 needs to be kept enabled since it's that one that
	 * increases Marcel's notion of time */
	marcel_vpset_set(&vpset, 0);
	marcel_vpset_clr(&vpset, 1);
#else
	marcel_vpset_set(&vpset, 1);
	marcel_vpset_clr(&vpset, 0);
#endif

	for (i = 0; i < N; i++) {
		marcel_disable_vps(&vpset);
		marcel_printf("nuit\n");
		marcel_sleep(DELAY);
		marcel_enable_vps(&vpset);
		marcel_printf("jour\n");
		marcel_sleep(DELAY);
	}

	marcel_end();

	return 0;
}



