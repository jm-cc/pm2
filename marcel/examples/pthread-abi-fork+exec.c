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

/* Test the behavior of Marcel's libpthread ABI compatibility layer
   wrt. fork(2).  */

#ifndef MARCEL_LIBPTHREAD

#  warning Marcel pthread must be enabled for this program
#  include <stdio.h>
int
main (int argc, char *argv[])
{
  fprintf (stderr, "`MARCEL_LIBPTHREAD' feature disabled, nothing done\n");
  return 0;
}

#else /* MARCEL_LIBPTHREAD */

/* Include the system's <pthread.h>, not Marcel's.  */
#include "pthread-abi.h"

#ifdef MARCEL_VERSION
# error "We are supposed to use the system pthread library header."
#endif

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <dlfcn.h>


#define THREADS 123

static void *
thread_entry_point (void *arg)
{
  /* Make sure we have a working `pthread_self ()'.  */
  if (* (pthread_t *) arg != pthread_self ())
    abort ();

  return arg;
}


int
main (int argc, char *argv[])
{
  int err;
  pid_t kid;
  unsigned i;
  pthread_t threads[THREADS];

  assert_running_marcel ();

  for (i = 0; i < THREADS; i++)
    {
      err = pthread_create (&threads[i], NULL, thread_entry_point, &threads[i]);
      if (err)
	{
	  perror ("pthread_create");
	  return EXIT_FAILURE;
	}
    }

  /* Fork while LWPs and user-level threads are still alive.  */
  kid = fork ();
  if (kid < 0)
    {
      perror ("fork");
      return EXIT_FAILURE;
    }
  else if (kid == 0)
    /* Child process.  The exec'd process should not get any SIGALRM from
       Marcel's timer interrupt.  */
    execlp ("sleep", "sleep", "1", NULL);
  else
    {
      /* Parent process.  */
      int kid_status, err;

      /* Join our threads.  */
      for (i = 0; i < THREADS; i++)
	{
	  void *result;

	  err = pthread_join (threads[i], &result);
	  if (err)
	    {
	      perror ("pthread_join");
	      return EXIT_FAILURE;
	    }

	  assert (result == &threads[i]);
	}

      err = waitpid (kid, &kid_status, 0);
      if (err == -1)
	{
	  perror ("wait");
	  return EXIT_FAILURE;
	}
      else
	assert (err == kid);

      if (! (WIFEXITED (kid_status) && WEXITSTATUS (kid_status) == EXIT_SUCCESS))
	{
	  if (!WIFEXITED (kid_status))
	    fprintf (stderr, "child terminated abnormally\n");
	  else
	    fprintf (stderr, "child returned %i\n", WEXITSTATUS (kid_status));

	  if (WIFSIGNALED (kid_status))
	    fprintf (stderr, "child terminated with signal %i\n",
		     WTERMSIG (kid_status));

	  abort ();
	}
    }

  return EXIT_SUCCESS;
}

#endif /* MARCEL_LIBPTHREAD */
