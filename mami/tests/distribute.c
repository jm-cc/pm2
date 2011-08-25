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

int main(int argc, char * argv[]) {
  int node, *nodes;
  int err, i;
  void *ptr;
  mami_manager_t *memory_manager;

  mami_init(&memory_manager, argc, argv);

  if (memory_manager->nb_nodes < 2) {
    fprintf(stderr,"This application needs at least two NUMA nodes.\n");
    mami_exit(&memory_manager);
    return MAMI_TEST_SKIPPED;
  }

  ptr = malloc(1000);
  err = mami_distribute(memory_manager, ptr, NULL, 0);
  MAMI_CHECK_RETURN_VALUE_IS(err, EINVAL, "mami_distribute");
  free(ptr);

  ptr = mami_malloc(memory_manager, 50000, MAMI_MEMBIND_POLICY_SPECIFIC_NODE, 1);
  MAMI_CHECK_MALLOC(ptr);

  err = mami_locate(memory_manager, ptr, 50000, &node);
  MAMI_CHECK_RETURN_VALUE(err, "mami_locate");
  if (node != 1) {
    mprintf(stderr, "Node is NOT 1 as expected but %d\n", node);
    return 1;
  }

  nodes = malloc(sizeof(int) * memory_manager->nb_nodes);
  for(i=0 ; i<memory_manager->nb_nodes ; i++) nodes[i] = i;
  err = mami_distribute(memory_manager, ptr, nodes, memory_manager->nb_nodes);
  MAMI_CHECK_RETURN_VALUE(err, "mami_distribute");

  err = mami_locate(memory_manager, ptr, 50000, &node);
  MAMI_CHECK_RETURN_VALUE(err, "mami_locate");
  if (node != MAMI_MULTIPLE_LOCATION_NODE) {
    mprintf(stderr, "Node is NOT <MAMI_MULTIPLE_LOCATION_NODE> as expected but %d\n", node);
    return 1;
  }

  err = mami_migrate_on_node(memory_manager, ptr, 0);
  MAMI_CHECK_RETURN_VALUE(err, "mami_migrate_on_node");
  err = mami_locate(memory_manager, ptr, 50000, &node);
  MAMI_CHECK_RETURN_VALUE(err, "mami_locate");

  if (node != 0) {
    mprintf(stderr, "Node is NOT 0 as expected but %d\n", node);
    return 1;
  }

  err = mami_check_pages_location(memory_manager, ptr, 50000, 0);
  MAMI_CHECK_RETURN_VALUE(err, "mami_check_pages_location");

  free(nodes);
  mami_free(memory_manager, ptr);

  mami_exit(&memory_manager);
  return 0;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
  return MAMI_TEST_SKIPPED;
}
#endif
