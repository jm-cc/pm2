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

//#define PRINT
#define PAGES 2
marcel_memory_manager_t memory_manager;

any_t memory(any_t arg) {
  int *b, *c, *d, *e;
  char *buffer;
  int node;

  b = marcel_memory_malloc(&memory_manager, 100*sizeof(int));
  c = marcel_memory_malloc(&memory_manager, 100*sizeof(int));
  d = marcel_memory_malloc(&memory_manager, 100*sizeof(int));
  e = marcel_memory_malloc(&memory_manager, 100*sizeof(int));
  buffer = marcel_memory_calloc(&memory_manager, 1, PAGES * memory_manager.normalpagesize);

  marcel_memory_locate(&memory_manager, &(buffer[0]), &node);
  if (node == marcel_self()->id) {
    marcel_printf("[%d] Address is located on the correct node %d\n", marcel_self()->id, node);
  }
  else {
    marcel_printf("[%d] Address is NOT located on the correct node %d\n", marcel_self()->id, node);
  }

#ifdef PRINT
  marcel_memory_print(&memory_manager);
#endif /* PRINT */

  marcel_memory_free(&memory_manager, c);
  marcel_memory_free(&memory_manager, b);
  marcel_memory_free(&memory_manager, d);
  marcel_memory_free(&memory_manager, e);
  marcel_memory_free(&memory_manager, buffer);
  return 0;
}

any_t memory2(any_t arg) {
  int i, node;
  void **buffers;
  void **buffers2;
  int maxnode = marcel_nbnodes;

  buffers = malloc(maxnode * sizeof(void *));
  buffers2 = malloc(maxnode * sizeof(void *));
  for(i=1 ; i<=5 ; i++) {
    for(node=0 ; node<maxnode ; node++)
      buffers[node] = marcel_memory_allocate_on_node(&memory_manager, i*memory_manager.normalpagesize, node);
    for(node=0 ; node<maxnode ; node++)
      marcel_memory_free(&memory_manager, buffers[node]);
  }
  for(node=0 ; node<maxnode ; node++) {
    buffers[node] = marcel_memory_allocate_on_node(&memory_manager, memory_manager.normalpagesize, node);
    buffers2[node] = marcel_memory_allocate_on_node(&memory_manager, memory_manager.normalpagesize, node);
  }
  for(node=0 ; node<maxnode ; node++) {
    marcel_memory_free(&memory_manager, buffers[node]);
    marcel_memory_free(&memory_manager, buffers2[node]);
  }
  free(buffers);
  free(buffers2);
  return 0;
}

void memory_bis(void) {
  marcel_memory_manager_t memory_manager;
  void *b[7];
  int i;

  marcel_memory_init(&memory_manager);
  memory_manager.initially_preallocated_pages = 2;
  b[0] = marcel_memory_malloc(&memory_manager, 1*memory_manager.normalpagesize);
  b[1] = marcel_memory_malloc(&memory_manager, 1*memory_manager.normalpagesize);
  b[2] = marcel_memory_malloc(&memory_manager, 2*memory_manager.normalpagesize);
  b[3] = marcel_memory_malloc(&memory_manager, 2*memory_manager.normalpagesize);
  b[4] = marcel_memory_malloc(&memory_manager, 1*memory_manager.normalpagesize);
  b[5] = marcel_memory_malloc(&memory_manager, 1*memory_manager.normalpagesize);
  b[6] = marcel_memory_malloc(&memory_manager, 1*memory_manager.normalpagesize);

  for(i=0 ; i<7 ; i++) marcel_memory_free(&memory_manager, b[i]);
  marcel_memory_exit(&memory_manager);
}

int marcel_main(int argc, char * argv[]) {
  marcel_t threads[2];
  marcel_attr_t attr;

  marcel_init(&argc,argv);
  marcel_memory_init(&memory_manager);
  marcel_attr_init(&attr);

  if (marcel_nbnodes < 2) {
    marcel_printf("This application needs at least two NUMA nodes.\n");
  }
  else {
    // Start the 1st thread on the node #0
    marcel_attr_setid(&attr, 0);
    marcel_attr_settopo_level(&attr, &marcel_topo_node_level[0]);
    marcel_create(&threads[0], &attr, memory, NULL);

    // Start the 2nd thread on the node #1
    marcel_attr_setid(&attr, 1);
    marcel_attr_settopo_level(&attr, &marcel_topo_node_level[1]);
    marcel_create(&threads[1], &attr, memory, NULL);

    // Wait for the threads to complete
    marcel_join(threads[0], NULL);
    marcel_join(threads[1], NULL);

    // Start the thread on the last VP
    marcel_attr_setid(&attr, 2);
    marcel_attr_settopo_level(&attr, &marcel_topo_vp_level[marcel_nbvps()-1]);
    marcel_create(&threads[1], &attr, memory2, NULL);
    marcel_join(threads[1], NULL);
  }

  marcel_memory_exit(&memory_manager);

  memory_bis();

  // Finish marcel
  marcel_end();
  return 0;
}

#else
int marcel_main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MAMI to be enabled\n");
}
#endif
