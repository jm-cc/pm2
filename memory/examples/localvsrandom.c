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

#define SIZE  10000
#define LOOPS 10000

mami_manager_t *memory_manager;
int **buffers;

any_t compute(any_t arg) {
  int *buffer;
  int i, j, *node, where;

  node = (int *) arg;

  fprintf(stderr,"[%d] launched on Node #%d\n", *node, th_mami_current_node());
  buffer = buffers[*node];

  mami_locate(memory_manager, buffer, 0, &where);
  if (where != *node) {
    fprintf(stderr,"[%d] Memory not located on node #%d\n", *node, where);
    exit(1);
  }

  for(j=0 ; j<LOOPS ; j++) {
    for(i=0 ; i<SIZE ; i++) {
      buffer[i] ++;
    }
  }
  return 0;
}

int main(int argc, char * argv[]) {
  th_mami_t *threads;
  th_mami_attr_t attr;
  int node;
  int *args;

  struct timeval random_tv1, random_tv2;
  struct timeval local_tv1, local_tv2;
  unsigned long local_us, local_ns;
  unsigned long random_us, random_ns;
  unsigned long benefit;

  common_init(&argc, argv, NULL);
  mami_init(&memory_manager, argc, argv);
  th_mami_attr_init(&attr);

  args = (int *) malloc(memory_manager->nb_nodes * sizeof(int));
  buffers = (int **) malloc(memory_manager->nb_nodes * sizeof(int *));
  threads = (th_mami_t *) malloc(memory_manager->nb_nodes * sizeof(th_mami_t));

  // Allocate memory on each node
  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    args[node] = node;
    buffers[node] = mami_malloc(memory_manager, SIZE*sizeof(int), MAMI_MEMBIND_POLICY_SPECIFIC_NODE, node);
  }

  // The thread which will work on the memory allocated on the node N is started on the same node N
  gettimeofday(&local_tv1, NULL);
  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    th_mami_attr_setnode_level(&attr, node);
    th_mami_create(&threads[node], &attr, compute, (any_t) &args[node]);
  }

  // Wait for the threads to complete
  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    th_mami_join(threads[node], NULL);
  }
  gettimeofday(&local_tv2, NULL);

  // Randomly place the threads
  gettimeofday(&random_tv1, NULL);
  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    th_mami_create(&threads[node], NULL, compute, (any_t) &args[node]);
  }

  // Wait for the threads to complete
  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    th_mami_join(threads[node], NULL);
  }
  gettimeofday(&random_tv2, NULL);

  // Deallocate memory on each node
  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    mami_free(memory_manager, buffers[node]);
  }

  local_us = (local_tv2.tv_sec - local_tv1.tv_sec) * 1000000 + (local_tv2.tv_usec - local_tv1.tv_usec);
  local_ns = local_us * 1000;
  random_us = (random_tv2.tv_sec - random_tv1.tv_sec) * 1000000 + (random_tv2.tv_usec - random_tv1.tv_usec);
  random_ns = random_us * 1000;

  benefit = local_ns / random_ns * 100;

  printf("%ld\t%ld\t%ld\n", local_ns, random_ns, benefit);

  mami_exit(&memory_manager);
  common_exit(NULL);
  return 0;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
