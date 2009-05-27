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

int marcel_main(int argc, char * argv[]) {
  mami_manager_t *memory_manager;
  int *ptr, *ptr2;
  size_t size;
  int err;

  marcel_init(&argc,argv);
  mami_init(&memory_manager);

  err = mami_membind(memory_manager, MAMI_MEMBIND_POLICY_FIRST_TOUCH, 0);
  if (err < 0) perror("mami_membind unexpectedly failed");

  size = memory_manager->initially_preallocated_pages * memory_manager->normalpagesize;
  ptr = mami_malloc(memory_manager, size/2, MAMI_MEMBIND_POLICY_DEFAULT, 0);
  ptr2 = mami_malloc(memory_manager, size, MAMI_MEMBIND_POLICY_DEFAULT, 0);

  mami_free(memory_manager, ptr);
  mami_free(memory_manager, ptr2);
  marcel_printf("Success\n");

  // Finish marcel
  mami_exit(&memory_manager);
  marcel_end();
  return 0;
}

#else
int marcel_main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
