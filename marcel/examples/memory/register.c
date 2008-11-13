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
  void *ptr;
  marcel_t self;
  marcel_memory_manager_t memory_manager;

  marcel_init(&argc,argv);
  marcel_memory_init(&memory_manager);

  ptr = malloc(1000);
  self = marcel_self();
  err = marcel_memory_task_attach(&memory_manager, ptr, 1000, &self);
  if (err < 0) {
    perror("marcel_memory_task_attach");
  }
  err = marcel_memory_task_unattach(&memory_manager, ptr, &self);
  if (err < 0) {
    perror("marcel_memory_task_unattach");
  }

  err = marcel_memory_unregister(&memory_manager, ptr);
  if (err < 0) {
    perror("marcel_memory_unregister");
  }
  free(ptr);

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
