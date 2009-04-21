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

#include "marcel_memory.h"

#if defined(MARCEL_MAMI_ENABLED)

int marcel_main(int argc, char * argv[]) {
  float cost;
  marcel_memory_manager_t memory_manager;

  marcel_init(&argc,argv);
  marcel_memory_init(&memory_manager);

  marcel_memory_migration_cost(&memory_manager, 0, 1, 100, &cost);
  marcel_printf("Cost for migrating %d bits from #%d to #%d = %f\n", 100, 0, 1, cost);

  marcel_memory_cost_for_write_access(&memory_manager, 0, 1, 100, &cost);
  marcel_printf("Cost for writing %d bits from #%d to #%d = %f\n", 100, 0, 1, cost);

  marcel_memory_cost_for_read_access(&memory_manager, 0, 1, 100, &cost);
  marcel_printf("Cost for reading %d bits from #%d to #%d = %f\n", 100, 0, 1, cost);

  marcel_memory_exit(&memory_manager);

  // Finish marcel
  marcel_end();
  return 0;
}

#else
int marcel_main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
