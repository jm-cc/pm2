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
  void *ptr1, *ptr2, *ptr3, *ptr4, *ptr5;
  size_t size;
  marcel_memory_manager_t memory_manager;

  marcel_init(&argc,argv);
  marcel_memory_init(&memory_manager);

  size = memory_manager.initially_preallocated_pages * memory_manager.normalpagesize;
  ptr1 = marcel_memory_malloc(&memory_manager, size+1, MARCEL_MEMORY_MEMBIND_POLICY_DEFAULT, 0);
  ptr2 = marcel_memory_malloc(&memory_manager, size/2, MARCEL_MEMORY_MEMBIND_POLICY_DEFAULT, 0);
  ptr3 = marcel_memory_malloc(&memory_manager, size/2, MARCEL_MEMORY_MEMBIND_POLICY_DEFAULT, 0);
  ptr4 = marcel_memory_malloc(&memory_manager, 4, MARCEL_MEMORY_MEMBIND_POLICY_DEFAULT, 0);
  ptr5 = marcel_memory_malloc(&memory_manager, size, MARCEL_MEMORY_MEMBIND_POLICY_DEFAULT, 0);

  marcel_memory_free(&memory_manager, ptr1);
  marcel_memory_free(&memory_manager, ptr2);
  marcel_memory_free(&memory_manager, ptr3);
  marcel_memory_free(&memory_manager, ptr4);
  marcel_memory_free(&memory_manager, ptr5);

  marcel_memory_exit(&memory_manager);

  // Finish marcel
  marcel_printf("Success\n");
  marcel_end();
  return 0;
}

#else
int marcel_main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
