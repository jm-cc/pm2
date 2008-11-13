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

int marcel_main(int argc, char * argv[]) {
  size_t bigsize;
  int *ptr, *ptr2;
  marcel_memory_manager_t memory_manager;

  marcel_init(&argc,argv);
  marcel_memory_init(&memory_manager);

  marcel_memory_membind(&memory_manager, MARCEL_MEMORY_MEMBIND_POLICY_HUGE_PAGES, 0);
  if (!marcel_topo_node_level) {
    marcel_printf("NUMA node topology not accessible.\n");
  }
  else {
    bigsize =  marcel_topo_node_level[0].huge_page_free * memory_manager.hugepagesize;

    ptr = marcel_memory_malloc(&memory_manager, bigsize/2);
    ptr2 = marcel_memory_malloc(&memory_manager, bigsize/2);
    if (ptr && ptr2) {
      marcel_printf("Success\n");
      marcel_memory_free(&memory_manager, ptr);
      marcel_memory_free(&memory_manager, ptr2);
      ptr2 = marcel_memory_malloc(&memory_manager, bigsize);
      if (!ptr2) perror("marcel_memory_malloc");
      marcel_memory_free(&memory_manager, ptr2);
    }
    else {
      printf("Failure. Could not allocated with huge pages.\n");
    }
  }

  marcel_memory_exit(&memory_manager);

  // Finish marcel
  marcel_end();
  return 0;
}

#else
int marcel_main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MAMI to be enabled\n");
}
#endif
