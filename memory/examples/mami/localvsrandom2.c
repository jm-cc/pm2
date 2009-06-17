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

#define SIZE  100
#define LOOPS 100000
#define CACHE_LINE_SIZE 64

mami_manager_t *memory_manager;
int **buffers;

any_t writer(any_t arg) {
  int *buffer;
  int i, j, *node;
  unsigned int gold = 1.457869;

  node = (int *) arg;
  buffer = buffers[*node];

  // TODO: faire un access aleatoire au lieu de lineaire

  for(j=0 ; j<LOOPS ; j++) {
    for(i=0 ; i<SIZE ; i++) {
      __builtin_ia32_movnti((void*) &buffer[i], gold);
    }
  }
  return 0;
}

any_t reader(any_t arg) {
  int *buffer;
  int i, j, *node;

  node = (int *) arg;
  buffer = buffers[*node];

  // TODO: faire un access aleatoire au lieu de lineaire

  for(j=0 ; j<LOOPS ; j++) {
    for(i=0 ; i<SIZE ; i+=CACHE_LINE_SIZE/4) {
      __builtin_prefetch((void*)&buffer[i]);
      __builtin_ia32_clflush((void*)&buffer[i]);
    }
  }
  return 0;
}

int main(int argc, char * argv[]) {
  th_mami_t thread;
  th_mami_attr_t attr;
  int *args;
  int t, node;
  unsigned long long **rtimes, **wtimes;

  common_init(&argc, argv, NULL);
  mami_init(&memory_manager);
  th_mami_attr_init(&attr);

  rtimes = (unsigned long long **) th_mami_malloc(memory_manager->nb_nodes * sizeof(unsigned long long *));
  wtimes = (unsigned long long **) th_mami_malloc(memory_manager->nb_nodes * sizeof(unsigned long long *));
  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    rtimes[node] = (unsigned long long *) th_mami_malloc(memory_manager->nb_nodes * sizeof(unsigned long long));
    wtimes[node] = (unsigned long long *) th_mami_malloc(memory_manager->nb_nodes * sizeof(unsigned long long));
  }

  args = (int *) th_mami_malloc(memory_manager->nb_nodes * sizeof(int));
  buffers = (int **) th_mami_malloc(memory_manager->nb_nodes * sizeof(int *));
  // Allocate memory on each node
  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    args[node] = node;
    buffers[node] = mami_malloc(memory_manager, SIZE*sizeof(int), MAMI_MEMBIND_POLICY_SPECIFIC_NODE, node);
  }

  // Create a thread on node t to work on memory allocated on node node
  for(t=0 ; t<memory_manager->nb_nodes ; t++) {
    for(node=0 ; node<memory_manager->nb_nodes ; node++) {
      struct timeval tv1, tv2;
      unsigned long us;

      marcel_attr_settopo_level(&attr, &marcel_topo_node_level[t]);

      gettimeofday(&tv1, NULL);
      th_mami_create(&thread, &attr, writer, (any_t) &args[node]);
      // Wait for the thread to complete
      th_mami_join(thread, NULL);
      gettimeofday(&tv2, NULL);

      us = (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);
      wtimes[node][t] = us*1000;
    }
  }

  // Create a thread on node t to work on memory allocated on node node
  for(t=0 ; t<memory_manager->nb_nodes ; t++) {
    for(node=0 ; node<memory_manager->nb_nodes ; node++) {
      struct timeval tv1, tv2;
      unsigned long us;

      marcel_attr_settopo_level(&attr, &marcel_topo_node_level[t]);

      gettimeofday(&tv1, NULL);
      th_mami_create(&thread, &attr, reader, (any_t) &args[node]);
      // Wait for the thread to complete
      th_mami_join(thread, NULL);
      gettimeofday(&tv2, NULL);

      us = (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);
      rtimes[node][t] = us*1000;
    }
  }

  // Deallocate memory on each node
  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    mami_free(memory_manager, buffers[node]);
  }

  printf("Thread\tNode\tBytes\t\tReader (ns)\tCache Line (ns)\tWriter (ns)\tCache Line (ns)\n");
  for(t=0 ; t<memory_manager->nb_nodes ; t++) {
    for(node=0 ; node<memory_manager->nb_nodes ; node++) {
      printf("%d\t%d\t%d\t%lld\t%f\t%lld\t%f\n",
             t, node, LOOPS*SIZE*4,
             rtimes[node][t],
             (float)(rtimes[node][t]) / (float)(LOOPS*SIZE*4) / CACHE_LINE_SIZE,
             wtimes[node][t],
             (float)(wtimes[node][t]) / (float)(LOOPS*SIZE*4) / CACHE_LINE_SIZE);
    }
  }

  mami_exit(&memory_manager);
  common_exit(NULL);
  return 0;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif

