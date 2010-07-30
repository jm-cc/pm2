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

#include <stdio.h>

#if defined(MM_MAMI_ENABLED)

#include <mm_mami.h>
#include "helper.h"

int stats(char *msg, void *ptr, long stat);

int main(int argc, char * argv[]) {
  mami_manager_t *memory_manager;
  int *ptr, err;

  marcel_init(&argc,argv);
  mami_init(&memory_manager, argc, argv);

  ptr = mami_malloc(memory_manager, 100, MAMI_MEMBIND_POLICY_SPECIFIC_NODE, 0);
  MAMI_CHECK_MALLOC(ptr);
  stats("[after malloc]", ptr, 0);

  err = mami_attach_on_next_touch(memory_manager, ptr);
  MAMI_CHECK_RETURN_VALUE(err, "mami_attach_on_next_touch");
  stats("[after attach_on_next_touch]", ptr, 0);
  ptr[0] = 42;
  stats("[after touching]", ptr, 4096);

  err = mami_task_unattach(memory_manager, ptr, marcel_self());
  MAMI_CHECK_RETURN_VALUE(err, "mami_task_unattach");
  stats("[after unattach]", ptr, 0);

  mami_free(memory_manager, ptr);
  mami_exit(&memory_manager);

  // Finish marcel
  marcel_end();
  return 0;
}

int stats(char *msg, void *ptr, long stat) {
  marcel_entity_t *entity;
  int err, node;
  long* stats;

  stats = (long *) marcel_task_stats_get (marcel_self(), MEMNODE);
  if (stats[0] != stat) {
    fprintf(stderr, "%s Value %ld != Expected value %ld\n", msg, stats[0], stat);
    return 1;
  }
  else {
    fprintf(stderr, "%s Value %ld == Expected value %ld\n", msg, stats[0], stat);
    return 0;
  }
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
