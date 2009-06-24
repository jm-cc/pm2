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
#include "mm_mami.h"
#include "mm_mami_private.h"

#if defined(MM_MAMI_ENABLED)

mami_manager_t *memory_manager;

int *b;

any_t writer(any_t arg) {
  b = mami_malloc(memory_manager, 3*getpagesize(), MAMI_MEMBIND_POLICY_DEFAULT, 1);
  return 0;
}

any_t reader(any_t arg) {
  int i, node;

  mami_locate(memory_manager, b, 0, &node);
  printf("Address is located on node %d\n", node);

  mami_unset_kernel_migration(memory_manager);
  mami_migrate_on_next_touch(memory_manager, b);

  mami_locate(memory_manager, b, 0, &node);
  printf("Address is located on node %d\n", node);

  for(i=0 ; i<(3*getpagesize())/sizeof(int) ; i++) b[i] = 42;

  mami_locate(memory_manager, b, 0, &node);
  printf("Address is located on node %d\n", node);

  mami_free(memory_manager, b);
  return 0;
}

int main(int argc, char * argv[]) {
  th_mami_t threads[2];
  th_mami_attr_t attr;

  common_init(&argc, argv, NULL);
  mami_init(&memory_manager);
  th_mami_attr_init(&attr);

  if (memory_manager->nb_nodes < 2) {
    printf("This application needs at least two NUMA nodes.\n");
  }
  else {
    // Start the thread on the numa node #0
    th_mami_attr_setnode_level(&attr, 0);
    th_mami_create(&threads[0], &attr, writer, NULL);
    th_mami_join(threads[0], NULL);

    // Start the thread on the numa node #1
    th_mami_attr_setnode_level(&attr, 1);
    th_mami_create(&threads[1], &attr, reader, NULL);
    th_mami_join(threads[1], NULL);
  }

  mami_exit(&memory_manager);
  common_exit(NULL);
  return 0;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
