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

mami_manager_t *memory_manager;

void allocation_and_migration(int cpu, int mem) {
  void *buffer;
  size_t size;
  int bnode, anode;

  size = getpagesize() * 2;

  buffer = mami_malloc(memory_manager, size, MAMI_MEMBIND_POLICY_SPECIFIC_NODE, mem);
  mami_locate(memory_manager, buffer, size, &bnode);
  mami_migrate_on_node(memory_manager, buffer, cpu);
  mami_locate(memory_manager, buffer, size, &anode);

  if (bnode == mem && anode == cpu) {
    printf("[%d:%d] Data has succesfully been migrated\n", mem, cpu);
  }
  else {
    printf("[%d:%d] Error when migrating data - bnode = %d -- anode = %d\n", mem, cpu, bnode, anode);
  }
  mami_free(memory_manager, buffer);
}

any_t migration(any_t arg) {
  int *cpu, mem;
  size_t size;
  float migration_cost, reading_cost;

  cpu = (int *)arg;
  size = getpagesize() * 10000;

  for(mem=0 ; mem<memory_manager->nb_nodes ; mem++) {
    if (mem == *cpu) continue;

    mami_cost_for_read_access(memory_manager, *cpu, mem, size, &reading_cost);
    mami_migration_cost(memory_manager, *cpu, mem, size, &migration_cost);
    //if (migration_cost < reading_cost) {
    allocation_and_migration(*cpu, mem);
    //}
  }
  return 0;
}

int main(int argc, char * argv[]) {
  int cpu;
  th_mami_t thread;

  common_init(&argc, argv, NULL);
  mami_init(&memory_manager, argc, argv);

  if (argc == 3) {
    allocation_and_migration(atoi(argv[1]), atoi(argv[2]));
  }
  else {
    for(cpu=0 ; cpu<memory_manager->nb_nodes ; cpu++) {
      th_mami_create(&thread, NULL, migration, (any_t) &cpu);
      th_mami_join(thread, NULL);
    }
  }

  mami_exit(&memory_manager);
  common_exit(NULL);
  return 0;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
  return 0;
}
#endif
