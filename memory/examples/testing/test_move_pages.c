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
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <numaif.h>

#define PAGES 4

void my_print_pagenodes(void **pageaddrs) {
  int status[PAGES];
  int i;
  int err;

  err = move_pages(0, PAGES, pageaddrs, NULL, status, 0);
  if (err < 0) {
    perror("move_pages");
    exit(-1);
  }

  for(i=0; i<PAGES; i++) {
    if (status[i] == -ENOENT)
      printf("  page #%d is not allocated\n", i);
    else
      printf("  page #%d is on node #%d\n", i, status[i]);
  }
}

void move_pagenodes(void **pageaddrs, const int *nodes) {
  int status[PAGES];
  int i;
  int err;

  err = move_pages(0, PAGES, pageaddrs, nodes, status, MPOL_MF_MOVE);
  if (err < 0) {
    if (errno == ENOENT) {
      printf("warning. cannot move pages which have not been allocated\n");
    }
    else {
      perror("move_pages");
      exit(-1);
    }
  }

  for(i=0; i<PAGES; i++) {
    if (status[i] == -ENOENT)
      printf("  page #%d is not allocated\n", i);
    else
      printf("  page #%d is on node #%d\n", i, status[i]);
  }
}

void test_pagenodes(unsigned long *buffer, unsigned long pagesize, unsigned long value) {
  int i, valid=1;
  for(i=0 ; i<PAGES*pagesize ; i++) {
    if (buffer[i] != value) {
      printf("buffer[%d] incorrect = %u\n", i, buffer[i]);
      valid=0;
    }
  }
  if (valid) {
    printf("buffer correct\n");
  }
}

int main(int argc, char * argv[])
{
  unsigned long *buffer;
  void **pageaddrs;
  int i, status[PAGES];
  unsigned long pagesize;
  unsigned long maxnode;
  int nodes[PAGES];

  marcel_init(&argc,argv);
#ifdef PROFILE
  profile_activate(FUT_ENABLE, MARCEL_PROF_MASK, 0);
#endif

  maxnode = numa_max_node();
  printf("numa_max_node %d\n", maxnode);

  if (!maxnode) {
    printf("Need more than one NUMA node. Abort\n");
    exit(1);
  }

  pagesize = getpagesize();
  buffer = malloc(PAGES * pagesize * sizeof(unsigned long));
  pageaddrs = malloc(PAGES * sizeof(void *));
  for(i=0; i<PAGES; i++)
    pageaddrs[i] = buffer + i*pagesize;

  printf("before touching the pages\n");
  my_print_pagenodes(pageaddrs);

  printf("move pages to node %d\n", maxnode);
  for(i=0; i<PAGES ; i++)
    nodes[i] = maxnode;
  move_pagenodes(pageaddrs, nodes);

  printf("filling in the buffer\n");
  for(i=0 ; i<PAGES*pagesize ; i++) {
    buffer[i] = (unsigned long)42;
  }
  test_pagenodes(buffer, pagesize, 42);

  my_print_pagenodes(pageaddrs);

  printf("move pages to node %d\n", 1);
  for(i=0; i<PAGES ; i++)
    nodes[i] = 1;
  move_pagenodes(pageaddrs, nodes);
  test_pagenodes(buffer, pagesize, 42);

#ifdef PROFILE
  profile_stop();
#endif

  return 0;
}
