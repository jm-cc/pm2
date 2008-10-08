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


/* Test Marcel's native barrier API with all supported barrier modes and both
   with and without thread seeds.  */

#include <marcel.h>


/* Total number of threads.  */
#define THREADS 17

static void *
start_thread (void *data)
{
  int err;
  marcel_barrier_t *barrier = (marcel_barrier_t *) data;

  err = marcel_barrier_wait (barrier);

  return (void *) (intptr_t) err;
}


int
main (int argc, char *argv[])
{
  static const ma_barrier_mode_t barrier_modes[] =
    {
      MA_BARRIER_SLEEP_MODE, MA_BARRIER_YIELD_MODE
    };

  unsigned mode, thread, yesno;

  marcel_init (&argc, argv);
	marcel_ensure_abi_compatibility (MARCEL_HEADER_HASH);

  for (yesno = 0; yesno < 2; yesno++)
    {
      for (mode = 0;
	   mode < sizeof (barrier_modes) / sizeof (barrier_modes[1]);
	   mode++)
	{
	  int err;
	  marcel_t threads[THREADS];
	  marcel_attr_t tattr;
	  marcel_barrier_t barrier;
	  marcel_barrierattr_t battr;
	  unsigned seen_barrier_serial_thread = 0;

	  marcel_barrierattr_init (&battr);
	  marcel_barrierattr_setmode (&battr, barrier_modes[mode]);

	  marcel_barrier_init (&barrier, &battr, THREADS);

	  marcel_attr_init (&tattr);
	  marcel_attr_setseed (&tattr, yesno);

	  /* Use the default priority, which is the same as that of the main
	     thread.  */
	  marcel_attr_setprio (&tattr, MA_DEF_PRIO);

	  for (thread = 0; thread < THREADS; thread++)
	    {
	      err = marcel_create (&threads[thread], &tattr,
				   start_thread, &barrier);
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

	      if (ret == (void *) MARCEL_BARRIER_SERIAL_THREAD)
		seen_barrier_serial_thread++;
	    }

	  if (seen_barrier_serial_thread != 1)
	    {
	      printf ("didn't get `MARCEL_BARRIER_SERIAL_THREAD' "
		      "once as expected: %u\n",
		      seen_barrier_serial_thread);
	      return 1;
	    }

	  err = marcel_attr_destroy (&tattr);
	  if (err != 0)
	    {
	      printf ("failed to destroy thread attributes\n");
	      return 1;
	    }

	  err = marcel_barrier_destroy (&barrier);
	  if (err != 0)
	    {
	      printf ("failed to destroy barrier with mode %i\n",
		      (int) barrier_modes[mode]);
	      return 1;
	    }

	  err = marcel_barrierattr_destroy (&battr);
	  if (err != 0)
	    {
	      printf ("failed to destroy barrier attribute\n");
	      return 1;
	    }
	}
    }

  marcel_end ();

  return 0;
}
