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

#if defined(MM_MAMI_ENABLED)

int main(int argc, char * argv[]) {
  mami_manager_t *memory_manager;
  int anode, bnode;
  void *ptr;
  size_t size;

  marcel_init(&argc,argv);
  mami_init(&memory_manager);

  mami_select_node(memory_manager, MAMI_LEAST_LOADED_NODE, &anode);
  size = marcel_topo_node_level[anode].memory_kB[MARCEL_TOPO_LEVEL_MEMORY_NODE]*1024/2;
  ptr = mami_malloc(memory_manager, size, MAMI_MEMBIND_POLICY_SPECIFIC_NODE, anode);

  mami_select_node(memory_manager, MAMI_LEAST_LOADED_NODE, &bnode);
  if (anode != bnode) {
    marcel_printf("Success\n");
  }
  else {
    marcel_printf("The least loaded anode is: %d\n", anode);
    marcel_printf("The least loaded bnode is: %d\n", bnode);
  }

  // Finish marcel
  mami_free(memory_manager, ptr);
  mami_exit(&memory_manager);
  marcel_end();
  return 0;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
