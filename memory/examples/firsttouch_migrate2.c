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
#include <malloc.h>
#include "mm_mami.h"

#if defined(MM_MAMI_ENABLED)

/*
 * Test when giving memory areas to MaMI with the first or the last
 * page not being fully used. MaMI considers the whole page but should
 * not mprotect it when calling mami_migrate_on_next_touch
 */

static void first_touch(mami_manager_t *memory_manager, void *buffer, size_t size, int touchme, void *ptr);

int main(int argc, char * argv[]) {
  mami_manager_t *memory_manager;
  void *buffer1, *buffer2, *buffer3, *buffer4;
  size_t size;
  int err;

  common_init(&argc, argv, NULL);
  mami_init(&memory_manager, argc, argv);
  mami_unset_kernel_migration(memory_manager);
  size = 10*getpagesize();

  buffer1=memalign(getpagesize(), size);
  buffer2=memalign(getpagesize(), size);
  buffer3=memalign(getpagesize(), size);
  buffer4=memalign(getpagesize(), size);

  first_touch(memory_manager, buffer1, size-100, 0, buffer1+(size-100));
  first_touch(memory_manager, buffer2, size-100, 1, buffer2+(size-100));
  first_touch(memory_manager, buffer3+100, size-100, 0, buffer3);
  first_touch(memory_manager, buffer4+100, size-100, 1, buffer4);

  free(buffer1);
  free(buffer2);
  free(buffer3);
  free(buffer4);

  mami_exit(&memory_manager);
  common_exit(NULL);
  return 0;
}

static void first_touch(mami_manager_t *memory_manager, void *buffer, size_t size, int touchme, void *ptr) {
  int i, err;
  int node;

  err = mami_register(memory_manager, buffer, size);
  if (err < 0) perror("mami_register unexpectedly failed");

  err = mami_migrate_on_next_touch(memory_manager, buffer);
  if (err < 0) perror("mami_migrate_on_next_touch unexpectedly failed");

  memset(ptr, 0, 10);
  if (touchme) memset(buffer, 0, size);

  err = mami_locate(memory_manager, buffer, size, &node);
  if (err < 0) perror("mami_locate unexpectedly failed");
  if (node >= 0) fprintf(stderr, "Memory located on node %d\n", node);
  else fprintf(stderr, "Memory not located on a specific node\n");

  err = mami_unregister(memory_manager, buffer);
  if (err < 0) perror("mami_unregister unexpectedly failed");
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif