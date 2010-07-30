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
#include <mm_mami_private.h>
#include "helper.h"

mami_manager_t *memory_manager;

int *b;

any_t writer(any_t arg) {
  b = mami_malloc(memory_manager, 3*getpagesize(), MAMI_MEMBIND_POLICY_DEFAULT, 1);
  MAMI_CHECK_MALLOC_THREAD(b);
  return ALL_IS_OK;
}

any_t reader(any_t arg) {
  int i, node, err;

  err = mami_locate(memory_manager, b, 0, &node);
  MAMI_CHECK_RETURN_VALUE_THREAD(err, "mami_locate");
  if (node != 0) {
    fprintf(stderr, "Address is NOT located on node %d but on node %d\n", 0, node);
    return ALL_IS_NOT_OK;
  }

  mami_unset_kernel_migration(memory_manager);
  mami_migrate_on_next_touch(memory_manager, b);

  err = mami_locate(memory_manager, b, 0, &node);
  MAMI_CHECK_RETURN_VALUE_THREAD(err, "mami_locate");
  if (node != 0) {
    fprintf(stderr, "Address is NOT located on node %d but on node %d\n", 0, node);
    return ALL_IS_NOT_OK;
  }

  for(i=0 ; i<(3*getpagesize())/sizeof(int) ; i++) b[i] = 42;

  err = mami_locate(memory_manager, b, 0, &node);
  MAMI_CHECK_RETURN_VALUE_THREAD(err, "mami_locate");
  if (node != 1) {
    fprintf(stderr, "Address is NOT located on node %d but on node %d\n", 1, node);
    return ALL_IS_NOT_OK;
  }

  mami_free(memory_manager, b);
  return ALL_IS_OK;
}

int main(int argc, char * argv[]) {
  th_mami_t threads[2];
  th_mami_attr_t attr;

  common_init(&argc, argv, NULL);
  mami_init(&memory_manager, argc, argv);
  th_mami_attr_init(&attr);

  if (memory_manager->nb_nodes < 2) {
    printf("This application needs at least two NUMA nodes.\n");
  }
  else {
    any_t status;

    // Start the thread on the numa node #0
    th_mami_attr_setnode_level(&attr, 0);
    th_mami_create(&threads[0], &attr, writer, NULL);
    th_mami_join(threads[0], &status);
    if (status == ALL_IS_NOT_OK) return 1;

    // Start the thread on the numa node #1
    th_mami_attr_setnode_level(&attr, 1);
    th_mami_create(&threads[1], &attr, reader, NULL);
    th_mami_join(threads[1], &status);
    if (status == ALL_IS_NOT_OK) return 1;
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
