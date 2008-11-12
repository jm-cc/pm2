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

marcel_memory_manager_t memory_manager;

int marcel_main(int argc, char * argv[]) {
  int *ptr, *ptr2;
  size_t bigsize;

  marcel_init(&argc,argv);
  marcel_memory_init(&memory_manager);

  if (!(marcel_memory_huge_pages_available(&memory_manager))) {
    marcel_printf("Huge pages are not available on the system.\n");
  }
  else {
    marcel_memory_membind(&memory_manager, MARCEL_MEMORY_MEMBIND_POLICY_HUGE_PAGES, 0);

    if (!marcel_topo_node_level) {
      marcel_printf("NUMA node topology not accessible.\n");
    }
    else {
      bigsize =  marcel_topo_node_level[0].huge_page_free * memory_manager.hugepagesize;
      ptr = marcel_memory_malloc(&memory_manager, bigsize+1);
      perror("marcel_memory_malloc");

      ptr = marcel_memory_malloc(&memory_manager, bigsize/2);
      if (!ptr) {
	perror("marcel_memory_malloc");
      }
      else {
	ptr[10] = 10;
	marcel_printf("ptr[10]=%d\n", ptr[10]);

	ptr2 =  marcel_memory_malloc(&memory_manager, bigsize/2);
	if (!ptr2) {
	  perror("marcel_memory_malloc");
	}
	else {
	  ptr2[20] = 20;
	  marcel_printf("ptr2[20]=%d\n", ptr2[20]);
	  marcel_memory_free(&memory_manager, ptr2);
	}
	marcel_memory_free(&memory_manager, ptr);
      }
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