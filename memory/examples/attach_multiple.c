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

static void *ptr=NULL;
static mami_manager_t *memory_manager;

any_t attach(any_t arg) {
  marcel_t self;
  int err, node;

  self = marcel_self();
  err = mami_task_attach(memory_manager, ptr, 1000, self, &node);
  MAMI_CHECK_RETURN_VALUE_THREAD(err, "mami_task_attach");

  return ALL_IS_OK;
}

int main(int argc, char * argv[]) {
  int err;
  marcel_t threads[2];
  any_t status;

  marcel_init(&argc,argv);
  mami_init(&memory_manager, argc, argv);

  ptr = mami_malloc(memory_manager, 1000, MAMI_MEMBIND_POLICY_SPECIFIC_NODE, 0);
  MAMI_CHECK_MALLOC(ptr);

  marcel_create(&threads[0], NULL, attach, NULL);
  marcel_create(&threads[1], NULL, attach, NULL);

  marcel_join(threads[0], &status);
  if (status == ALL_IS_NOT_OK) return 1;
  marcel_join(threads[1], &status);
  if (status == ALL_IS_NOT_OK) return 1;

  err = mami_task_unattach_all(memory_manager, threads[0]);
  MAMI_CHECK_RETURN_VALUE(err, "mami_task_unattach_all");
  err = mami_task_unattach_all(memory_manager, threads[1]);
  MAMI_CHECK_RETURN_VALUE(err, "mami_task_unattach_all");

  mami_free(memory_manager, ptr);

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
