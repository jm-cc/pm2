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

/* Test the `pthread_key' API when using Marcel's libpthread ABI
   compatibility layer.  */

#include "pthread-abi.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#undef NDEBUG
#include <assert.h>


#define THREADS 123

static pthread_key_t key;
static int key_destroyed[THREADS];

static void
key_destructor (void *arg)
{
  unsigned int number;

  number = (uintptr_t) arg;
  assert (number < THREADS);
  printf ("%s %u\n", __func__, number);
  key_destroyed[number] = 1;
}

static void *
thread_entry_point (void *arg)
{
  pthread_setspecific (key, arg);
  assert (pthread_getspecific (key) == arg);

  return arg;
}

int
main (int argc, char *argv[])
{
  int err;
  unsigned i;
  pthread_t threads[THREADS];

  assert_running_marcel ();

  err = pthread_key_create (&key, key_destructor);
  if (err)
    {
      perror ("pthread_key_create");
      exit (1);
    }

  for (i = 0; i < THREADS; i++)
    {
      err = pthread_create (&threads[i], NULL, thread_entry_point, (void *)(uintptr_t) i);
      if (err)
	{
	  perror ("pthread_create");
	  exit (1);
	}
    }

  thread_entry_point ((void *) 0);

  for (i = 0; i < THREADS; i++)
    {
      void *result;

      err = pthread_join (threads[i], &result);
      if (err)
  	{
  	  perror ("pthread_join");
  	  exit (1);
  	}

      assert ((uintptr_t) result == i);
    }

  err = pthread_key_delete (key);
  if (err)
    {
      perror ("pthread_key_delete");
      exit (1);
    }

  for (i = 1; i < THREADS; i++)
    assert (key_destroyed[i] == 1);

  return 0;
}

