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

#if !defined(MARCEL_NUMA)
#error This application needs NUMA to be enabled
#else

#define SIZE  100000
#define LOOPS 100000

marcel_memory_manager_t memory_manager;
int **buffers;
int **buffers;

any_t compute(any_t arg) {
  int *buffer;
  int i, j, node;//, where;

  node = marcel_self()->id;

  //  marcel_fprintf(stderr,"[%d] launched on Node #%d - VP #%u\n", marcel_self()->id, marcel_current_node(), marcel_current_vp());
  buffer = buffers[node];

  //marcel_memory_locate(&memory_manager, buffer, &where);
  //marcel_fprintf(stderr,"[%d] Memory on node #%d\n", marcel_self()->id, where);

  // TODO: faire un access aleatoire au lieu de lineaire
  // TODO: utiliser de l'assembleur ...

  for(j=0 ; j<LOOPS ; j++) {
    for(i=0 ; i<SIZE ; i++) {
      buffer[i] ++;
    }
  }
}

int marcel_main(int argc, char * argv[]) {
  marcel_t thread;
  marcel_attr_t attr;
  int t, node;

  struct timeval tv1, tv2;
  unsigned long us, ns;

  marcel_init(&argc,argv);
  marcel_memory_init(&memory_manager, 1000);
  marcel_attr_init(&attr);

  buffers = (int **) malloc(marcel_nbnodes * sizeof(int *));

  // Allocate memory on each node
  for(node=0 ; node<marcel_nbnodes ; node++) {
    buffers[node] = marcel_memory_allocate_on_node(&memory_manager, SIZE*sizeof(int), node);
  }

  // Create a thread on node t to work on memory allocated on node node
  for(t=0 ; t<marcel_nbnodes ; t++) {
    for(node=0 ; node<marcel_nbnodes ; node++) {
      gettimeofday(&tv1, NULL);
      marcel_attr_setid(&attr, node);
      marcel_attr_settopo_level(&attr, &marcel_topo_node_level[t]);
      marcel_create(&thread, &attr, compute, NULL);

      // Wait for the thread to complete
      marcel_join(thread, NULL);

      gettimeofday(&tv2, NULL);

      us = (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);
      ns = us * 1000;
      printf("Thread #%d\tNode #%d\t%ld\n", t, node, ns);
    }
  }

  // Finish marcel
  marcel_memory_exit(&memory_manager);
  marcel_end();
}
#endif
