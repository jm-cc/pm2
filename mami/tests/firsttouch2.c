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
#include <unistd.h>
#include "mami.h"
#include "mami_private.h"

#if defined(MAMI_ENABLED)

int main(int argc, char * argv[]) {
  mami_manager_t *memory_manager;
  int *ptr, *ptr2;
  size_t size;
  int err;

  mami_init(&memory_manager, argc, argv);

  err = mami_membind(memory_manager, MAMI_MEMBIND_POLICY_FIRST_TOUCH, 0);
  if (err < 0) perror("mami_membind unexpectedly failed");

  size = memory_manager->initially_preallocated_pages * getpagesize();
  ptr = mami_malloc(memory_manager, size/2, MAMI_MEMBIND_POLICY_DEFAULT, 0);
  ptr2 = mami_malloc(memory_manager, size, MAMI_MEMBIND_POLICY_DEFAULT, 0);

  mami_free(memory_manager, ptr);
  mami_free(memory_manager, ptr2);

  mami_exit(&memory_manager);
  return 0;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
  return MAMI_TEST_SKIPPED;
}
#endif
