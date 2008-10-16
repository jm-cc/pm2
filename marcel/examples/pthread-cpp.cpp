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

#include <pthread.h>

#ifndef MARCEL_VERSION
# error "We're not using Marcel's <pthread.h>."
#endif

#include <cassert>
#include <list>

#define THREADS  77

static void *
thread_entry_point (void *unused)
{
	std::list<int> *list;

	list = new std::list<int> ();

	list->push_back (777);

	return list;
}


int
main (int argc, char *argv[])
{
	int err;
	unsigned i;
	pthread_t threads[THREADS];

  threads[0] = pthread_self ();
  for (i = 1; i < THREADS - 1; i++)
    {
      err = pthread_create (&threads[i], NULL, thread_entry_point, NULL);
      assert (err == 0);
    }

  for (i = 1; i < THREADS - 1; i++)
    {
			std::list<int> *list;

      err = pthread_join (threads[i], (void **) &list);
      assert (err == 0);

			delete list;
    }

	return 0;
}
