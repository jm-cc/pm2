/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2008, 2009 "the PM2 team" (see AUTHORS file)
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

/* Test the `marcel_lookup_bubble_scheduler_class ()' function.  */

#include <marcel.h>

int
main (int argc, char *argv[])
{
#define lookup marcel_lookup_bubble_scheduler_class
  int success;

  marcel_init (&argc, argv);
  marcel_ensure_abi_compatibility (MARCEL_HEADER_HASH);

  success =
    (lookup ("cache") == &marcel_bubble_cache_sched_class
     && lookup ("null") == &marcel_bubble_null_sched_class
     && lookup ("@does-not-exist") == NULL);

  marcel_end ();

  return success ? 0 : 1;
#undef lookup
}
