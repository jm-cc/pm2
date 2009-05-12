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

#define SIZE  100000
#define LOOPS 100000

mami_manager_t memory_manager;
int **buffers;

any_t compute(any_t arg) {
  int *buffer;
  int i, j, node, where;

  node = marcel_self()->id;

  marcel_fprintf(stderr,"[%d] launched on Node #%d - VP #%u\n", marcel_self()->id, marcel_current_node(), marcel_current_vp());
  buffer = buffers[node];

  mami_locate(&memory_manager, buffer, 0, &where);
  marcel_fprintf(stderr,"[%d] Memory on node #%d\n", marcel_self()->id, where);

  for(j=0 ; j<LOOPS ; j++) {
    for(i=0 ; i<SIZE ; i++) {
      buffer[i] ++;
    }
  }
  return 0;
}

int marcel_main(int argc, char * argv[]) {
  marcel_t *threads;
  marcel_attr_t attr;
  marcel_attr_t attr2;
  int node;

  struct timeval random_tv1, random_tv2;
  struct timeval local_tv1, local_tv2;
  unsigned long local_us, local_ns;
  unsigned long random_us, random_ns;
  unsigned long benefit;

  marcel_init(&argc,argv);
  mami_init(&memory_manager);
  marcel_attr_init(&attr);
  marcel_attr_init(&attr2);

  buffers = (int **) malloc(marcel_nbnodes * sizeof(int *));
  threads = (marcel_t *) malloc(marcel_nbnodes * sizeof(marcel_t));

  // Allocate memory on each node
  for(node=0 ; node<marcel_nbnodes ; node++) {
    buffers[node] = mami_malloc(&memory_manager, SIZE*sizeof(int), MAMI_MEMBIND_POLICY_SPECIFIC_NODE, node);
  }

  // The thread which will work on the memory allocated on the node N is started on the same node N
  gettimeofday(&local_tv1, NULL);
  for(node=0 ; node<marcel_nbnodes ; node++) {
    marcel_attr_setid(&attr, node);
    marcel_attr_settopo_level(&attr, &marcel_topo_node_level[node]);
    marcel_create(&threads[node], &attr, compute, NULL);
  }

  // Wait for the threads to complete
  for(node=0 ; node<marcel_nbnodes ; node++) {
    marcel_join(threads[node], NULL);
  }
  gettimeofday(&local_tv2, NULL);

  // Let marcel decide where to place the threads
  gettimeofday(&random_tv1, NULL);
  for(node=0 ; node<marcel_nbnodes ; node++) {
    marcel_attr_setid(&attr2, node);
    marcel_create(&threads[node], &attr2, compute, NULL);
  }

  // Wait for the threads to complete
  for(node=0 ; node<marcel_nbnodes ; node++) {
    marcel_join(threads[node], NULL);
  }
  gettimeofday(&random_tv2, NULL);

  // Deallocate memory on each node
  for(node=0 ; node<marcel_nbnodes ; node++) {
    mami_free(&memory_manager, buffers[node]);
  }

  local_us = (local_tv2.tv_sec - local_tv1.tv_sec) * 1000000 + (local_tv2.tv_usec - local_tv1.tv_usec);
  local_ns = local_us * 1000;
  random_us = (random_tv2.tv_sec - random_tv1.tv_sec) * 1000000 + (random_tv2.tv_usec - random_tv1.tv_usec);
  random_ns = random_us * 1000;

  benefit = local_ns / random_ns * 100;

  marcel_printf("%ld\t%ld\t%ld\n", local_ns, random_ns, benefit);

  // Finish marcel
  mami_exit(&memory_manager);
  marcel_end();
  return 0;
}

#else
int marcel_main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif