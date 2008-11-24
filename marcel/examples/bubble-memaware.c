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

#include "bubble-testing.h"


int
main (int argc, char *argv[])
{
  /* A simple topology: 2 nodes, each of which has 2 CPUs, each of which has 4
     cores.  */
  static const char topology_description[] = "2 2 4";

  /* A flat bubble hierarchy.  */
  static const unsigned bubble_hierarchy_description[] =
    { 16, 0 };

  return test_marcel_bubble_scheduler (argc, argv,
                                       &marcel_bubble_memaware_sched,
                                       topology_description,
                                       bubble_hierarchy_description,
                                       NULL);
}
