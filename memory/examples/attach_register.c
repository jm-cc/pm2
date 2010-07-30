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

#if defined(MM_MAMI_ENABLED)

#include <mm_mami.h>
#include "helper.h"

int main(int argc, char * argv[]) {
  int err, node;
  void *ptr;
  marcel_t self;
  mami_manager_t *memory_manager;

  marcel_init(&argc,argv);
  mami_init(&memory_manager, argc, argv);

  ptr = malloc(1000);
  self = marcel_self();
  err = mami_task_attach(memory_manager, ptr, 1000, self, &node);
  MAMI_CHECK_RETURN_VALUE(err, "mami_task_attach");

  err = mami_task_unattach(memory_manager, ptr, self);
  MAMI_CHECK_RETURN_VALUE(err, "mami_task_unattach");

  err = mami_unregister(memory_manager, ptr);
  MAMI_CHECK_RETURN_VALUE(err, "mami_unregister");

  mami_exit(&memory_manager);

  // Finish marcel
  marcel_end();
  return 0;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
