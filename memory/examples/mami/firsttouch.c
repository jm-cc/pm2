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

#include <stdio.h>
#include "mm_mami.h"

#if defined(MM_MAMI_ENABLED)

mami_manager_t *memory_manager;

any_t firsttouch(any_t arg) {
  int *ptr;
  int err, node;

  ptr = mami_malloc(memory_manager, 100, MAMI_MEMBIND_POLICY_DEFAULT, 0);
  mami_free(memory_manager, ptr);

  ptr = mami_malloc(memory_manager, 100, MAMI_MEMBIND_POLICY_DEFAULT, 0);
  err = mami_locate(memory_manager, ptr, 100, &node);
  if (err < 0) perror("mami_membind unexpectedly failed");
  marcel_printf("Before first touch, memory located on node %d\n", node);

  ptr[0] = 42;

  err = mami_task_attach(memory_manager, ptr, 100, marcel_self(), &node);
  if (err < 0) perror("mami_task_attach unexpectedly failed");

  err = mami_locate(memory_manager, ptr, 100, &node);
  if (err < 0) perror("mami_locate unexpectedly failed");
  if (node == marcel_nbnodes-1)
    marcel_printf("After first touch, memory located on last node\n");
  else
    marcel_printf("After first touch, memory NOT located on last node but on node %d\n", node);

  err = mami_task_unattach(memory_manager, ptr, marcel_self());
  if (err < 0) perror("mami_task_unattach unexpectedly failed");
  mami_free(memory_manager, ptr);
}

int main(int argc, char * argv[]) {
  int err;
  marcel_t thread;
  marcel_attr_t attr;

  marcel_init(&argc,argv);
  marcel_attr_init(&attr);
  mami_init(&memory_manager);

  err = mami_membind(memory_manager, MAMI_MEMBIND_POLICY_FIRST_TOUCH, 0);
  if (err < 0) perror("mami_membind unexpectedly failed");

  marcel_attr_settopo_level(&attr, &marcel_topo_node_level[marcel_nbnodes-1]);
  marcel_create(&thread, &attr, firsttouch, NULL);
  marcel_join(thread, NULL);

  // Finish marcel
  mami_exit(&memory_manager);
  marcel_end();
  return 0;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
