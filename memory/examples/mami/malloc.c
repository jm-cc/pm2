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
  int node, least_loaded_node, maxnode;
  void *ptr, *ptr2, *ptr3, *ptr4;
  mami_manager_t *memory_manager;

  common_init(&argc, argv, NULL);
  mami_init(&memory_manager);

  ptr = mami_malloc(memory_manager, 100, MAMI_MEMBIND_POLICY_DEFAULT, 0);
  mami_locate(memory_manager, ptr, 100, &node);
  if (node == th_mami_current_node()) {
    printf("Memory allocated on current node\n");
  }
  else {
    printf("Error. Memory NOT allocated on current node (%d != %d)\n", node, th_mami_current_node());
  }

  maxnode = memory_manager->nb_nodes-1;
  mami_membind(memory_manager, MAMI_MEMBIND_POLICY_SPECIFIC_NODE, maxnode);
  ptr2 = mami_malloc(memory_manager, 100, MAMI_MEMBIND_POLICY_DEFAULT, 0);
  mami_locate(memory_manager, ptr2, 100, &node);
  if (node == maxnode) {
    printf("Memory allocated on given node\n");
  }
  else {
    printf("Error. Memory NOT allocated on given node (%d != %d)\n", node, maxnode);
  }

  ptr3 = mami_malloc(memory_manager, 100, MAMI_MEMBIND_POLICY_SPECIFIC_NODE, 1);
  mami_select_node(memory_manager, MAMI_LEAST_LOADED_NODE, &least_loaded_node);
  mami_membind(memory_manager, MAMI_MEMBIND_POLICY_LEAST_LOADED_NODE, 0);
  ptr4 = mami_malloc(memory_manager, 100, MAMI_MEMBIND_POLICY_DEFAULT, 0);
  mami_locate(memory_manager, ptr4, 100, &node);
  if (node == least_loaded_node) {
    printf("Memory allocated on least loaded node\n");
  }
  else {
    printf("Error. Memory NOT allocated on least loaded node (%d != %d)\n", node, least_loaded_node);
  }

  mami_free(memory_manager, ptr);
  mami_free(memory_manager, ptr2);
  mami_free(memory_manager, ptr3);
  mami_free(memory_manager, ptr4);

  mami_exit(&memory_manager);
  common_exit(NULL);
  return 0;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
