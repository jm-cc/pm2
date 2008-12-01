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

#define SIZE  100
#define LOOPS 100000
#define CACHE_LINE_SIZE 64

marcel_memory_manager_t memory_manager;
int **buffers;

any_t writer(any_t arg) {
  int *buffer;
  int i, j, node;//, where;
  unsigned int gold = 1.457869;

  node = marcel_self()->id;
  buffer = buffers[node];

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
  int i, j, node;

  node = marcel_self()->id;
  buffer = buffers[node];

  // TODO: faire un access aleatoire au lieu de lineaire

  for(j=0 ; j<LOOPS ; j++) {
    for(i=0 ; i<SIZE ; i+=CACHE_LINE_SIZE/4) {
      __builtin_prefetch((void*)&buffer[i]);
      __builtin_ia32_clflush((void*)&buffer[i]);
    }
  }
  return 0;
}

int marcel_main(int argc, char * argv[]) {
  marcel_t thread;
  marcel_attr_t attr;
  int t, node;
  unsigned long long **rtimes, **wtimes;

  marcel_init(&argc,argv);
  marcel_memory_init(&memory_manager);
  marcel_attr_init(&attr);

  rtimes = (unsigned long long **) malloc(marcel_nbnodes * sizeof(unsigned long long *));
  wtimes = (unsigned long long **) malloc(marcel_nbnodes * sizeof(unsigned long long *));
  for(node=0 ; node<marcel_nbnodes ; node++) {
    rtimes[node] = (unsigned long long *) malloc(marcel_nbnodes * sizeof(unsigned long long));
    wtimes[node] = (unsigned long long *) malloc(marcel_nbnodes * sizeof(unsigned long long));
  }

  buffers = (int **) malloc(marcel_nbnodes * sizeof(int *));
  // Allocate memory on each node
  for(node=0 ; node<marcel_nbnodes ; node++) {
    buffers[node] = marcel_memory_malloc_on_node(&memory_manager, SIZE*sizeof(int), node);
  }

  // Create a thread on node t to work on memory allocated on node node
  for(t=0 ; t<marcel_nbnodes ; t++) {
    for(node=0 ; node<marcel_nbnodes ; node++) {
      struct timeval tv1, tv2;
      unsigned long us;

      marcel_attr_setid(&attr, node);
      marcel_attr_settopo_level(&attr, &marcel_topo_node_level[t]);

      gettimeofday(&tv1, NULL);
      marcel_create(&thread, &attr, writer, NULL);
      // Wait for the thread to complete
      marcel_join(thread, NULL);
      gettimeofday(&tv2, NULL);

      us = (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);
      wtimes[node][t] = us*1000;
    }
  }

  // Create a thread on node t to work on memory allocated on node node
  for(t=0 ; t<marcel_nbnodes ; t++) {
    for(node=0 ; node<marcel_nbnodes ; node++) {
      struct timeval tv1, tv2;
      unsigned long us;

      marcel_attr_setid(&attr, node);
      marcel_attr_settopo_level(&attr, &marcel_topo_node_level[t]);

      gettimeofday(&tv1, NULL);
      marcel_create(&thread, &attr, reader, NULL);
      // Wait for the thread to complete
      marcel_join(thread, NULL);
      gettimeofday(&tv2, NULL);

      us = (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);
      rtimes[node][t] = us*1000;
    }
  }

  // Deallocate memory on each node
  for(node=0 ; node<marcel_nbnodes ; node++) {
    marcel_memory_free(&memory_manager, buffers[node]);
  }

  printf("Thread\tNode\tBytes\t\tReader (ns)\tCache Line (ns)\tWriter (ns)\tCache Line (ns)\n");
  for(t=0 ; t<marcel_nbnodes ; t++) {
    for(node=0 ; node<marcel_nbnodes ; node++) {
      printf("%d\t%d\t%d\t%lld\t%f\t%lld\t%f\n",
             t, node, LOOPS*SIZE*4,
             rtimes[node][t],
             (float)(rtimes[node][t]) / (float)(LOOPS*SIZE*4) / CACHE_LINE_SIZE,
             wtimes[node][t],
             (float)(wtimes[node][t]) / (float)(LOOPS*SIZE*4) / CACHE_LINE_SIZE);
    }
  }

  // Finish marcel
  marcel_memory_exit(&memory_manager);
  marcel_end();
  return 0;
}

#else
int marcel_main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MAMI to be enabled\n");
}
#endif

