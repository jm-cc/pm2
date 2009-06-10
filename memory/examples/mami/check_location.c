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

int main(int argc, char * argv[]) {
  int n, node;
  void *ptr;
  mami_manager_t *memory_manager;

  marcel_init(&argc,argv);
  mami_init(&memory_manager);

  for(n=0 ; n<marcel_nbnodes ; n++) {
    ptr = mami_malloc(memory_manager, 4*getpagesize(), MAMI_MEMBIND_POLICY_SPECIFIC_NODE, n);
    mami_locate(memory_manager, ptr, 1, &node);
    if (node != n) marcel_printf("Wrong location: %d, asked for %d\n", node, n);
    mami_check_pages_location(memory_manager, ptr, 1, n);
    mami_free(memory_manager, ptr);
  }

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
