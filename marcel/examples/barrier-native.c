/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2008 "the PM2 team" (see AUTHORS file)
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


/* Test Marcel's native barrier API with all supported barrier modes.  */

#include <marcel.h>


/* Total number of threads.  */
#define THREADS 17

static void *
start_thread (void *data)
{
  int err;
  marcel_barrier_t *barrier = (marcel_barrier_t *) data;

  err = marcel_barrier_wait (barrier);

  return (void *) err;
}


int
main (int argc, char *argv[])
{
  static const ma_barrier_mode_t barrier_modes[] =
    {
      MA_BARRIER_SLEEP_MODE, MA_BARRIER_YIELD_MODE
    };

  static const marcel_attr_t tattr = MARCEL_ATTR_INITIALIZER;

  unsigned mode, thread;

  marcel_init (&argc, argv);

  for (mode = 0;
       mode < sizeof (barrier_modes) / sizeof (barrier_modes[1]);
       mode++)
    {
      int err;
      marcel_t threads[THREADS];
      marcel_barrier_t barrier;
      marcel_barrierattr_t battr;

      marcel_barrierattr_init (&battr);
      marcel_barrierattr_setmode (&battr, barrier_modes[mode]);

      marcel_barrier_init (&barrier, &battr, THREADS);

      for (thread = 0; thread < THREADS; thread++)
	{
	  err = marcel_create (&threads[thread], &tattr, start_thread, &barrier);
	  if (err != 0)
	    {
	      printf ("failed to create thread %u: error %i\n",
		      thread, err);
	      return 1;
	    }
	}

      for (thread = 0; thread < THREADS; thread++)
	{
	  void *ret;

	  err = marcel_join (threads[thread], &ret);
	  if (err != 0)
	    {
	      printf ("failed to join thread %u: error %i\n",
		      thread, err);
	      return 1;
	    }

	  if (ret != NULL)
	    {
	      printf ("thread %u failed with %p\n", thread, ret);
	      return 2;
	    }
	}

      err = marcel_barrier_destroy (&barrier);
      if (err != 0)
	{
	  printf ("failed to destroy barrier with mode %i\n",
		  (int) barrier_modes[mode]);
	  return 1;
	}
    }

  return 0;
}
