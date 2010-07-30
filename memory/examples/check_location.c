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

#if defined(MM_MAMI_ENABLED)

#include <mm_mami.h>
#include <mm_mami_private.h>
#include "helper.h"

int main(int argc, char * argv[]) {
  int n, node, err;
  void *ptr;
  mami_manager_t *memory_manager;

  common_init(&argc, argv, NULL);
  mami_init(&memory_manager, argc, argv);

  for(n=0 ; n<memory_manager->nb_nodes ; n++) {
    ptr = mami_malloc(memory_manager, 4*getpagesize(), MAMI_MEMBIND_POLICY_SPECIFIC_NODE, n);
    MAMI_CHECK_MALLOC(ptr);
    err = mami_locate(memory_manager, ptr, 1, &node);
    MAMI_CHECK_RETURN_VALUE(err, "mami_locate");
    if (node != n) {
      fprintf(stderr, "Wrong location: %d, asked for %d\n", node, n);
      return 1;
    }
    err = mami_check_pages_location(memory_manager, ptr, 1, n);
    MAMI_CHECK_RETURN_VALUE(err, "mami_check_pages_location");
    mami_free(memory_manager, ptr);
  }

  mami_exit(&memory_manager);
  common_exit(NULL);
  fprintf(stdout, "Success\n");
  return 0;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
