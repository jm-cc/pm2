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
  mami_manager_t *memory_manager;
  int anode, bnode, err;
  void *ptr;
  size_t size;

  mami_init(&memory_manager, argc, argv);

  if (memory_manager->nb_nodes < 2) {
    fprintf(stderr,"This application needs at least two NUMA nodes.\n");
    mami_exit(&memory_manager);
    return MAMI_TEST_SKIPPED;
  }

  err = mami_select_node(memory_manager, MAMI_LEAST_LOADED_NODE, &anode);
  MAMI_CHECK_RETURN_VALUE(err, "mami_select_node");

  size = memory_manager->mem_total[anode]/4;
  ptr = mami_malloc(memory_manager, size, MAMI_MEMBIND_POLICY_SPECIFIC_NODE, anode);
  MAMI_CHECK_MALLOC(ptr);

  err = mami_select_node(memory_manager, MAMI_LEAST_LOADED_NODE, &bnode);
  MAMI_CHECK_RETURN_VALUE(err, "mami_select_node");
  if (anode == bnode) {
    mprintf(stderr, "The least loaded anode is: %d\n", anode);
    mprintf(stderr, "The least loaded bnode is: %d\n", bnode);
    return 1;
  }

  mami_free(memory_manager, ptr);
  mami_exit(&memory_manager);
  mprintf(stdout, "Success\n");
  return 0;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
  return MAMI_TEST_SKIPPED;
}
#endif
