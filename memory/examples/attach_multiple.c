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

static void *ptr=NULL;
static mami_manager_t *memory_manager;

any_t attach(any_t arg) {
  marcel_t self;
  int err, node;

  self = marcel_self();
  err = mami_task_attach(memory_manager, ptr, 1000, self, &node);
  if (err < 0) perror("mami_task_attach unexpectedly failed");
}

int main(int argc, char * argv[]) {
  int err;
  marcel_t threads[2];

  marcel_init(&argc,argv);
  mami_init(&memory_manager, argc, argv);

  ptr = mami_malloc(memory_manager, 1000, MAMI_MEMBIND_POLICY_SPECIFIC_NODE, 0);

  marcel_create(&threads[0], NULL, attach, NULL);
  marcel_create(&threads[1], NULL, attach, NULL);

  marcel_join(threads[0], NULL);
  marcel_join(threads[1], NULL);

  err = mami_task_unattach_all(memory_manager, threads[0]);
  if (err < 0) perror("mami_task_unattach_all unexpectedly failed");
  err = mami_task_unattach_all(memory_manager, threads[1]);
  if (err < 0) perror("mami_task_unattach_all unexpectedly failed");

  mami_free(memory_manager, ptr);
  marcel_printf("Success\n");

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
