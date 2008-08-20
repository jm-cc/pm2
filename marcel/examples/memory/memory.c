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

#include <sys/mman.h>
#include <numaif.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "marcel.h"

any_t memory2(any_t arg) {
  int *c;
  int i, node;

  c = malloc(sizeof(char));
  marcel_memory_locate(&memory_manager, memory_manager.root, c, &node);
  printf("Address %p is located on node %d\n", c, node);
  free(c);

  {
    void **buffers;
    void **buffers2;
    int maxnode = marcel_nbnodes;
    buffers = malloc(maxnode * sizeof(void *));
    buffers2 = malloc(maxnode * sizeof(void *));
    for(i=1 ; i<=5 ; i++) {
      for(node=0 ; node<maxnode ; node++)
	buffers[node] = marcel_memory_allocate_on_node(&memory_manager, i*memory_manager.pagesize, node);
      for(node=0 ; node<maxnode ; node++)
      	marcel_memory_free(&memory_manager, buffers[node]);
    }
    for(node=0 ; node<maxnode ; node++) {
      buffers[node] = marcel_memory_allocate_on_node(&memory_manager, memory_manager.pagesize, node);
      buffers2[node] = marcel_memory_allocate_on_node(&memory_manager, memory_manager.pagesize, node);
    }
    for(node=0 ; node<maxnode ; node++) {
      marcel_memory_free(&memory_manager, buffers[node]);
      marcel_memory_free(&memory_manager, buffers2[node]);
    }
    free(buffers);
    free(buffers2);
  }
}

void memory_bis() {
  marcel_memory_manager_t memory_manager;
  void *b;

  marcel_memory_init(&memory_manager, 2);
  b = marcel_memory_malloc(&memory_manager, 1*memory_manager.pagesize);
  b = marcel_memory_malloc(&memory_manager, 1*memory_manager.pagesize);
  b = marcel_memory_malloc(&memory_manager, 2*memory_manager.pagesize);
  b = marcel_memory_malloc(&memory_manager, 2*memory_manager.pagesize);
  b = marcel_memory_malloc(&memory_manager, 1*memory_manager.pagesize);
  b = marcel_memory_malloc(&memory_manager, 1*memory_manager.pagesize);
  b = marcel_memory_malloc(&memory_manager, 1*memory_manager.pagesize);
}

int marcel_main(int argc, char * argv[]) {
  marcel_t threads[2];
  marcel_attr_t attr;

  marcel_init(&argc,argv);
  marcel_memory_init(&memory_manager, 1000);
  marcel_attr_init(&attr);

  // Start the 1st thread on the first VP
  marcel_attr_setid(&attr, 0);
  marcel_attr_settopo_level(&attr, &marcel_topo_vp_level[0]);
  marcel_create(&threads[0], &attr, memory, NULL);

  // Start the 2nd thread on the last VP
  marcel_attr_setid(&attr, 1);
  marcel_attr_settopo_level(&attr, &marcel_topo_vp_level[marcel_nbvps()-1]);
  marcel_create(&threads[1], &attr, memory, NULL);

  // Wait for the threads to complete
  marcel_join(threads[0], NULL);
  marcel_join(threads[1], NULL);

  // Start the thread on the last VP
  marcel_attr_setid(&attr, 1);
  marcel_attr_settopo_level(&attr, &marcel_topo_vp_level[marcel_nbvps()-1]);
  marcel_create(&threads[1], &attr, memory2, NULL);
  marcel_join(threads[1], NULL);

  memory_bis();

  // Finish marcel
  marcel_end();
}

// TODO: use memalign
