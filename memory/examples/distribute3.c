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

#include <sys/mman.h>
#include <mm_mami.h>
#include <mm_mami_private.h>
#include <mm_helper.h>
#include "helper.h"

int main(int argc, char * argv[]) {
  int node, *nodes;
  int err, i;
  void *ptr;
  unsigned long nodemask;
  mami_manager_t *memory_manager;

  common_init(&argc, argv, NULL);
  mami_init(&memory_manager, argc, argv);

  ptr = mmap(NULL, 50000, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  for(i=0 ; i<memory_manager->nb_nodes ; i++) nodemask += (1<<i);
  err = _mm_mbind(ptr, 50000, MPOL_INTERLEAVE, &nodemask, memory_manager->nb_nodes+2, MPOL_MF_MOVE|MPOL_MF_STRICT);
  MAMI_CHECK_RETURN_VALUE(err, "_mm_mbind");
  memset(ptr, 0, 50000);

  err = mami_register(memory_manager, ptr, 50000);
  MAMI_CHECK_RETURN_VALUE(err, "mami_register");

  nodes = malloc(sizeof(int) * memory_manager->nb_nodes);
  for(i=0 ; i<memory_manager->nb_nodes ; i++) nodes[i] = i;
  err = mami_distribute(memory_manager, ptr, nodes, memory_manager->nb_nodes);
  MAMI_CHECK_RETURN_VALUE(err, "mami_distribute");

  err = mami_migrate_on_node(memory_manager, ptr, 1);
  MAMI_CHECK_RETURN_VALUE(err, "mami_migrate_on_node");
  err = mami_locate(memory_manager, ptr, 50000, &node);
  MAMI_CHECK_RETURN_VALUE(err, "mami_locate");

  if (node != 1) {
      fprintf(stderr, "Node is NOT 1 as expected but %d\n", node);
      return 1;
  }
  err = mami_check_pages_location(memory_manager, ptr, 50000, 1);
  MAMI_CHECK_RETURN_VALUE(err, "mami_check_pages_location");

  free(nodes);
  mami_unregister(memory_manager, ptr);
  munmap(ptr, 50000);
  mami_exit(&memory_manager);

  common_exit(NULL);
  return 0;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif