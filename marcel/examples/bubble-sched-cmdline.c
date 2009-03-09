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

/* Test the `marcel_lookup_bubble_scheduler ()' function.  */

#include <marcel.h>

int
main (int argc, char *argv[])
{
  int ret;
  char *new_argv[4];
  char  sched_opt[] = "--marcel-bubble-scheduler";
  char  sched_arg[] = "cache";
  marcel_bubble_sched_t *prev_sched;

  argc = 3;
  new_argv[0] = argv[0];
  new_argv[1] = sched_opt;
  new_argv[2] = sched_arg;
  new_argv[3] = NULL;

  marcel_init (&argc, new_argv);
  marcel_ensure_abi_compatibility (MARCEL_HEADER_HASH);

  prev_sched = marcel_bubble_change_sched (&marcel_bubble_null_sched);

  ret = (prev_sched->klass == &marcel_bubble_cache_sched_class) ? 0 : 1;

  marcel_free (prev_sched);

  marcel_end ();

  return ret;
}
