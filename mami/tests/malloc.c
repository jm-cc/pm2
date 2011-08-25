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
  int err, node, least_loaded_node, maxnode;
  void *ptr, *ptr2, *ptr3, *ptr4;
  mami_manager_t *memory_manager;

  mami_init(&memory_manager, argc, argv);

  if (memory_manager->nb_nodes < 2) {
    fprintf(stderr,"This application needs at least two NUMA nodes.\n");
    mami_exit(&memory_manager);
    return MAMI_TEST_SKIPPED;
  }

  ptr = mami_malloc(memory_manager, 100, MAMI_MEMBIND_POLICY_DEFAULT, 0);
  MAMI_CHECK_MALLOC(ptr);
  err = mami_locate(memory_manager, ptr, 100, &node);
  MAMI_CHECK_RETURN_VALUE(err, "mami_locate");

  if (node == th_mami_current_node()) {
    mprintf(stderr, "Memory allocated on current node\n");
  }
  else {
    mprintf(stderr, "Error. Memory NOT allocated on current node (%d != %d)\n", node, th_mami_current_node());
    return 1;
  }

  maxnode = memory_manager->nb_nodes-1;
  err = mami_membind(memory_manager, MAMI_MEMBIND_POLICY_SPECIFIC_NODE, maxnode);
  MAMI_CHECK_RETURN_VALUE(err, "mami_membind");
  ptr2 = mami_malloc(memory_manager, 100, MAMI_MEMBIND_POLICY_DEFAULT, 0);
  MAMI_CHECK_MALLOC(ptr2);
  err = mami_locate(memory_manager, ptr2, 100, &node);
  MAMI_CHECK_RETURN_VALUE(err, "mami_locate");

  if (node == maxnode) {
    mprintf(stderr, "Memory allocated on given node\n");
  }
  else {
    mprintf(stderr, "Error. Memory NOT allocated on given node (%d != %d)\n", node, maxnode);
    return 1;
  }

  ptr3 = mami_malloc(memory_manager, 100, MAMI_MEMBIND_POLICY_SPECIFIC_NODE, 1);
  MAMI_CHECK_MALLOC(ptr3);
  err = mami_select_node(memory_manager, MAMI_LEAST_LOADED_NODE, &least_loaded_node);
  MAMI_CHECK_RETURN_VALUE(err, "mami_select_node");
  err = mami_membind(memory_manager, MAMI_MEMBIND_POLICY_LEAST_LOADED_NODE, 0);
  MAMI_CHECK_RETURN_VALUE(err, "mami_membind");
  ptr4 = mami_malloc(memory_manager, 100, MAMI_MEMBIND_POLICY_DEFAULT, 0);
  MAMI_CHECK_MALLOC(ptr4);
  err = mami_locate(memory_manager, ptr4, 100, &node);
  MAMI_CHECK_RETURN_VALUE(err, "mami_locate");

  if (node == least_loaded_node) {
    mprintf(stderr, "Memory allocated on least loaded node\n");
  }
  else {
    mprintf(stderr, "Error. Memory NOT allocated on least loaded node (%d != %d)\n", node, least_loaded_node);
    return 1;
  }

  mami_free(memory_manager, ptr);
  mami_free(memory_manager, ptr2);
  mami_free(memory_manager, ptr3);
  mami_free(memory_manager, ptr4);

  mami_exit(&memory_manager);
  return 0;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
  return MAMI_TEST_SKIPPED;
}
#endif
