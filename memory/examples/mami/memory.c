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

//#define PRINT
#define PAGES 2
mami_manager_t *memory_manager;

any_t memory(any_t arg) {
  int *b, *c, *d, *e;
  char *buffer;
  int node, id;

  id = marcel_self()->id;
  b = mami_malloc(memory_manager, 100*sizeof(int), MAMI_MEMBIND_POLICY_SPECIFIC_NODE, id);
  c = mami_malloc(memory_manager, 100*sizeof(int), MAMI_MEMBIND_POLICY_SPECIFIC_NODE, id);
  d = mami_malloc(memory_manager, 100*sizeof(int), MAMI_MEMBIND_POLICY_SPECIFIC_NODE, id);
  e = mami_malloc(memory_manager, 100*sizeof(int), MAMI_MEMBIND_POLICY_SPECIFIC_NODE, id);
  buffer = mami_calloc(memory_manager, 1, PAGES * getpagesize(), MAMI_MEMBIND_POLICY_SPECIFIC_NODE, id);

  mami_locate(memory_manager, &(buffer[0]), 0, &node);
  if (node == id) {
    marcel_printf("[%d] Address is located on the correct node %d\n", id, node);
  }
  else {
    marcel_printf("[%d] Address is NOT located on the correct node but on node %d\n", id, node);
  }

#ifdef PRINT
  mami_print(memory_manager);
#endif /* PRINT */

  mami_free(memory_manager, c);
  mami_free(memory_manager, b);
  mami_free(memory_manager, d);
  mami_free(memory_manager, e);
  mami_free(memory_manager, buffer);
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
      buffers[node] = mami_malloc(memory_manager, i*getpagesize(), MAMI_MEMBIND_POLICY_SPECIFIC_NODE, node);
    for(node=0 ; node<maxnode ; node++)
      mami_free(memory_manager, buffers[node]);
  }
  for(node=0 ; node<maxnode ; node++) {
    buffers[node] = mami_malloc(memory_manager, getpagesize(), MAMI_MEMBIND_POLICY_SPECIFIC_NODE, node);
    buffers2[node] = mami_malloc(memory_manager, getpagesize(), MAMI_MEMBIND_POLICY_SPECIFIC_NODE, node);
  }
  for(node=0 ; node<maxnode ; node++) {
    mami_free(memory_manager, buffers[node]);
    mami_free(memory_manager, buffers2[node]);
  }
  free(buffers);
  free(buffers2);
  return 0;
}

void memory_bis(void) {
  mami_manager_t *memory_manager;
  void *b[7];
  int i;

  mami_init(&memory_manager);
  memory_manager->initially_preallocated_pages = 2;
  b[0] = mami_malloc(memory_manager, 1*getpagesize(), MAMI_MEMBIND_POLICY_DEFAULT, 0);
  b[1] = mami_malloc(memory_manager, 1*getpagesize(), MAMI_MEMBIND_POLICY_DEFAULT, 0);
  b[2] = mami_malloc(memory_manager, 2*getpagesize(), MAMI_MEMBIND_POLICY_DEFAULT, 0);
  b[3] = mami_malloc(memory_manager, 2*getpagesize(), MAMI_MEMBIND_POLICY_DEFAULT, 0);
  b[4] = mami_malloc(memory_manager, 1*getpagesize(), MAMI_MEMBIND_POLICY_DEFAULT, 0);
  b[5] = mami_malloc(memory_manager, 1*getpagesize(), MAMI_MEMBIND_POLICY_DEFAULT, 0);
  b[6] = mami_malloc(memory_manager, 1*getpagesize(), MAMI_MEMBIND_POLICY_DEFAULT, 0);

  for(i=0 ; i<7 ; i++) mami_free(memory_manager, b[i]);
  mami_exit(&memory_manager);
}

int marcel_main(int argc, char * argv[]) {
  marcel_t threads[2];
  marcel_attr_t attr;

  marcel_init(&argc,argv);
  mami_init(&memory_manager);
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
    marcel_attr_setid(&attr, marcel_nbvps()-1);
    marcel_attr_settopo_level(&attr, &marcel_topo_vp_level[marcel_nbvps()-1]);
    marcel_create(&threads[1], &attr, memory2, NULL);
    marcel_join(threads[1], NULL);
  }

  mami_exit(&memory_manager);

  memory_bis();

  // Finish marcel
  marcel_end();
  return 0;
}

#else
int marcel_main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
