/* MaMI --- NUMA Memory Interface
 *
 * Copyright (C) 2008, 2009, 2010, 2011  Centre National de la Recherche Scientifique
 *
 * MaMI is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * MaMI is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU Lesser General Public License in COPYING.LGPL for more details.
 */
#include <stdio.h>
#include "mami.h"
#include "mami_private.h"
#include "helper.h"

#if defined(MAMI_ENABLED)

mami_manager_t *memory_manager;
int allocation_and_migration(int cpu, int mem);
any_t migration(any_t arg);

int main(int argc, char * argv[]) {
  int cpu;
  th_mami_t thread;

  mami_init(&memory_manager, argc, argv);
  if (memory_manager->nb_nodes < 2) {
    fprintf(stderr,"This application needs at least two NUMA nodes.\n");
    mami_exit(&memory_manager);
    return MAMI_TEST_SKIPPED;
  }

  if (argc == 3) {
    allocation_and_migration(atoi(argv[1]), atoi(argv[2]));
  }
  else {
    for(cpu=0 ; cpu<memory_manager->nb_nodes ; cpu++) {
      any_t status;

      th_mami_create(&thread, NULL, migration, (any_t) &cpu);
      th_mami_join(thread, &status);
      if (status == ALL_IS_NOT_OK) return 1;
    }
  }

  mami_exit(&memory_manager);
  return 0;
}

int allocation_and_migration(int cpu, int mem) {
  void *buffer;
  size_t size;
  int bnode, anode, ret;

  size = getpagesize() * 2;

  buffer = mami_malloc(memory_manager, size, MAMI_MEMBIND_POLICY_SPECIFIC_NODE, mem);
  mami_locate(memory_manager, buffer, size, &bnode);
  mami_migrate_on_node(memory_manager, buffer, cpu);
  mami_locate(memory_manager, buffer, size, &anode);

  if (bnode == mem && anode == cpu) {
    mprintf(stderr, "[%d:%d] Data has succesfully been migrated\n", mem, cpu);
    ret=0;
  }
  else {
    mprintf(stderr, "[%d:%d] Error when migrating data - bnode = %d -- anode = %d\n", mem, cpu, bnode, anode);
    ret=1;
  }
  mami_free(memory_manager, buffer);
  return ret;
}

any_t migration(any_t arg) {
  int *cpu, mem, ret;
  size_t size;
  float migration_cost, reading_cost;

  cpu = (int *)arg;
  size = getpagesize() * 10000;

  for(mem=0 ; mem<memory_manager->nb_nodes ; mem++) {
    if (mem == *cpu) continue;

    mami_cost_for_read_access(memory_manager, *cpu, mem, size, &reading_cost);
    mami_migration_cost(memory_manager, *cpu, mem, size, &migration_cost);
    //if (migration_cost < reading_cost) {
    ret = allocation_and_migration(*cpu, mem);
    //}
  }
  if (ret) return ALL_IS_NOT_OK; else return ALL_IS_OK;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
  return MAMI_TEST_SKIPPED;
}
#endif

