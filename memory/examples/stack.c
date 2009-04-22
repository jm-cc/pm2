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

#include "mami.h"

#if defined(MAMI_ENABLED)

int marcel_main(int argc, char * argv[]) {
  marcel_memory_manager_t memory_manager;
  int ptr[10000];
  int i, err, node;
  size_t size;

  marcel_init(&argc,argv);
  marcel_memory_init(&memory_manager);
  marcel_memory_unset_alignment(&memory_manager);

  size=10000*sizeof(int);
  err = marcel_memory_task_attach(&memory_manager, ptr, size, marcel_self(), &node);
  if (err < 0) perror("marcel_memory_task_attach unexpectedly failed");

  err = marcel_memory_locate(&memory_manager, ptr, size, &node);
  if (err < 0) perror("marcel_memory_locate unexpectedly failed");
  marcel_fprintf(stderr, "Memory located on node %d\n", node);

  err = marcel_memory_check_pages_location(&memory_manager, ptr, size, node);
  if (err < 0) perror("marcel_memory_check_pages_location unexpectedly failed");

  memory_manager.kernel_nexttouch_migration = 0;
  err = marcel_memory_migrate_on_next_touch(&memory_manager, ptr);
  if (err < 0) perror("marcel_memory_migrate_on_next_touch unexpectedly failed");

  for(i=0 ; i<10000 ; i++) ptr[i] = 12;

  err = marcel_memory_locate(&memory_manager, ptr, size, &node);
  if (err < 0) perror("marcel_memory_locate unexpectedly failed");
  marcel_fprintf(stderr, "Memory located on node %d\n", node);

  err = marcel_memory_check_pages_location(&memory_manager, ptr, size, node);
  if (err < 0) {
    perror("marcel_memory_check_pages_location unexpectedly failed");
    err = marcel_memory_update_pages_location(&memory_manager, ptr, size);
    if (err < 0) perror("marcel_memory_update_pages_location unexpectedly failed");
    err = marcel_memory_locate(&memory_manager, ptr, size, &node);
    if (err < 0) perror("marcel_memory_locate unexpectedly failed");
    marcel_fprintf(stderr, "Memory located on node %d\n", node);
  }

  err = marcel_memory_task_unattach(&memory_manager, ptr, marcel_self());
  if (err < 0) perror("marcel_memory_task_unattach unexpectedly failed");

  err = marcel_memory_unregister(&memory_manager, ptr);
  if (err < 0) perror("marcel_memory_unregister unexpectedly failed");

  // Finish marcel
  marcel_memory_exit(&memory_manager);
  marcel_end();
  return 0;
}

#else
int marcel_main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
