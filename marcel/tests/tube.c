
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

#ifndef MARCEL_ACT
#define NON_BLOCKING_IO
#endif

#include "marcel.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#define ITERATIONS    1000

typedef struct {
	int tub[2];
	marcel_mutex_t mutex;
} tube;

static tube tube_1, tube_2;

static void create_tube(tube * t)
{
	marcel_mutex_init(&t->mutex, NULL);
	pipe(t->tub);
#ifdef NON_BLOCKING_IO
	fcntl(t->tub[0], F_SETFL, O_NONBLOCK);
	fcntl(t->tub[1], F_SETFL, O_NONBLOCK);
#endif
}

static void destroy_tube(tube * t)
{
	close(t->tub[0]);
	close(t->tub[1]);
	marcel_mutex_destroy(&t->mutex);
}

static int send_message(tube * t, char *message, int len)
{
	int n;

	marcel_mutex_lock(&t->mutex);

#ifdef NON_BLOCKING_IO
	while (1) {
		n = write(t->tub[1], message, len);
		if (n == -1 && errno == EAGAIN)
			marcel_yield();
		else
			break;
	}
#else
#ifdef SHOW
	marcel_printf("Before write %p\n", marcel_self());
#endif
	n = write(t->tub[1], message, len);
#ifdef SHOW
	marcel_printf("After write %p\n", marcel_self());
#endif
#endif

	marcel_mutex_unlock(&t->mutex);

	return n;
}

static int receive_message(tube * t, char *message, int len)
{
	int n;

#ifdef NON_BLOCKING_IO
	while (1) {
		n = read(t->tub[0], message, len);
		if (n == -1 && errno == EAGAIN)
			marcel_yield();
		else
			break;
	}
#else
#ifdef SHOW
	marcel_printf("Before read %p\n", marcel_self());
#endif
	//marcel_yield();
	n = read(t->tub[0], message, len);
#ifdef SHOW
	marcel_printf("After read %p\n", marcel_self());
#endif
#endif

	return n;
}

static void *producteur(void *arg TBX_UNUSED)
{
	int i, n;

	for (i = 0; i < ITERATIONS; i++) {
		send_message(&tube_1, (char *) &i, sizeof(int));
		receive_message(&tube_2, (char *) &n, sizeof(int));
	}
	return NULL;
}

#ifdef __ACT__
extern int nb;
extern unsigned long temps_act;
#endif
static void *consommateur(void *arg TBX_UNUSED)
{
	int i, n;
	tbx_tick_t t1, t2;
	unsigned long temps;

	for (i = 0; i < 3; i++) {
		receive_message(&tube_1, (char *) &n, sizeof(int));
		send_message(&tube_2, (char *) &i, sizeof(int));
	}
	TBX_GET_TICK(t1);
	for (i = 3; i < ITERATIONS; i++) {
		receive_message(&tube_1, (char *) &n, sizeof(int));
		send_message(&tube_2, (char *) &i, sizeof(int));
	}
	TBX_GET_TICK(t2);

	temps = TBX_TIMING_DELAY(t1, t2);
	marcel_printf("time = %ld.%03ldms\n", temps / 1000, temps % 1000);
#ifdef __ACT__
	marcel_printf("nb=%i\n", nb);
	marcel_printf("dont time idle = %ld.%03ldms\n", temps_act / 1000,
		      temps_act % 1000);
#endif

	return NULL;
}


int marcel_main(int argc, char **argv)
{
	marcel_attr_t attr;
	marcel_t pid[2];

	create_tube(&tube_1);
	create_tube(&tube_2);

	marcel_init(&argc, argv);
	marcel_attr_init(&attr);

#ifdef SMP
	marcel_attr_setvpset(&attr, MARCEL_VPSET_VP(0));
#endif
	marcel_create(&pid[0], &attr, consommateur, (void *) 0);
#ifdef SMP
	marcel_attr_setvpset(&attr, MARCEL_VPSET_VP(1));
#endif
	marcel_create(&pid[1], &attr, producteur, (void *) 0);

	marcel_join(pid[0], NULL);
	marcel_join(pid[1], NULL);

	destroy_tube(&tube_1);
	destroy_tube(&tube_2);

	marcel_end();
	return 0;
}
