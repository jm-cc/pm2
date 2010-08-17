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
  float cost;
  mami_manager_t *memory_manager;

  common_init(&argc, argv, NULL);
  mami_init(&memory_manager, argc, argv);

  mami_migration_cost(memory_manager, 0, 1, 100, &cost);
  printf("Cost for migrating %d bits from #%d to #%d = %f\n", 100, 0, 1, cost);

  mami_cost_for_write_access(memory_manager, 0, 1, 100, &cost);
  printf("Cost for writing %d bits from #%d to #%d = %f\n", 100, 0, 1, cost);

  mami_cost_for_read_access(memory_manager, 0, 1, 100, &cost);
  printf("Cost for reading %d bits from #%d to #%d = %f\n", 100, 0, 1, cost);

  mami_exit(&memory_manager);

  common_exit(NULL);
  return 0;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif