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
struct location_s {
  int node_1;
  int node_2;
};
any_t reader(any_t arg);

int main(int argc, char * argv[]) {
  th_mami_t thread;
  th_mami_attr_t attr;

  mami_init(&memory_manager, argc, argv);
  th_mami_attr_init(&attr);

  if (memory_manager->nb_nodes < 3) {
    fprintf(stderr, "This application needs at least two NUMA nodes.\n");
    mami_exit(&memory_manager);
    return MAMI_TEST_SKIPPED;
  }
  else {
    struct location_s location;
    any_t status;
    int err;

    b = mami_malloc(memory_manager, 3*getpagesize(), MAMI_MEMBIND_POLICY_FIRST_TOUCH, 0);

    mami_unset_kernel_migration(memory_manager);
    mami_migrate_on_next_touch(memory_manager, b);

    // Start the thread on the numa node #1
    location.node_1 = MAMI_FIRST_TOUCH_NODE;
    location.node_2 = 1;
    th_mami_attr_setnode_level(&attr, 1);
    th_mami_create(&thread, &attr, reader, (any_t)&location);
    th_mami_join(thread, &status);
    if (status == ALL_IS_NOT_OK) return 1;

    err = mami_migrate_on_next_touch(memory_manager, b);
    MAMI_CHECK_RETURN_VALUE(err, "mami_migrate_on_next_touch");

    // Start the thread on the numa node #2
    location.node_1 = 1;
    location.node_2 = 2;
    th_mami_attr_setnode_level(&attr, 2);
    th_mami_create(&thread, &attr, reader, (any_t)&location);
    th_mami_join(thread, &status);
    if (status == ALL_IS_NOT_OK) return 1;

    mami_free(memory_manager, b);
  }

  mami_exit(&memory_manager);
  return 0;
}

any_t reader(any_t arg) {
  int i, node, err;
  struct location_s *location = (struct location_s *)arg;

  err = mami_locate(memory_manager, b, 0, &node);
  MAMI_CHECK_RETURN_VALUE_THREAD(err, "mami_locate");

  if (node != location->node_1) {
    mprintf(stderr, "Address is NOT located on node %d but on node %d\n", location->node_1, node);
    return ALL_IS_NOT_OK;
  }

  for(i=0 ; i<(3*getpagesize())/sizeof(int) ; i++)
    b[i] = 42;

  err = mami_update_pages_location(memory_manager, b, 0);
  MAMI_CHECK_RETURN_VALUE_THREAD(err, "mami_update_pages_location");
  err = mami_locate(memory_manager, b, 0, &node);
  MAMI_CHECK_RETURN_VALUE_THREAD(err, "mami_locate");

  if (node != location->node_2) {
    mprintf(stderr, "Address is NOT located on node %d but on node %d\n", location->node_2, node);
    return ALL_IS_NOT_OK;
  }

  err = mami_check_pages_location(memory_manager, b, 0, th_mami_current_node());
  MAMI_CHECK_RETURN_VALUE_THREAD(err, "mami_check_pages_location");

  return ALL_IS_OK;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
  return MAMI_TEST_SKIPPED;
}
#endif
