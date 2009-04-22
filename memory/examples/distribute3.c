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

#include "mami.h"
#include <numa.h>
#include <sys/mman.h>

/* Policies */
enum {
	MPOL_DEFAULT,
	MPOL_PREFERRED,
	MPOL_BIND,
	MPOL_INTERLEAVE,
	MPOL_MAX,	/* always last member of enum */
};
/* Flags for mbind */
#  define MPOL_MF_STRICT	(1<<0)	/* Verify existing pages in the mapping */
#  define MPOL_MF_MOVE		(1<<1)	/* Move pages owned by this process to conform to mapping */

#if defined(MAMI_ENABLED)

int marcel_main(int argc, char * argv[]) {
  int node, *nodes;
  int err, i;
  void *ptr;
  unsigned long nodemask;
  marcel_memory_manager_t memory_manager;

  marcel_init(&argc,argv);
  marcel_memory_init(&memory_manager);

  ptr = mmap(NULL, 50000, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  for(i=0 ; i<memory_manager.nb_nodes ; i++) nodemask += (1<<i);
  err = ma_memory_mbind(ptr, 50000, MPOL_INTERLEAVE, &nodemask, memory_manager.nb_nodes+2, MPOL_MF_MOVE|MPOL_MF_STRICT);
  if (err < 0) perror("ma_memory_mbind unexpectedly failed");
  memset(ptr, 0, 50000);

  marcel_memory_register(&memory_manager, ptr, 50000);

  nodes = malloc(sizeof(int) * marcel_nbnodes);
  for(i=0 ; i<marcel_nbnodes ; i++) nodes[i] = i;
  err = marcel_memory_distribute(&memory_manager, ptr, nodes, marcel_nbnodes);
  if (err < 0) perror("marcel_memory_distribute unexpectedly failed");
  else {
    marcel_memory_gather(&memory_manager, ptr, 1);
    marcel_memory_locate(&memory_manager, ptr, 50000, &node);
    if (node == 1)
      marcel_fprintf(stderr, "Node is %d as expected\n", node);
    else
      marcel_fprintf(stderr, "Node is NOT 1 as expected but %d\n", node);
    marcel_memory_check_pages_location(&memory_manager, ptr, 50000, 1);
  }

  free(nodes);
  marcel_memory_unregister(&memory_manager, ptr);
  munmap(ptr, 50000);
  marcel_memory_exit(&memory_manager);

  // Finish marcel
  marcel_end();
  return 0;
}

#else
int marcel_main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
