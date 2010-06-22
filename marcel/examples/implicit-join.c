/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2009 "the PM2 team" (see AUTHORS file)
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

/* Test whether `marcel_end ()' implicitly joins all currently running
   threads.  */

#include <marcel.h>
#include <stdio.h>
#include <stdint.h>


#define THREADS 123

static void *
thread_entry_point (void *arg)
{
  return arg;
}

int
main (int argc, char *argv[])
{
  int err;
  unsigned i;
  marcel_t threads[THREADS];

  marcel_init(&argc, argv);

  for (i = 0; i < THREADS; i++)
    {
      err = marcel_create (&threads[i], NULL, thread_entry_point, (void *)(uintptr_t) i);
      if (err)
	{
	  perror ("marcel_create");
	  exit (1);
	}
    }

  /* Don't explicitly join threads, let `marcel_end ()' do it.  */

  marcel_end ();

  return 0;
}
