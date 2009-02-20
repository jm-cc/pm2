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
#include <sys/mman.h>
#include <numaif.h>

#define MAX_PAGES	10000
#define LOOPS           1000

marcel_barrier_t allbarrier, barrier, endbarrier;
void *buffer0, *buffer1;
unsigned long node0, node1;
void **pageaddrs0, **pageaddrs1;
int *status0, *status1, *node0s, *node1s;

void check_location(void **pageaddrs, int pages, int node) {
  int *pagenodes;
  int i;
  int err;

  pagenodes = malloc(pages * sizeof(int));
  err = move_pages(0, pages, pageaddrs, NULL, pagenodes, 0);
  if (err < 0) {
    perror("move_pages (check_location)");
    exit(-1);
  }

  for(i=0; i<pages; i++) {
    if (pagenodes[i] != node) {
      printf("  page #%d is not located on node #%d\n", i, node);
      exit(-1);
    }
  }
  free(pagenodes);
}

void migrate(void **pageaddrs, int pages, int *nodes, int *status) {
  int err;

  //marcel_printf("binding on numa node #%d\n", nodes[0]);

  err = move_pages(0, pages, pageaddrs, nodes, status, MPOL_MF_MOVE);

  if (err < 0) {
    perror("move_pages (set_bind)");
    exit(-1);
  }
}

any_t congestion0(any_t arg) {
  int i;

  /* Synchronise all the threads */
  marcel_barrier_init(&barrier, NULL, 2);
  marcel_barrier_wait(&allbarrier);

  for(i=0 ; i<LOOPS ; i++) {
    if (i%2 == 0) {
      migrate(pageaddrs0, MAX_PAGES, node1s, status0);
    }
    else {
      migrate(pageaddrs1, MAX_PAGES, node1s, status0);
    }
    marcel_barrier_wait(&barrier);
  }

  marcel_barrier_wait(&endbarrier);
}

any_t congestion1(any_t arg) {
  int i;

  /* Synchronise all the threads */
  marcel_barrier_wait(&allbarrier);

  for(i=0 ; i<LOOPS ; i++) {
    if (i%2 == 0) {
      migrate(pageaddrs1, MAX_PAGES, node0s, status1);
    }
    else {
      migrate(pageaddrs0, MAX_PAGES, node0s, status1);
    }
    marcel_barrier_wait(&barrier);
  }

  marcel_barrier_wait(&endbarrier);
}

int marcel_main(int argc, char * argv[]) {
  marcel_t threads[2];
  marcel_attr_t attr;
  unsigned long pagesize;
  unsigned long nodemask;
  unsigned long maxnode;
  int i, err;
  struct timeval tv1, tv2;
  unsigned long us, ns;

  marcel_init(&argc,argv);
  marcel_attr_init(&attr);

  marcel_barrier_init(&allbarrier, NULL, 3);
  marcel_barrier_init(&endbarrier, NULL, 3);

  node0 = 1;
  node1 = 2;
  maxnode = numa_max_node();

  // Start the 1st thread on node0
  marcel_attr_setid(&attr, 0);
  marcel_attr_settopo_level(&attr, &marcel_topo_node_level[node0]);
  marcel_create(&threads[0], &attr, congestion0, NULL);

  // Start the 2nd thread on the last VP
  marcel_attr_setid(&attr, 1);
  marcel_attr_settopo_level(&attr, &marcel_topo_node_level[node1]);
  marcel_create(&threads[1], &attr, congestion1, NULL);

  // Allocate the buffers
  pagesize = getpagesize();
  buffer0 = mmap(NULL, MAX_PAGES * pagesize, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
  if (buffer0 < 0) {
    perror("mmap");
    exit(-1);
  }
  buffer1 = mmap(NULL, MAX_PAGES * pagesize, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
  if (buffer1 < 0) {
    perror("mmap");
    exit(-1);
  }

  // Set the buffers on the nodes
  nodemask = (1<<node0);
  err = set_mempolicy(MPOL_BIND, &nodemask, maxnode+2);
  if (err < 0) {
    perror("set_mempolicy");
    exit(-1);
  }
  memset(buffer0, 0, MAX_PAGES*pagesize);
  nodemask = (1<<node1);
  err = set_mempolicy(MPOL_BIND, &nodemask, maxnode+2);
  if (err < 0) {
    perror("set_mempolicy");
    exit(-1);
  }
  memset(buffer1, 0, MAX_PAGES*pagesize);

  // Set the page addresses
  pageaddrs0 = malloc(MAX_PAGES * sizeof(void *));
  for(i=0; i<MAX_PAGES; i++)
    pageaddrs0[i] = buffer0 + i*pagesize;
  pageaddrs1 = malloc(MAX_PAGES * sizeof(void *));
  for(i=0; i<MAX_PAGES; i++)
    pageaddrs1[i] = buffer1 + i*pagesize;

  // Set the other variables
  status0 = malloc(MAX_PAGES * sizeof(int));
  status1 = malloc(MAX_PAGES * sizeof(int));
  node0s = malloc(MAX_PAGES * sizeof(int));
  node1s = malloc(MAX_PAGES * sizeof(int));
  for(i=0; i<MAX_PAGES ; i++) node0s[i] = node0;
  for(i=0; i<MAX_PAGES ; i++) node1s[i] = node1;

  // Check the location of the buffers
  check_location(pageaddrs0, MAX_PAGES, node0);
  check_location(pageaddrs1, MAX_PAGES, node1);

  /* threads places */
  marcel_barrier_wait(&allbarrier);

  gettimeofday(&tv1, NULL);
  marcel_barrier_wait(&endbarrier);
  gettimeofday(&tv2, NULL);

  us = (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);
  ns = us * 1000;
  ns /= (LOOPS);

  marcel_printf("%ld\t%ld\t%ld\n", node0, node1, ns);

  // Wait for the threads to complete
  marcel_join(threads[0], NULL);
  marcel_join(threads[1], NULL);

  // Finish marcel
  marcel_end();
}

// résultats pour kwak:
// 1       3       150775108
// 1       2       142785693
