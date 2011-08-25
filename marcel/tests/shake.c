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


#include <marcel.h>


#ifdef MA__BUBBLES


marcel_barrier_t shake_barrier;

static void *
my_shake (void *arg TBX_UNUSED)
{
	/* Wait for the main thread to end its initial thread
	   distribution. */
	marcel_barrier_wait (&shake_barrier);

	/* Shake it baby! */
	marcel_bubble_shake ();

	return NULL;
}

int
main (int argc, char **argv)
{
	unsigned int i, nb_vps;
	marcel_attr_t thread_attr;
	marcel_barrierattr_t barrier_attr;

	marcel_init(&argc, argv);

	nb_vps = marcel_nbvps ();
	marcel_t tids[nb_vps];
	marcel_attr_init (&thread_attr);
	marcel_attr_setnaturalbubble (&thread_attr, &marcel_root_bubble);
	marcel_barrierattr_init (&barrier_attr);
	marcel_barrierattr_setmode (&barrier_attr, MARCEL_BARRIER_YIELD_MODE);
	marcel_barrier_init (&shake_barrier, &barrier_attr, nb_vps + 1);

	for (i = 0; i < nb_vps; i++)
		marcel_create (&tids[i], &thread_attr, my_shake, NULL);

	/* Distribute the threads. */
	marcel_bubble_sched_begin ();

	/* Let the threads shake! */
	marcel_barrier_wait (&shake_barrier);

	for (i = 0; i < nb_vps; i++)
		marcel_join (tids[i], NULL);

	marcel_end ();
	return MARCEL_TEST_SUCCEEDED;
}


#else /* MA__BUBBLES */


int main(int argc TBX_UNUSED, char *argv[] TBX_UNUSED) {
	return MARCEL_TEST_SKIPPED;
}


#endif /* MA__BUBBLES */
