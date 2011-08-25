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
#include "mami.h"
#include "mami_helper.h"

#include <sys/mman.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "helper.h"

#if defined(MAMI_ENABLED)

int main(int argc, char **argv) {
  int nbpages=4;
  void *buffer;
  void *pageaddrs[nbpages];
  int nodes[nbpages];
  int status[nbpages];
  int i, err;
  unsigned long pagesize;
  unsigned long nodemask;
  int nbnodes;

  tbx_topology_init(argc, argv);
  nbnodes = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_NODE);
  if (nbnodes < 2) {
    fprintf(stderr,"This application needs at least two NUMA nodes.\n");
    return MAMI_TEST_SKIPPED;
  }

  pagesize = getpagesize();
  nodemask = (1<<0);

  buffer = mmap(NULL, nbpages*pagesize, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
  err = _mami_mbind(buffer, nbpages*pagesize, MPOL_BIND, &nodemask, nbnodes+2, MPOL_MF_MOVE);
  if (err < 0) {
    perror("mbind");
    exit(1);
  }
  buffer = memset(buffer, 0, pagesize);

  pageaddrs[0] = 0;
  pageaddrs[1] = buffer;
  pageaddrs[2] = buffer;
  pageaddrs[3] = buffer+pagesize;

  mprintf(stderr, "move pages to node %d\n", 1);
  for(i=0; i<nbpages ; i++) nodes[i] = 1;

  err = _mami_move_pages((const void **)pageaddrs, nbpages, nodes, status, MPOL_MF_MOVE);
  if (err < 0) {
    if (errno == ENOENT) {
      mprintf(stderr, "warning. cannot move pages which have not been allocated\n");
    }
    else {
      perror("move_pages");
      exit(-1);
    }
  }

  for(i=0; i<nbpages; i++) {
    if (status[i] < 0)
      mprintf(stderr, "  page #%d returned the status %d\n", i, status[i]);
    else
      mprintf(stderr, "  page #%d is on node #%d\n", i, status[i]);
  }

  return 0;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
  return MAMI_TEST_SKIPPED;
}
#endif
