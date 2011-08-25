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
#include "helper.h"

#if defined(MAMI_ENABLED)

int main(int argc, char * argv[]) {
  float cost;
  mami_manager_t *memory_manager;

  mami_init(&memory_manager, argc, argv);

  mami_migration_cost(memory_manager, 0, 1, 100, &cost);
  mprintf(stdout, "Cost for migrating %d bits from #%d to #%d = %f\n", 100, 0, 1, cost);

  mami_cost_for_write_access(memory_manager, 0, 1, 100, &cost);
  mprintf(stdout, "Cost for writing %d bits from #%d to #%d = %f\n", 100, 0, 1, cost);

  mami_cost_for_read_access(memory_manager, 0, 1, 100, &cost);
  mprintf(stdout, "Cost for reading %d bits from #%d to #%d = %f\n", 100, 0, 1, cost);

  mami_exit(&memory_manager);

  return 0;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
  return MAMI_TEST_SKIPPED;
}
#endif
