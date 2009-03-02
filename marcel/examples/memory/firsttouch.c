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

any_t firsttouch(any_t arg) {
  int *ptr;
  int err, node;

  ptr = marcel_memory_malloc(&memory_manager, 100, MARCEL_MEMORY_MEMBIND_POLICY_DEFAULT, 0);
  marcel_memory_free(&memory_manager, ptr);

  ptr = marcel_memory_malloc(&memory_manager, 100, MARCEL_MEMORY_MEMBIND_POLICY_DEFAULT, 0);
  err = marcel_memory_locate(&memory_manager, ptr, 100, &node);
  if (err < 0) perror("marcel_memory_membind unexpectedly failed");
  marcel_printf("Before first touch, memory located on node %d\n", node);

  ptr[0] = 42;

  err = marcel_memory_task_attach(&memory_manager, ptr, 100, marcel_self(), &node);
  if (err < 0) perror("marcel_memory_task_attach unexpectedly failed");

  err = marcel_memory_locate(&memory_manager, ptr, 100, &node);
  if (err < 0) perror("marcel_memory_locate unexpectedly failed");
  if (node == marcel_nbnodes-1)
    marcel_printf("After first touch, memory located on last node\n");
  else
    marcel_printf("After first touch, memory NOT located on last node but on node %d\n", node);

  err = marcel_memory_task_unattach(&memory_manager, ptr, marcel_self());
  if (err < 0) perror("marcel_memory_task_unattach unexpectedly failed");
  marcel_memory_free(&memory_manager, ptr);
}

int marcel_main(int argc, char * argv[]) {
  int err;
  marcel_t thread;
  marcel_attr_t attr;

  marcel_init(&argc,argv);
  marcel_attr_init(&attr);
  marcel_memory_init(&memory_manager);

  err = marcel_memory_membind(&memory_manager, MARCEL_MEMORY_MEMBIND_POLICY_FIRST_TOUCH, 0);
  if (err < 0) perror("marcel_memory_membind unexpectedly failed");

  marcel_attr_settopo_level(&attr, &marcel_topo_node_level[marcel_nbnodes-1]);
  marcel_create(&thread, &attr, firsttouch, NULL);
  marcel_join(thread, NULL);

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
