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

int main(int argc, char * argv[]) {
  int node, *nodes;
  int err, i;
  void *ptr;
  mami_manager_t *memory_manager;

  common_init(&argc, argv, NULL);
  mami_init(&memory_manager);

  ptr = malloc(1000);
  err = mami_distribute(memory_manager, ptr, NULL, 0);
  free(ptr);
  if (err < 0) perror("mami_distribute successfully failed");

  ptr = mami_malloc(memory_manager, 50000, MAMI_MEMBIND_POLICY_SPECIFIC_NODE, 1);
  if (!ptr) {
    perror("mami_malloc failed");
  }
  else {
    mami_locate(memory_manager, ptr, 50000, &node);
    if (node == 1)
      fprintf(stderr, "Node is %d as expected\n", node);
    else
      fprintf(stderr, "Node is NOT 1 as expected but %d\n", node);

    nodes = malloc(sizeof(int) * memory_manager->nb_nodes);
    for(i=0 ; i<memory_manager->nb_nodes ; i++) nodes[i] = i;
    err = mami_distribute(memory_manager, ptr, nodes, memory_manager->nb_nodes);
    if (err < 0) perror("mami_distribute unexpectedly failed");
    else {
      mami_locate(memory_manager, ptr, 50000, &node);
      if (node == MAMI_MULTIPLE_LOCATION_NODE)
	fprintf(stderr, "Node is <MAMI_MULTIPLE_LOCATION_NODE> as expected\n");
      else
	fprintf(stderr, "Node is NOT <MAMI_MULTIPLE_LOCATION_NODE> as expected but %d\n", node);

      mami_migrate_on_node(memory_manager, ptr, 0);
      mami_locate(memory_manager, ptr, 50000, &node);
      if (node == 0)
	fprintf(stderr, "Node is %d as expected\n", node);
      else
	fprintf(stderr, "Node is NOT 0 as expected but %d\n", node);

      mami_check_pages_location(memory_manager, ptr, 50000, 0);
    }

    free(nodes);
    mami_free(memory_manager, ptr);
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
