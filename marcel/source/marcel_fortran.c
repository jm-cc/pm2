/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2006 "the PM2 team" (see AUTHORS file)
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

#include <stdio.h>
#include <stdlib.h>

#ifdef MARCEL_FORTRAN

void marcel_init_(void)
{
	int i, argc;
	char **argv;

	tbx_fortran_init(&argc, &argv);
	marcel_init(&argc, argv);

	for (i = 0; i < argc; i++) {
		marcel_free(argv[i]);
	}

	marcel_free(argv);
}


void marcel_end_(void)
{
	marcel_end();
}


void marcel_self_(marcel_t * self)
{
	*self = ma_self();
}

// a tester sur d'autres architectures et compilateurs
void marcel_create_(marcel_t * pid, void *(*func) (void *), void *arg)
{
	marcel_create(pid, NULL, func, arg);
}


void marcel_join_(marcel_t * pid)
{
	marcel_join(*pid, NULL);
}


void marcel_yield_(void)
{
	marcel_yield();
}


#ifdef MARCEL_STATS_ENABLED
void marcel_set_load_(int *load)
{
	*marcel_task_stats_get(ma_self(), LOAD) = *load;
}
#endif

/* Utile pour debug */
void marcel_rien_(void)
{
}

#ifdef MA__BUBBLES

/* TODO: rajouter la bulle qu'on soumet en parametre */
void marcel_bubble_submit_(void)
{
	marcel_bubble_submit(&marcel_root_bubble);
}

void marcel_bubble_sched_begin_()
{
	marcel_bubble_sched_begin();
}

void marcel_bubble_shake_()
{
	marcel_bubble_shake();
}

#endif				/* MA__BUBBLES */

#endif
