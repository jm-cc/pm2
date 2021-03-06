/* MaMI --- NUMA Memory Interface
 *
 * Copyright (C) 2008, 2009, 2010, 2011  Centre National de la Recherche Scientifique
 *
 * MaMI is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * MaMI is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU Lesser General Public License in COPYING.LGPL for more details.
 */
#include <stdio.h>
#include <sys/time.h>
#include "mami.h"
#include "mami_private.h"
#include "helper.h"

#if defined(MAMI_ENABLED)

#define SIZE  100
#define LOOPS 100000
#define CACHE_LINE_SIZE 64

mami_manager_t *memory_manager;
int **buffers;

any_t writer(any_t arg);
any_t reader(any_t arg);

int main(int argc, char * argv[]) {
  th_mami_t thread;
  th_mami_attr_t attr;
  int *args;
  int t, node;
  unsigned long long **rtimes, **wtimes;

  mami_init(&memory_manager, argc, argv);
  th_mami_attr_init(&attr);

  rtimes = (unsigned long long **) malloc(memory_manager->nb_nodes * sizeof(unsigned long long *));
  wtimes = (unsigned long long **) malloc(memory_manager->nb_nodes * sizeof(unsigned long long *));
  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    rtimes[node] = (unsigned long long *) malloc(memory_manager->nb_nodes * sizeof(unsigned long long));
    wtimes[node] = (unsigned long long *) malloc(memory_manager->nb_nodes * sizeof(unsigned long long));
  }

  args = (int *) malloc(memory_manager->nb_nodes * sizeof(int));
  buffers = (int **) malloc(memory_manager->nb_nodes * sizeof(int *));
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

      th_mami_attr_setnode_level(&attr, t);

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

      th_mami_attr_setnode_level(&attr, t);

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

  mprintf(stdout, "Thread\tNode\tBytes\t\tReader (ns)\tCache Line (ns)\tWriter (ns)\tCache Line (ns)\n");
  for(t=0 ; t<memory_manager->nb_nodes ; t++) {
    for(node=0 ; node<memory_manager->nb_nodes ; node++) {
      mprintf(stdout, "%d\t%d\t%d\t%lld\t%f\t%lld\t%f\n",
              t, node, LOOPS*SIZE*4,
              rtimes[node][t],
              (float)(rtimes[node][t]) / (float)(LOOPS*SIZE*4) / CACHE_LINE_SIZE,
              wtimes[node][t],
              (float)(wtimes[node][t]) / (float)(LOOPS*SIZE*4) / CACHE_LINE_SIZE);
    }
  }

  mami_exit(&memory_manager);
  return 0;
}

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

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
  return MAMI_TEST_SKIPPED;
}
#endif

