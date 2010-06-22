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
#include "mm_mami_private.h"

#if defined(MM_MAMI_ENABLED)

int main(int argc, char * argv[]) {
  mami_manager_t *memory_manager;
  int ptr[getpagesize()*10/sizeof(int)];
  int i, err, node;
  size_t size;

  marcel_init(&argc,argv);
  mami_init(&memory_manager, argc, argv);

  size=getpagesize()*10;
  err = mami_task_attach(memory_manager, ptr, size, marcel_self(), &node);
  if (err < 0) perror("mami_task_attach unexpectedly failed");

  err = mami_locate(memory_manager, ptr, size, &node);
  if (err < 0) perror("mami_locate unexpectedly failed");

  if (node != MAMI_MULTIPLE_LOCATION_NODE) {
    err = mami_check_pages_location(memory_manager, ptr, size, node);
    if (err < 0) perror("mami_check_pages_location unexpectedly failed");
  }

  mami_unset_kernel_migration(memory_manager);
  err = mami_migrate_on_next_touch(memory_manager, ptr);
  if (err < 0) perror("mami_migrate_on_next_touch unexpectedly failed");

  for(i=0 ; i<10000 ; i++) ptr[i] = 12;

  err = mami_locate(memory_manager, ptr, size, &node);
  if (err < 0) perror("mami_locate unexpectedly failed");
  marcel_fprintf(stderr, "Memory located on node %d\n", node);

  err = mami_check_pages_location(memory_manager, ptr, size, node);
  if (err < 0) {
    perror("mami_check_pages_location unexpectedly failed");
    err = mami_update_pages_location(memory_manager, ptr, size);
    if (err < 0) perror("mami_update_pages_location unexpectedly failed");
    err = mami_locate(memory_manager, ptr, size, &node);
    if (err < 0) perror("mami_locate unexpectedly failed");
    marcel_fprintf(stderr, "Memory located on node %d\n", node);
  }

  err = mami_task_unattach(memory_manager, ptr, marcel_self());
  if (err < 0) perror("mami_task_unattach unexpectedly failed");

  err = mami_unregister(memory_manager, ptr);
  if (err < 0) perror("mami_unregister unexpectedly failed");

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
