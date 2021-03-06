/* MaMI --- NUMA Memory Interface
 *
 * Copyright (C) 2008, 2009, 2010, 2011  Centre National de la Recherche Scientifique
 *
 * MaMI is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * MaMI is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU Lesser General Public License in COPYING.LGPL for more details.
 */
#include <stdio.h>

#if defined(MAMI_ENABLED)

#include <mami.h>
#include <mami_private.h>
#include "helper.h"

mami_manager_t *memory_manager;
int *b;
any_t writer(any_t arg);
any_t reader(any_t arg);

int main(int argc, char * argv[]) {
  th_mami_t threads[3];
  th_mami_attr_t attr;

  mami_init(&memory_manager, argc, argv);
  th_mami_attr_init(&attr);

  if (memory_manager->nb_nodes < 3) {
    fprintf(stderr, "This application needs at least three NUMA nodes.\n");
    mami_exit(&memory_manager);
    return MAMI_TEST_SKIPPED;
  }
  else {
    int enode = 0;
    any_t status;

    // Start the thread on the numa node #0
    th_mami_attr_setnode_level(&attr, 0);
    th_mami_create(&threads[0], &attr, writer, NULL);
    th_mami_join(threads[0], &status);
    if (status == ALL_IS_NOT_OK) return 1;

    // Start the thread on the numa node #1
    th_mami_attr_setnode_level(&attr, 1);
    th_mami_create(&threads[1], &attr, reader, (any_t)&enode);
    th_mami_join(threads[1], &status);
    if (status == ALL_IS_NOT_OK) return 1;

    // Start the thread on the numa node #2
    enode = 1;
    th_mami_attr_setnode_level(&attr, 2);
    th_mami_create(&threads[2], &attr, reader, (any_t)&enode);
    th_mami_join(threads[2], &status);
    if (status == ALL_IS_NOT_OK) return 1;
  }

  mami_free(memory_manager, b);
  mami_exit(&memory_manager);
  return 0;
}

any_t writer(any_t arg) {
  int err;

  b = mami_malloc(memory_manager, 3*getpagesize(), MAMI_MEMBIND_POLICY_DEFAULT, 0);
  MAMI_CHECK_MALLOC_THREAD(b);
  mami_unset_kernel_migration(memory_manager);
  err = mami_migrate_on_next_touch(memory_manager, b);
  MAMI_CHECK_RETURN_VALUE_THREAD(err, "mami_migrate_on_next_touch");

  return ALL_IS_OK;
}

any_t reader(any_t arg) {
  int node, err;
  int *enode = (int *) arg;

  err = mami_locate(memory_manager, b, 0, &node);
  MAMI_CHECK_RETURN_VALUE_THREAD(err, "mami_locate");
  if (node != *enode) {
    mprintf(stderr, "Address is NOT located on node %d but on node %d\n", *enode, node);
    return ALL_IS_NOT_OK;
  }

  b[1] = 42;

  err = mami_locate(memory_manager, b, 0, &node);
  MAMI_CHECK_RETURN_VALUE_THREAD(err, "mami_locate");
  if (node != 1) {
    mprintf(stderr, "Address is NOT located on node %d but on node %d\n", 1, node);
    return ALL_IS_NOT_OK;
  }

  return ALL_IS_OK;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
  return MAMI_TEST_SKIPPED;
}
#endif


