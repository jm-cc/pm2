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

static void do_first_touch(void *buffer, size_t size);
static mami_manager_t *memory_manager;
static any_t first_touch(any_t arg);

int main(int argc, char * argv[]) {
  int err;
  marcel_t thread;
  marcel_attr_t attr;

  marcel_init(&argc,argv);
  marcel_attr_init(&attr);
  mami_init(&memory_manager, argc, argv);
  mami_unset_kernel_migration(memory_manager);

  marcel_attr_settopo_level_on_node(&attr, marcel_nbnodes-1);
  marcel_create(&thread, &attr, first_touch, NULL);
  marcel_join(thread, NULL);

  // Finish marcel
  mami_exit(&memory_manager);
  marcel_end();
  return 0;
}

static any_t first_touch(any_t arg) {
  void *buffer;
  size_t size;
  int err;

  size = 10000*sizeof(int);

  // Case with user-allocated memory
  fprintf(stderr, "... Testing user-allocated memory\n");
  posix_memalign(&buffer, getpagesize(), size);
  do_first_touch(buffer, size);
  err = mami_unregister(memory_manager, buffer);
  if (err < 0) perror("mami_unregister unexpectedly failed");
  free(buffer);

  // Case with mami-allocated memory
  err = mami_membind(memory_manager, MAMI_MEMBIND_POLICY_FIRST_TOUCH, 0);
  if (err < 0) perror("mami_membind unexpectedly failed");
  fprintf(stderr, "... Testing MaMI-allocated memory\n");
  buffer = mami_malloc(memory_manager, size, MAMI_MEMBIND_POLICY_DEFAULT, 0);
  do_first_touch(buffer, size);
  mami_free(memory_manager, buffer);
}

static void do_first_touch(void *buffer, size_t size) {
  int i, err;
  int node;

  err = mami_task_attach(memory_manager, buffer, size, marcel_self(), &node);
  if (err < 0) perror("mami_task_attach unexpectedly failed");

  err = mami_locate(memory_manager, buffer, size, &node);
  if (err < 0) perror("mami_locate unexpectedly failed");
  if (node < 0)
    marcel_fprintf(stderr, "Memory not located on any node\n");
  else
    marcel_fprintf(stderr, "Memory located on node %d\n", node);

  err = mami_migrate_on_next_touch(memory_manager, buffer);
  if (err < 0) perror("mami_migrate_on_next_touch unexpectedly failed");

  memset(buffer, 0, size);

  err = mami_locate(memory_manager, buffer, size, &node);
  if (err < 0) perror("mami_locate unexpectedly failed");
  marcel_fprintf(stderr, "Memory located on node %d\n", node);

  err = mami_check_pages_location(memory_manager, buffer, size, node);
  if (err < 0) perror("mami_check_pages_location unexpectedly failed");

  err = mami_task_unattach(memory_manager, buffer, marcel_self());
  if (err < 0) perror("mami_task_unattach unexpectedly failed");
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
