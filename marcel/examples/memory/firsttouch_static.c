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

static void first_touch(int *buffer, size_t size, int elems);
static marcel_memory_manager_t memory_manager;

int marcel_main(int argc, char * argv[]) {
  //  int ptr2[100]; // That variable is only used as a buffer to avoid a segfault when mprotecting ptr as MaMI aligns on page boundaries
  int ptr[10000];
  int *buffer;
  int err;

  marcel_init(&argc,argv);
  marcel_memory_init(&memory_manager);

  // Case with user-allocated memory
  buffer=memalign(memory_manager.normalpagesize, 10000*sizeof(int));
  first_touch(buffer, 10000*sizeof(int), 10000);
  err = marcel_memory_unregister(&memory_manager, buffer);
  if (err < 0) perror("marcel_memory_unregister");
  free(buffer);

  err = marcel_memory_membind(&memory_manager, MARCEL_MEMORY_MEMBIND_POLICY_FIRST_TOUCH, 0);
  if (err < 0) perror("marcel_memory_membind");

  // Case with mami-allocated memory
  buffer = marcel_memory_malloc(&memory_manager, 10000*sizeof(int));
  first_touch(buffer, 10000*sizeof(int), 10000);
  marcel_memory_free(&memory_manager, buffer);

  // Case with static memory
//  buffer=ptr;
//  first_touch(buffer, 10000*sizeof(int), 10000);
//  err = marcel_memory_unregister(&memory_manager, buffer);
//  if (err < 0) perror("marcel_memory_unregister");

  // Finish marcel
  marcel_memory_exit(&memory_manager);
  marcel_end();
  return 0;
}

static void first_touch(int *buffer, size_t size, int elems) {
  int i, err;
  int node=-2;

  err = marcel_memory_task_attach(&memory_manager, buffer, size, marcel_self());
  if (err < 0) perror("marcel_memory_task_attach");

  err = marcel_memory_locate(&memory_manager, buffer, size, &node);
  if (err < 0) perror("marcel_memory_locate");
  marcel_printf("Memory located on node %d\n", node);

  err = marcel_memory_migrate_on_next_touch(&memory_manager, buffer);
  if (err < 0) perror("marcel_memory_migrate_on_next_touch");

  for(i=0 ; i<10000 ; i++) buffer[i] = 12;

  err = marcel_memory_locate(&memory_manager, buffer, size, &node);
  if (err < 0) perror("marcel_memory_locate");
  marcel_printf("Memory located on node %d\n", node);

  err = marcel_memory_check_pages_location(&memory_manager, buffer, size, node);
  if (err < 0) perror("marcel_memory_check_pages_location");

  err = marcel_memory_task_unattach(&memory_manager, buffer, marcel_self());
  if (err < 0) perror("marcel_memory_task_unattach");
}

#else
int marcel_main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MAMI to be enabled\n");
}
#endif
