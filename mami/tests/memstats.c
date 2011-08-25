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

#if defined(MAMI_ENABLED)

#include <mami.h>
#include "helper.h"

int main(int argc, char * argv[]) {
  mami_manager_t *memory_manager;
  void *ptr;
  int err;
  unsigned long memtotal1, memfree1;
  unsigned long memtotal2, memfree2;
  unsigned long memtotal3, memfree3;

  mami_init(&memory_manager, argc, argv);

  err = mami_stats(memory_manager, 0, MAMI_STAT_MEMORY_TOTAL, &memtotal1);
  MAMI_CHECK_RETURN_VALUE(err, "mami_stats");
  err = mami_stats(memory_manager, 0, MAMI_STAT_MEMORY_FREE, &memfree1);
  MAMI_CHECK_RETURN_VALUE(err, "mami_stats");

  ptr = mami_malloc(memory_manager, 100, MAMI_MEMBIND_POLICY_SPECIFIC_NODE, 0);
  MAMI_CHECK_MALLOC(ptr);

  err = mami_stats(memory_manager, 0, MAMI_STAT_MEMORY_TOTAL, &memtotal2);
  MAMI_CHECK_RETURN_VALUE(err, "mami_stats");
  err = mami_stats(memory_manager, 0, MAMI_STAT_MEMORY_FREE, &memfree2);
  MAMI_CHECK_RETURN_VALUE(err, "mami_stats");

  mami_free(memory_manager, ptr);

  err = mami_stats(memory_manager, 0, MAMI_STAT_MEMORY_TOTAL, &memtotal3);
  MAMI_CHECK_RETURN_VALUE(err, "mami_stats");
  err = mami_stats(memory_manager, 0, MAMI_STAT_MEMORY_FREE, &memfree3);
  MAMI_CHECK_RETURN_VALUE(err, "mami_stats");

  if (memfree1 == memfree3 && (memfree1-memfree2) == getpagesize()) {
    mprintf(stdout, "Success\n");
  }
  else {
    mprintf(stderr, "Error\n");
    mprintf(stderr, "Memtotal: %ld -- Memfree: %ld\n", memtotal1, memfree1);
    mprintf(stderr, "Memtotal: %ld -- Memfree: %ld\n", memtotal2, memfree2);
    mprintf(stderr, "Memtotal: %ld -- Memfree: %ld\n", memtotal3, memfree3);
    return 1;
  }

  mami_exit(&memory_manager);
  return 0;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
  return MAMI_TEST_SKIPPED;
}
#endif
