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

/* Test whether Marcel's <pthread.h> works when used in C++ programs with
   static initializers.  */

#include <pthread.h>

#ifndef MARCEL_VERSION
# error "We're not using Marcel's <pthread.h>."
#endif

#include <cassert>
#include <list>

/* A C++ initializer.  When `new' is called, Marcel is not initialized yet,
   so we expect libc's `malloc ()' to be used rather than Marcel's.  */
static std::list<int> *the_list = new std::list<int> ();


int
main (int argc, char *argv[])
{
  assert (marcel_test_activity ());

  assert (the_list != NULL);

  /* At this point Marcel is initialized so we expect Marcel's `delete' to be
     used, which shouldn't be a problem.  */
  delete the_list;

  return 0;
}
