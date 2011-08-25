
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


#ifdef MA__LWPS


#define N 2
#define DELAY 2

int marcel_main(int argc, char *argv[])
{
	unsigned int i;
	marcel_vpset_t vpset = MARCEL_VPSET_ZERO;

	marcel_init(&argc, argv);
	if (marcel_nbvps() < 2)
		exit(MARCEL_TEST_SKIPPED);

	for (i = 2; i < marcel_nbvps(); i++)
		if (marcel_random() % 2)
			marcel_vpset_set(&vpset, i);

	/* Force at least some enabled and disabled */
	marcel_vpset_set(&vpset, 0);
	marcel_vpset_clr(&vpset, 1);

	for (i = 0; i < N; i++) {
		marcel_disable_vps(&vpset);
#ifdef VERBOSE_TESTS
		marcel_printf("nuit\n");
#endif
		marcel_sleep(DELAY);
		marcel_enable_vps(&vpset);
#ifdef VERBOSE_TESTS
		marcel_printf("jour\n");
#endif
		marcel_sleep(DELAY);
	}
	marcel_disable_vps(&vpset);

	marcel_end();

	return MARCEL_TEST_SUCCEEDED;
}


#else


int marcel_main(int argc TBX_UNUSED, char *argv[]TBX_UNUSED)
{
	return MARCEL_TEST_SKIPPED;
}


#endif
