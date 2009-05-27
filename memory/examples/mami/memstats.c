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

#define MARCEL_INTERNAL_INCLUDE
#include <stdio.h>
#include "mm_mami.h"

#if defined(MM_MAMI_ENABLED)

int marcel_main(int argc, char * argv[]) {
  mami_manager_t *memory_manager;
  void *ptr;
  unsigned long memtotal1, memfree1;
  unsigned long memtotal2, memfree2;
  unsigned long memtotal3, memfree3;

  marcel_init(&argc,argv);
  mami_init(&memory_manager);

  mami_stats(memory_manager, 0, MAMI_STAT_MEMORY_TOTAL, &memtotal1);
  mami_stats(memory_manager, 0, MAMI_STAT_MEMORY_FREE, &memfree1);

  ptr = mami_malloc(memory_manager, 100, MAMI_MEMBIND_POLICY_SPECIFIC_NODE, 0);

  mami_stats(memory_manager, 0, MAMI_STAT_MEMORY_TOTAL, &memtotal2);
  mami_stats(memory_manager, 0, MAMI_STAT_MEMORY_FREE, &memfree2);

  mami_free(memory_manager, ptr);

  mami_stats(memory_manager, 0, MAMI_STAT_MEMORY_TOTAL, &memtotal3);
  mami_stats(memory_manager, 0, MAMI_STAT_MEMORY_FREE, &memfree3);

  if (memfree1 == memfree3 && (memfree1-memfree2) == (getpagesize()/1024)) {
    marcel_printf("Success\n");
  }
  else {
    marcel_printf("Error\n");
    marcel_printf("Memtotal: %ld -- Memfree: %ld\n", memtotal1, memfree1);
    marcel_printf("Memtotal: %ld -- Memfree: %ld\n", memtotal2, memfree2);
    marcel_printf("Memtotal: %ld -- Memfree: %ld\n", memtotal3, memfree3);
  }

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
