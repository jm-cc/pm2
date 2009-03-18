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

#include "marcel.h"

#if defined(MARCEL_MAMI_ENABLED)

int marcel_main(int argc, char * argv[]) {
  int node, *nodes;
  int err, i;
  void *ptr;
  marcel_memory_manager_t memory_manager;

  marcel_init(&argc,argv);
  marcel_memory_init(&memory_manager);

  ptr = malloc(1000);
  err = marcel_memory_distribute(&memory_manager, ptr, NULL, 0);
  free(ptr);
  if (err < 0) perror("marcel_memory_distribute successfully failed");

  ptr = marcel_memory_malloc(&memory_manager, 50000, MARCEL_MEMORY_MEMBIND_POLICY_SPECIFIC_NODE, 1);

  marcel_memory_locate(&memory_manager, ptr, 50000, &node);
  if (node == 1)
    marcel_fprintf(stderr, "Node is %d as expected\n", node);
  else
    marcel_fprintf(stderr, "Node is NOT 1 as expected but %d\n", node);

  nodes = malloc(sizeof(int) * marcel_nbnodes);
  for(i=0 ; i<marcel_nbnodes ; i++) nodes[i] = i;
  err = marcel_memory_distribute(&memory_manager, ptr, nodes, marcel_nbnodes);
  if (err < 0) perror("marcel_memory_distribute unexpectedly failed");
  else {
    marcel_memory_locate(&memory_manager, ptr, 50000, &node);
    if (node == -1)
      marcel_fprintf(stderr, "Node is %d as expected\n", node);
    else
      marcel_fprintf(stderr, "Node is NOT -1 as expected but %d\n", node);

    marcel_memory_gather(&memory_manager, ptr, 0);
    marcel_memory_locate(&memory_manager, ptr, 50000, &node);
    if (node == 0)
      marcel_fprintf(stderr, "Node is %d as expected\n", node);
    else
      marcel_fprintf(stderr, "Node is NOT 0 as expected but %d\n", node);

    marcel_memory_check_pages_location(&memory_manager, ptr, 50000, 0);
  }

  free(nodes);
  marcel_memory_free(&memory_manager, ptr);
  marcel_memory_exit(&memory_manager);

  // Finish marcel
  marcel_end();
  return 0;
}

#else
int marcel_main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MAMI to be enabled\n");
}
#endif
