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
  marcel_memory_manager_t memory_manager;
  int ptr[10000], i;
  int *buffer;
  int err, node=-2;
  size_t size;

  marcel_init(&argc,argv);
  marcel_memory_init(&memory_manager);

  err = marcel_memory_membind(&memory_manager, MARCEL_MEMORY_MEMBIND_POLICY_FIRST_TOUCH, 0);
  if (err < 0) perror("marcel_memory_membind");

  //buffer=ptr;
  size=10000*sizeof(int);
  //buffer=memalign(memory_manager.normalpagesize, size);
  buffer = marcel_memory_malloc(&memory_manager, size);

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

  err = marcel_memory_task_unattach(&memory_manager, buffer, marcel_self());
  if (err < 0) perror("marcel_memory_task_unattach");
  marcel_memory_free(&memory_manager, buffer);
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
