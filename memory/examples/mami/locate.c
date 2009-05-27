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
  int node, err;
  void *ptr;
  mami_manager_t *memory_manager;

  marcel_init(&argc,argv);
  mami_init(&memory_manager);

  ptr = mami_malloc(memory_manager, 1000, MAMI_MEMBIND_POLICY_SPECIFIC_NODE, 0);

  err = mami_locate(memory_manager, ptr, 1000, &node);
  marcel_printf("The memory is located on node #%d\n", node);
  if (err < 0) marcel_printf("mami_locate: error %d\n", err);

  err = mami_locate(memory_manager, NULL, 0, &node);
  marcel_printf("The memory is located on node #%d\n", node);
  if (err < 0) marcel_printf("mami_locate: error %d\n", err);

  mami_free(memory_manager, ptr);

  ptr = malloc(1000);
  err = mami_locate(memory_manager, ptr, 1000, &node);
  marcel_printf("The memory is located on node #%d\n", node);
  if (err < 0) marcel_printf("mami_locate: error %d\n", err);

  free(ptr);
  mami_exit(&memory_manager);

  // Finish marcel
  marcel_end();
  return 0;
}

#else
int marcel_main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
