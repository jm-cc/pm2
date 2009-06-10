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

#define MARCEL_INTERNAL_INCLUDE
#include <stdio.h>
#include "mm_mami.h"

#if defined(MM_MAMI_ENABLED)

static mami_manager_t *memory_manager;
void stats(void *ptr);

int main(int argc, char * argv[]) {
  int *ptr;

  marcel_init(&argc,argv);
  mami_init(&memory_manager);

  ptr = mami_malloc(memory_manager, 100, MAMI_MEMBIND_POLICY_SPECIFIC_NODE, 0);
  stats(ptr);
  mami_free(memory_manager, ptr);

  ptr = mami_malloc(memory_manager, 100, MAMI_MEMBIND_POLICY_FIRST_TOUCH, 0);
  stats(ptr);
  mami_free(memory_manager, ptr);

  mami_exit(&memory_manager);

  // Finish marcel
  marcel_end();
  return 0;
}

void stats(void *ptr) {
  marcel_entity_t *entity;
  int err, node;

  entity = ma_entity_task(marcel_self());

  marcel_printf("[before attach]  stats=%ld\n", ((long *) ma_stats_get (entity, ma_stats_memnode_offset))[0]);

  err = mami_task_attach(memory_manager, ptr, 100, marcel_self(), &node);
  if (err < 0) perror("mami_task_attach unexpectedly failed");

  marcel_printf("[after attach]   stats=%ld\n", ((long *) ma_stats_get (entity, ma_stats_memnode_offset))[0]);

  err = mami_task_unattach(memory_manager, ptr, marcel_self());
  if (err < 0) perror("mami_task_unattach unexpectedly failed");

  marcel_printf("[after unattach] stats=%ld\n", ((long *) ma_stats_get (entity, ma_stats_memnode_offset))[0]);
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
