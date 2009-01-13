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
  int n, node;
  void *ptr;
  marcel_memory_manager_t memory_manager;

  marcel_init(&argc,argv);
  marcel_memory_init(&memory_manager);

  for(n=0 ; n<memory_manager.nb_nodes ; n++) {
    ptr = marcel_memory_malloc(&memory_manager, 4*memory_manager.normalpagesize, MARCEL_MEMORY_MEMBIND_POLICY_SPECIFIC_NODE, n);
    marcel_memory_locate(&memory_manager, ptr, 1, &node);
    if (node != n) marcel_printf("Wrong location: %d, asked for %d\n", node, n);
    marcel_memory_check_pages_location(&memory_manager, ptr, 1, n);
    marcel_memory_free(&memory_manager, ptr);
  }

  // Finish marcel
  marcel_memory_exit(&memory_manager);
  marcel_end();
  return 0;
}

#else
int marcel_main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MAMI to be enabled\n");
}
#endif
