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

/* Test Marcel's libpthread ABI compatibility layer.  */

#ifndef MARCEL_LIBPTHREAD
#  warning Marcel pthread must be enabled for this program
#include <stdio.h>
int marcel_main(int argc, char *argv[])
{
  fprintf(stderr, "'marcel pthread' feature disabled in the flavor\n");

  return 0;
}
#else

/* Include the system's <pthread.h>, not Marcel's.  */
#include <pthread.h>

#ifdef MARCEL_VERSION
# error "We are supposed to use the system pthread library header."
#endif

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <dlfcn.h>


#define THREADS 123

/* Make sure we are actually using Marcel's `libpthread.so', loaded via
   `LD_PRELOAD' or some such.  */
static void
assert_running_marcel (void)
{
  void *self, *sym;

  self = dlopen (NULL, RTLD_NOW);
  if (self)
    {
      sym = dlsym (self, "marcel_create");
      dlclose (self);

      if (sym)
	return;
    }

  fprintf (stderr, "error: not using Marcel's libpthread\n");
  abort ();
}

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
  pthread_t threads[THREADS];

  assert_running_marcel ();

  for (i = 0; i < THREADS; i++)
    {
      err = pthread_create (&threads[i], NULL, thread_entry_point, &threads[i]);
      if (err)
	{
	  fprintf (stderr, "pthread_create: %s\n", strerror (err));
	  exit (1);
	}
    }

  for (i = 0; i < THREADS; i++)
    {
      void *result;

      err = pthread_join (threads[i], &result);
      if (err)
	{
	  fprintf (stderr, "pthread_join: %s\n", strerror (err));
	  exit (1);
	}

      assert (result == &threads[i]);
    }

  return 0;
}

#endif /* MARCEL_LIBPTHREAD */
