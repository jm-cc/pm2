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

/* Test thread-local storage (TLS) support for Marcel threads.  */

#include <marcel.h>

#ifndef MA__PROVIDE_TLS
# error "TLS support is disabled."
#endif

#include <stdlib.h>
#include <stdio.h>



/* Number of threads.  */
#define THREADS 17

/* Thread-local storage.  */
static __thread int tls_data;


static void *
thread_main (void *arg)
{
  /* Initialize our TLS.  */
  tls_data = (int) arg;

  /* Just leave enough time for others to override TLS_DATA (provided TLS is
     broken).  */
  marcel_sleep (marcel_random () % 3);

  /* Return the last value seen.  */
  return (void *) tls_data;
}


int
main (int argc, char *argv[])
{
  int err;
  unsigned i;
  marcel_t threads[THREADS];

  marcel_init (&argc, argv);

  tls_data = -1;

  for (i = 0; i < THREADS; i++)
    {
      err = marcel_create (&threads[i], NULL,
			   thread_main, (void *) i);
      if (err)
	return 1;
    }

  for (i = 0; i < THREADS; i++)
    {
      void *ret;

      err = marcel_join (threads[i], &ret);
      if (err)
	return 2;

      if (i != (int) ret)
	{
	  printf ("got %i instead of %i\n", (int) ret, i);
	  return 3;
	}
    }

  if (tls_data != -1)
    {
      printf ("main thread got %i instead of -1\n", tls_data);
      return 4;
    }

  marcel_end ();

  return 0;
}
