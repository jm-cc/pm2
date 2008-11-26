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
  int err;
  void *ptr=NULL;
  void *ptr2;
  marcel_t self;
  marcel_memory_manager_t memory_manager;

  marcel_init(&argc,argv);
  marcel_memory_init(&memory_manager);
  self = marcel_self();

  err = marcel_memory_task_attach(&memory_manager, ptr, 100, self);
  if (err < 0) perror("marcel_memory_task_attach successfully fails");

  ptr = marcel_memory_allocate_on_node(&memory_manager, 1000, 0);
  ptr2 = marcel_memory_allocate_on_node(&memory_manager, 1000, 0);

  err = marcel_memory_task_attach(&memory_manager, ptr, 1000, self);
  if (err < 0) perror("marcel_memory_task_attach should not fail");
  err = marcel_memory_task_unattach(&memory_manager, ptr, self);
  if (err < 0) perror("marcel_memory_task_unattach should not fail");

  err = marcel_memory_task_attach(&memory_manager, ptr, 1000, self);
  if (err < 0) perror("marcel_memory_task_attach should not fail");
  err = marcel_memory_task_attach(&memory_manager, ptr2, 1000, self);
  if (err < 0) perror("marcel_memory_task_attach should not fail");
  err = marcel_memory_task_unattach_all(&memory_manager, self);
  if (err < 0) perror("marcel_memory_task_unattach_all should not fail");

  marcel_memory_free(&memory_manager, ptr);
  marcel_memory_free(&memory_manager, ptr2);
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
