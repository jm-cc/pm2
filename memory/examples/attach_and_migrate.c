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
#include <mm_mami_private.h>
#include "helper.h"

int main(int argc, char * argv[]) {
  int err, node;
  void *ptr=NULL;
  void *ptr2;
  marcel_t self;
  mami_manager_t *memory_manager;

  marcel_init(&argc,argv);
  mami_init(&memory_manager, argc, argv);
  self = marcel_self();

  if (memory_manager->nb_nodes < 2) {
    printf("This application needs at least two NUMA nodes.\n");
  }
  else {
    ptr = mami_malloc(memory_manager, 1000, MAMI_MEMBIND_POLICY_SPECIFIC_NODE, 1);
    MAMI_CHECK_MALLOC(ptr);
    ptr2 = mami_malloc(memory_manager, 1000, MAMI_MEMBIND_POLICY_SPECIFIC_NODE, 0);
    MAMI_CHECK_MALLOC(ptr2);

    err = mami_task_attach(memory_manager, ptr, 1000, self, &node);
    MAMI_CHECK_RETURN_VALUE(err, "mami_task_attach");
    err = mami_task_attach(memory_manager, ptr2, 1000, self, &node);
    MAMI_CHECK_RETURN_VALUE(err, "mami_task_attach");

    err = mami_task_migrate_all(memory_manager, self, 1);
    MAMI_CHECK_RETURN_VALUE(err, "mami_task_migrate_all");

    err = mami_check_pages_location(memory_manager, ptr, 1000, 1);
    MAMI_CHECK_RETURN_VALUE(err, "mami_check_pages_location");
    err = mami_check_pages_location(memory_manager, ptr2, 1000, 1);
    MAMI_CHECK_RETURN_VALUE(err, "mami_check_pages_location");

    err = mami_task_unattach_all(memory_manager, self);
    MAMI_CHECK_RETURN_VALUE(err, "mami_task_unattach_all");

    mami_free(memory_manager, ptr);
    mami_free(memory_manager, ptr2);
  }

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