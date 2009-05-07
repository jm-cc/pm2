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

/* Test whether the program terminates even in the presence on
   non-cooperative threads (sleeping on a mutex).  */

#include "pthread-abi.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>


#define THREADS 123

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void *
thread_entry_point (void *arg)
{
  pthread_mutex_lock (&mutex);
  return arg;
}

int
main (int argc, char *argv[])
{
  int err;
  unsigned i;
  pthread_t threads[THREADS];

  assert_running_marcel ();

  /* Take the mutex.  */
  thread_entry_point ((void *) 0);

  for (i = 0; i < THREADS; i++)
    {
      err = pthread_create (&threads[i], NULL, thread_entry_point, (void *)(uintptr_t) i);
      if (err)
	{
	  perror ("pthread_create");
	  exit (1);
	}
    }

  /* Don't explicitly join threads.  At this point, all threads but the main
     thread are waiting on `mutex_lock ()', but the program should terminate
     nonetheless.  */

  return 0;
}
