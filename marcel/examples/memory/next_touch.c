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

int *b;

any_t writer(any_t arg) {
  b = marcel_memory_malloc(&memory_manager, 3*memory_manager.pagesize);
  return 0;
}

any_t reader(any_t arg) {
  int node;

  marcel_memory_locate(&memory_manager, b, &node);
  marcel_printf("Address is located on node %d\n", node);

  marcel_memory_migrate_on_next_touch(&memory_manager, b);

  marcel_memory_locate(&memory_manager, b, &node);
  marcel_printf("Address is located on node %d\n", node);

  b[1] = 42;

  marcel_memory_locate(&memory_manager, b, &node);
  marcel_printf("Address is located on node %d\n", node);

  marcel_memory_free(&memory_manager, b);
  return 0;
}

int marcel_main(int argc, char * argv[]) {
  marcel_t threads[2];
  marcel_attr_t attr;

  marcel_init(&argc,argv);
  marcel_memory_init(&memory_manager, 1000);
  marcel_attr_init(&attr);

  // Start the thread on the numa node #0
  marcel_attr_settopo_level(&attr, &marcel_topo_node_level[0]);
  marcel_create(&threads[0], &attr, writer, NULL);
  marcel_join(threads[0], NULL);

  // Start the thread on the numa node #1
  marcel_attr_settopo_level(&attr, &marcel_topo_node_level[1]);
  marcel_create(&threads[1], &attr, reader, NULL);
  marcel_join(threads[1], NULL);

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
