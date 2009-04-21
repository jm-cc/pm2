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
#include <malloc.h>

#if defined(MARCEL_MAMI_ENABLED)

/*
 * Test when giving memory areas to MaMI with the first or the last
 * page not being fully used. MaMI considers the whole page but should
 * not mprotect it when calling mami_migrate_on_next_touch
 */

static void first_touch(marcel_memory_manager_t *memory_manager, void *buffer, size_t size, int touchme, void *ptr);

int marcel_main(int argc, char * argv[]) {
  marcel_memory_manager_t memory_manager;
  void *buffer1, *buffer2, *buffer3, *buffer4;
  size_t size;
  int err;

  marcel_init(&argc,argv);
  marcel_memory_init(&memory_manager);
  memory_manager.kernel_nexttouch_migration = 0;
  size = 10*memory_manager.normalpagesize;

  buffer1=memalign(memory_manager.normalpagesize, size);
  buffer2=memalign(memory_manager.normalpagesize, size);
  buffer3=memalign(memory_manager.normalpagesize, size);
  buffer4=memalign(memory_manager.normalpagesize, size);

  first_touch(&memory_manager, buffer1, size-100, 0, buffer1+(size-100));
  first_touch(&memory_manager, buffer2, size-100, 1, buffer2+(size-100));
  first_touch(&memory_manager, buffer3+100, size-100, 0, buffer3);
  first_touch(&memory_manager, buffer4+100, size-100, 1, buffer4);

  free(buffer1);
  free(buffer2);
  free(buffer3);
  free(buffer4);

  // Finish marcel
  marcel_memory_exit(&memory_manager);
  marcel_end();
  return 0;
}

static void first_touch(marcel_memory_manager_t *memory_manager, void *buffer, size_t size, int touchme, void *ptr) {
  int i, err;
  int node;

  err = marcel_memory_register(memory_manager, buffer, size);
  if (err < 0) perror("marcel_memory_register unexpectedly failed");

  err = marcel_memory_migrate_on_next_touch(memory_manager, buffer);
  if (err < 0) perror("marcel_memory_migrate_on_next_touch unexpectedly failed");

  memset(ptr, 0, 10);
  if (touchme) memset(buffer, 0, size);

  err = marcel_memory_locate(memory_manager, buffer, size, &node);
  if (err < 0) perror("marcel_memory_locate unexpectedly failed");
  if (node >= 0) marcel_fprintf(stderr, "Memory located on node %d\n", node);
  else marcel_fprintf(stderr, "Memory not located on a specific node\n");

  err = marcel_memory_unregister(memory_manager, buffer);
  if (err < 0) perror("marcel_memory_unregister unexpectedly failed");
}

#else
int marcel_main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
