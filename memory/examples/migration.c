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

#include "mami.h"

#if defined(MAMI_ENABLED)

marcel_memory_manager_t memory_manager;

void allocation_and_migration(int cpu, int mem) {
  void *buffer;
  size_t size;
  int bnode, anode;

  size = memory_manager.normalpagesize * 2;

  buffer = marcel_memory_malloc(&memory_manager, size, MARCEL_MEMORY_MEMBIND_POLICY_SPECIFIC_NODE, mem);
  marcel_memory_locate(&memory_manager, buffer, size, &bnode);
  marcel_printf("Node before migration %d\n", bnode);

  marcel_memory_migrate_pages(&memory_manager, buffer, cpu);

  marcel_memory_locate(&memory_manager, buffer, size, &anode);
  marcel_printf("Node after migration %d\n", anode);
  marcel_memory_free(&memory_manager, buffer);
}

any_t migration(any_t arg) {
  int cpu, mem;
  size_t size;
  float migration_cost, reading_cost;

  cpu = marcel_self()->id;
  size = memory_manager.normalpagesize * 10000;

  for(mem=0 ; mem<marcel_nbnodes ; mem++) {
    if (mem == cpu) continue;

    marcel_memory_cost_for_read_access(&memory_manager,
				       cpu, mem,
				       size, &reading_cost);
    marcel_memory_migration_cost(&memory_manager,
                                 cpu, mem,
                                 size, &migration_cost);
    marcel_printf("\n[%d:%d] Migration cost %f - Reading access cost %f\n", cpu, mem, migration_cost, reading_cost);
    if (migration_cost < reading_cost) {
      marcel_printf("Migration is cheaper than remote reading. Let's migrate...\n");
      allocation_and_migration(cpu, mem);
    }
  }
  return 0;
}

int marcel_main(int argc, char * argv[]) {
  int cpu;
  marcel_t thread;
  marcel_attr_t attr;

  // Initialise marcel
  marcel_init(&argc, argv);
  marcel_attr_init(&attr);
  marcel_memory_init(&memory_manager);

  if (argc == 3) {
    allocation_and_migration(atoi(argv[1]), atoi(argv[2]));
  }
  else {
    for(cpu=0 ; cpu<marcel_nbnodes ; cpu++) {
      marcel_attr_setid(&attr, cpu);
      marcel_attr_settopo_level(&attr, &marcel_topo_node_level[cpu]);
      marcel_create(&thread, &attr, migration, NULL);
      marcel_join(thread, NULL);
    }
  }

  // Finish marcel
  marcel_memory_exit(&memory_manager);
  marcel_end();
  return 0;
}

#else
int marcel_main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
  return 0;
}
#endif
