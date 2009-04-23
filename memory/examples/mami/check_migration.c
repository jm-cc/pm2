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

#include "mm_mami.h"

#include <sys/mman.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <numa.h>
#include <numaif.h>

#if defined(MM_MAMI_ENABLED)

int marcel_main(int argc, char **argv) {
  int nbpages=4;
  void *buffer;
  void *pageaddrs[nbpages];
  int nodes[nbpages];
  int status[nbpages];
  int i, err;
  unsigned long pagesize;
  unsigned long maxnode;
  unsigned long nodemask;

  maxnode = numa_max_node();
  if (!maxnode) {
    marcel_printf("Need more than one NUMA node. Abort\n");
    exit(1);
  }

  pagesize = getpagesize();
  nodemask = (1<<0);

  buffer = mmap(NULL, nbpages*pagesize, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
  err = mbind(buffer, nbpages*pagesize, MPOL_BIND, &nodemask, maxnode+2, MPOL_MF_MOVE);
  if (err < 0) {
    perror("mbind");
    exit(1);
  }
  buffer = memset(buffer, 0, pagesize);

  pageaddrs[0] = 0;
  pageaddrs[1] = buffer;
  pageaddrs[2] = buffer;
  pageaddrs[3] = buffer+pagesize;

  printf("move pages to node %d\n", 1);
  for(i=0; i<nbpages ; i++) nodes[i] = 1;

  err = _mami_move_pages(pageaddrs, nbpages, nodes, status, MPOL_MF_MOVE);
  if (err < 0) {
    if (errno == ENOENT) {
      printf("warning. cannot move pages which have not been allocated\n");
    }
    else {
      perror("move_pages");
      exit(-1);
    }
  }

  for(i=0; i<nbpages; i++) {
    if (status[i] < 0)
      printf("  page #%d returned the status %d\n", i, status[i]);
    else
      printf("  page #%d is on node #%d\n", i, status[i]);
  }

  return 0;
}

#else
int marcel_main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
