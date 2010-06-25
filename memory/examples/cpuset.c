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

#include "mm_mami.h"
#include "mm_mami_private.h"
#include "helper.h"

int main(int argc, char * argv[]) {
  int node, cnode, err;
  void *ptr;
  mami_manager_t *memory_manager;

  common_init(&argc, argv, NULL);
  mami_init(&memory_manager, argc, argv);

  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    ptr = mami_malloc(memory_manager, 100, MAMI_MEMBIND_POLICY_SPECIFIC_NODE, node);
    MAMI_CHECK_MALLOC(ptr);
    cnode = -1;
    err = mami_locate(memory_manager, ptr, 100, &cnode);
    MAMI_CHECK_RETURN_VALUE(err, "mami_locate");
    if (cnode != node) {
      fprintf(stderr, "Error: Memory allocated on node %d (requested node %d)\n", cnode, node);
      return 1;
    }
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
