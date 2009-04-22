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

#include "mami.h"

#if defined(MAMI_ENABLED)

int marcel_main(int argc, char * argv[]) {
  int node, least_loaded_node, maxnode;
  void *ptr, *ptr2, *ptr3, *ptr4;
  marcel_memory_manager_t memory_manager;

  marcel_init(&argc,argv);
  marcel_memory_init(&memory_manager);

  ptr = marcel_memory_malloc(&memory_manager, 100, MARCEL_MEMORY_MEMBIND_POLICY_DEFAULT, 0);
  marcel_memory_locate(&memory_manager, ptr, 100, &node);
  if (node == marcel_current_node()) {
    marcel_printf("Memory allocated on current node\n");
  }
  else {
    marcel_printf("Error. Memory NOT allocated on current node (%d != %d)\n", node, marcel_current_node());
  }

  maxnode = marcel_nbnodes-1;
  marcel_memory_membind(&memory_manager, MARCEL_MEMORY_MEMBIND_POLICY_SPECIFIC_NODE, maxnode);
  ptr2 = marcel_memory_malloc(&memory_manager, 100, MARCEL_MEMORY_MEMBIND_POLICY_DEFAULT, 0);
  marcel_memory_locate(&memory_manager, ptr2, 100, &node);
  if (node == maxnode) {
    marcel_printf("Memory allocated on given node\n");
  }
  else {
    marcel_printf("Error. Memory NOT allocated on given node (%d != %d)\n", node, maxnode);
  }

  ptr3 = marcel_memory_malloc(&memory_manager, 100, MARCEL_MEMORY_MEMBIND_POLICY_SPECIFIC_NODE, 1);
  marcel_memory_select_node(&memory_manager, MARCEL_MEMORY_LEAST_LOADED_NODE, &least_loaded_node);
  marcel_memory_membind(&memory_manager, MARCEL_MEMORY_MEMBIND_POLICY_LEAST_LOADED_NODE, 0);
  ptr4 = marcel_memory_malloc(&memory_manager, 100, MARCEL_MEMORY_MEMBIND_POLICY_DEFAULT, 0);
  marcel_memory_locate(&memory_manager, ptr4, 100, &node);
  if (node == least_loaded_node) {
    marcel_printf("Memory allocated on least loaded node\n");
  }
  else {
    marcel_printf("Error. Memory NOT allocated on least loaded node (%d != %d)\n", node, least_loaded_node);
  }

  marcel_memory_free(&memory_manager, ptr);
  marcel_memory_free(&memory_manager, ptr2);
  marcel_memory_free(&memory_manager, ptr3);
  marcel_memory_free(&memory_manager, ptr4);
  marcel_memory_exit(&memory_manager);

  // Finish marcel
  marcel_end();
  return 0;
}

#else
int marcel_main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
