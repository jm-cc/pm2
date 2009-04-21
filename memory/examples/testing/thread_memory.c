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
#include "marcel_topology.h"

#include <errno.h>
#include <numaif.h>

#define PAGES 4

typedef struct thread_memory_s {
  void *address;
  size_t size;
  marcel_t thread;
  int thread_id;

  int *nodes;
  unsigned long pagesize;
  void **pageaddrs;
  int nbpages;
} thread_memory_t;

int thread_memory_attach(void *address, size_t size, thread_memory_t* thread_memory) {
  int i, err;

  thread_memory->address = address;
  thread_memory->size = size;
  thread_memory->thread = marcel_self();
  thread_memory->thread_id = thread_memory->thread->id;

  // Count the number of pages
  thread_memory->pagesize = getpagesize();
  thread_memory->nbpages = size / thread_memory->pagesize;
  if (thread_memory->nbpages * thread_memory->pagesize != size) thread_memory->nbpages++;

  // Set the page addresses
  thread_memory->pageaddrs = malloc(thread_memory->nbpages * sizeof(void *));
  for(i=0; i<thread_memory->nbpages ; i++)
    thread_memory->pageaddrs[i] = address + i*thread_memory->pagesize;

  // fill in the nodes
  thread_memory->nodes = malloc(thread_memory->nbpages * sizeof(int));
  err = move_pages(0, thread_memory->nbpages, thread_memory->pageaddrs, NULL, thread_memory->nodes, 0);
  if (err < 0) {
    perror("move_pages");
    exit(-1);
  }

  // Display information
  for(i=0; i<thread_memory->nbpages; i++) {
    if (thread_memory->nodes[i] == -ENOENT)
      marcel_printf("[%d]  page #%d is not allocated\n", thread_memory->thread_id, i);
    else
      marcel_printf("[%d]  page #%d is on node #%d\n", thread_memory->thread_id, i, thread_memory->nodes[i]);
  }

  return 0;
}

int thread_memory_consult(thread_memory_t* thread_memory) {
  int i, err, *status;

  status = malloc(thread_memory->nbpages * sizeof(int));
  err = move_pages(0, thread_memory->nbpages, thread_memory->pageaddrs, NULL, status, 0);
  if (err < 0) {
    perror("move_pages");
    exit(-1);
  }

  // Display information
  for(i=0; i<thread_memory->nbpages; i++) {
    if (status[i] == -ENOENT)
      marcel_printf("[%d]  page #%d is not allocated\n", thread_memory->thread_id, i);
    else
      marcel_printf("[%d]  page #%d is on node #%d\n", thread_memory->thread_id, i, status[i]);
  }
}

int thread_memory_move(thread_memory_t* thread_memory, int node) {
  int i, err;
  int *status;

  status = malloc(thread_memory->nbpages * sizeof(int));
  for(i=0; i<thread_memory->nbpages ; i++) thread_memory->nodes[i] = node;

  err = move_pages(0, thread_memory->nbpages, thread_memory->pageaddrs,
                   thread_memory->nodes, status, MPOL_MF_MOVE);
  if (err < 0) {
    if (errno == ENOENT) {
      printf("[%d] warning. either the pages are not allocated or the pages are already standing at the asked location\n",
             thread_memory->thread_id);
    }
    else {
      perror("move_pages");
      exit(-1);
    }
  }
  else {
    for(i=0; i<PAGES; i++) {
      if (status[i] == -ENOENT)
        printf("[%d] page #%d is not allocated\n", thread_memory->thread_id, i);
      else
        printf("[%d] page #%d is on node #%d\n", thread_memory->thread_id, i, status[i]);
    }
  }
}

int thread_memory_detach(thread_memory_t* thread_memory) {
}

any_t memory(any_t arg) {
  char *buffer;
  int nodes[PAGES];
  thread_memory_t thread_memory;

  marcel_fprintf(stderr,"[%d] launched on VP #%u\n", marcel_self()->id,
                 marcel_current_vp());

  buffer = malloc(PAGES * getpagesize() * sizeof(char));
  memset(buffer, 0, PAGES * getpagesize() * sizeof(char));

  marcel_fprintf(stderr,"[%d] allocating buffer %p of size %u with pagesize %u\n",
                 marcel_self()->id, buffer, PAGES * getpagesize() * sizeof(char),
                 getpagesize());

  thread_memory_attach(buffer, PAGES * getpagesize() * sizeof(char),
                       &thread_memory);
  thread_memory_move(&thread_memory, 0);
}

any_t memory2(any_t arg) {
  char *buffer;
  int nodes[PAGES];
  thread_memory_t thread_memory;

  marcel_fprintf(stderr,"[%d] launched on VP #%u\n", marcel_self()->id,
                 marcel_current_vp());

  buffer = malloc(PAGES * getpagesize() * sizeof(char));

  marcel_fprintf(stderr,"[%d] allocating buffer %p of size %u with pagesize %u\n",
                 marcel_self()->id, buffer, PAGES * getpagesize() * sizeof(char),
                 getpagesize());

  thread_memory_attach(buffer, PAGES * getpagesize() * sizeof(char),
                       &thread_memory);
  thread_memory_move(&thread_memory, 3);
  memset(buffer, 0, PAGES * getpagesize() * sizeof(char));
  thread_memory_consult(&thread_memory);
  thread_memory_move(&thread_memory, 2);
}

int marcel_main(int argc, char * argv[]) {
  marcel_t threads[3];
  marcel_attr_t attr;

  marcel_init(&argc,argv);

  marcel_attr_init(&attr);
  // Start the 1st thread on the first VP
  //  marcel_print_level(&marcel_topo_vp_level[0], stderr, 1, 1, " ", "#", ":", "");
  marcel_attr_setid(&attr, 0);
  marcel_attr_settopo_level(&attr, &marcel_topo_vp_level[0]);
  marcel_create(&threads[0], &attr, memory, (any_t) (intptr_t) 0);

  // Start the 2nd thread on the last VP
  //  marcel_print_level(&marcel_topo_vp_level[marcel_nbvps()-1], stderr, 1, 1, " ", "#", ":", "");
  marcel_attr_setid(&attr, 1);
  marcel_attr_settopo_level(&attr, &marcel_topo_vp_level[marcel_nbvps()-1]);
  marcel_create(&threads[1], &attr, memory, (any_t) (intptr_t) 1);

  // Start the thread on the first VP
  marcel_attr_setid(&attr, 2);
  marcel_attr_settopo_level(&attr, &marcel_topo_vp_level[0]);
  marcel_create(&threads[2], &attr, memory2, (any_t) (intptr_t) 0);

  // Wait for the threads to complete
  marcel_join(threads[0], NULL);
  marcel_join(threads[1], NULL);
  marcel_join(threads[2], NULL);
}

