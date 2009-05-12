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

int marcel_main(int argc, char * argv[]) {
  int err, node;
  void *ptr=NULL;
  marcel_t self;
  mami_manager_t memory_manager;

  marcel_init(&argc,argv);
  mami_init(&memory_manager);

  ptr = mami_malloc(&memory_manager, 10*getpagesize(), MAMI_MEMBIND_POLICY_DEFAULT, 0);
  self = marcel_self();

  err = mami_task_attach(&memory_manager, ptr, (2*getpagesize())-10, self, &node);
  if (err < 0) perror("mami_attach unexpectedly failed");

  err = mami_task_unattach(&memory_manager, ptr, self);
  if (err < 0) perror("mami_unattach unexpectedly failed");

  mami_free(&memory_manager, ptr);
  mami_exit(&memory_manager);
  marcel_printf("Success\n");

  // Finish marcel
  marcel_end();
  return 0;
}

#else
int marcel_main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif