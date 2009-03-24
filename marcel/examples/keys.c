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

/* keys.c */

#define MARCEL_INTERNAL_INCLUDE
#include <marcel.h>
#include <stdint.h>

#undef NDEBUG
#include <assert.h>


#ifdef MARCEL_KEYS_ENABLED

#define THREADS 123

static marcel_key_t key;
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
  marcel_setspecific (key, arg);
  marcel_yield ();
  assert (marcel_getspecific (key) == arg);

  return arg;
}

int
main (int argc, char *argv[])
{
  int err;
  unsigned i;
  marcel_t threads[THREADS];

  marcel_init (&argc, argv);

  err = marcel_key_create (&key, key_destructor);
  if (err)
    {
      perror ("marcel_key_create");
      exit (1);
    }

  for (i = 0; i < THREADS; i++)
    {
      err = marcel_create (&threads[i], NULL, thread_entry_point, (void *)(uintptr_t) i);
      if (err)
	{
	  perror ("marcel_create");
	  exit (1);
	}
    }

  thread_entry_point ((void *) 0);

  for (i = 0; i < THREADS; i++)
    {
      void *result;

      err = marcel_join (threads[i], &result);
      if (err)
	{
	  perror ("marcel_join");
	  exit (1);
	}

      assert ((uintptr_t) result == i);
    }

  err = marcel_key_delete (key);
  if (err)
    {
      perror ("marcel_key_delete");
      exit (1);
    }

  for (i = 1; i < THREADS; i++)
    assert (key_destroyed[i] == 1);

  marcel_end ();

  return 0;
}

#else /* !MARCEL_KEYS_ENABLED */
#  warning Marcel keys must be enabled for this program
int main(int argc, char *argv[])
{
  fprintf(stderr,
	  "'marcel keys' feature disabled in the flavor\n");

  return 0;
}
#endif /* !MARCEL_KEYS_ENABLED */
