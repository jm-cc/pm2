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

#if !defined(MARCEL_MAMI_ENABLED)
#error This application needs MAMI to be enabled
#else

#define SIZE  100
#define LOOPS 100000

marcel_memory_manager_t memory_manager;
int **buffers;
int **buffers;

static inline unsigned long long get_cycles(void) {
#if defined __i386__ || defined __x86_64__
  unsigned int l,h;
  __asm__ __volatile__ ("rdtsc": "=a" (l), "=d" (h));
  return l + (((unsigned long long)h) << 32);
#elif defined __ia64__
  unsigned long long t;
  __asm__ __volatile__ ("mov %0=ar.itc" : "=r" (t));
  return t;
#else
#error Only i386, x86_64 and ia64 supporte so far
#endif
}

static inline unsigned long long get_cycles_per_second(void) {
  unsigned long long start, end;
  start = get_cycles();
  sleep(1);
  end = get_cycles();
  return end - start;
}

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
}

any_t reader(any_t arg) {
  int *buffer;
  int i, j, node;//, where;
  unsigned int gold = 1.457869;

  node = marcel_self()->id;
  buffer = buffers[node];

  // TODO: faire un access aleatoire au lieu de lineaire

  for(j=0 ; j<LOOPS ; j++) {
    for(i=0 ; i<SIZE ; i++) {
      __builtin_prefetch((void*)&buffer[i]);
      __builtin_ia32_clflush((void*)&buffer[i]);
    }
  }
}

int marcel_main(int argc, char * argv[]) {
  marcel_t thread;
  marcel_attr_t attr;
  int t, node;
  unsigned long long **rtimes, **wtimes;
  unsigned long long cycles_per_second;

  cycles_per_second = get_cycles_per_second();
  printf("%lld cycles per second\n", cycles_per_second);

  marcel_init(&argc,argv);
  marcel_memory_init(&memory_manager, 1000);
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
    buffers[node] = marcel_memory_allocate_on_node(&memory_manager, SIZE*sizeof(int), node);
  }

  // Create a thread on node t to work on memory allocated on node node
  for(t=0 ; t<marcel_nbnodes ; t++) {
    for(node=0 ; node<marcel_nbnodes ; node++) {
      unsigned long long start, end;
      start = get_cycles();

      marcel_attr_setid(&attr, node);
      marcel_attr_settopo_level(&attr, &marcel_topo_node_level[t]);
      marcel_create(&thread, &attr, writer, NULL);

      // Wait for the thread to complete
      marcel_join(thread, NULL);

      end = get_cycles();
      wtimes[node][t] = end-start;
    }
  }

  // Create a thread on node t to work on memory allocated on node node
  for(t=0 ; t<marcel_nbnodes ; t++) {
    for(node=0 ; node<marcel_nbnodes ; node++) {
      unsigned long long start, end;
      start = get_cycles();

      marcel_attr_setid(&attr, node);
      marcel_attr_settopo_level(&attr, &marcel_topo_node_level[t]);
      marcel_create(&thread, &attr, reader, NULL);

      // Wait for the thread to complete
      marcel_join(thread, NULL);

      end = get_cycles();
      rtimes[node][t] = end-start;
    }
  }

  printf("Thread\tNode\tBytes\tReader Cycles\tReader Seconds\tReader MB/s\tWriter Cycles\tWriter Seconds\tWriter MB/s\n");
  for(t=0 ; t<marcel_nbnodes ; t++) {
    for(node=0 ; node<marcel_nbnodes ; node++) {
      printf("%d\t%d\t%lld\t%lld\t%f\t%f\t%lld\t%f\t%f\n",
             t, node, LOOPS*SIZE*4,
             rtimes[node][t], 
             (((float)(rtimes[node][t])) / ((float)cycles_per_second)),
             ((float)LOOPS*SIZE*4) / (((float)(rtimes[node][t])) / ((float)cycles_per_second)) / 1000000,
             wtimes[node][t], 
             (((float)(wtimes[node][t])) / ((float)cycles_per_second)),
             ((float)LOOPS*SIZE*4) / (((float)(wtimes[node][t])) / ((float)cycles_per_second)) / 1000000);
    }
  }

  // Finish marcel
  marcel_memory_exit(&memory_manager);
  marcel_end();
}
#endif
