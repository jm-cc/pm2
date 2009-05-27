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
  mami_manager_t *memory_manager;
  void *buffer, *buffer2;
  int err;

  // Initialise marcel
  marcel_init(&argc, argv);
  mami_init(&memory_manager);

  buffer = mami_malloc(memory_manager, 100, MAMI_MEMBIND_POLICY_SPECIFIC_NODE, 0);
  err = mami_migrate_pages(memory_manager, buffer, 0);
  if (err < 0) perror("mami_migrate_pages");

  buffer2 = malloc(100);
  err = mami_migrate_pages(memory_manager, buffer2, 0);
  if (err < 0) perror("mami_migrate_pages");

  // Finish marcel
  mami_free(memory_manager, buffer);
  free(buffer2);
  mami_exit(&memory_manager);
  marcel_end();
  return 0;
}

#else
int marcel_main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
  return 0;
}
#endif
