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

#include <mm_mami.h>
#include "helper.h"

int main(int argc, char * argv[]) {
  int node, err;
  void *ptr;
  mami_manager_t *memory_manager;

  common_init(&argc, argv, NULL);
  mami_init(&memory_manager, argc, argv);

  ptr = mami_malloc(memory_manager, 1000, MAMI_MEMBIND_POLICY_SPECIFIC_NODE, 0);
  MAMI_CHECK_MALLOC(ptr);
  err = mami_locate(memory_manager, ptr, 1000, &node);
  MAMI_CHECK_RETURN_VALUE(err, "mami_locate");
  if (node != 0) {
    fprintf(stderr, "(1) The memory is NOT located on node #0 but on node #%d\n", node);
    return 1;
  }
  mami_free(memory_manager, ptr);

  err = mami_locate(memory_manager, NULL, 0, &node);
  MAMI_CHECK_RETURN_VALUE_IS(err, EINVAL, "(2) mami_locate");

  ptr = malloc(1000);
  err = mami_locate(memory_manager, ptr, 1000, &node);
  MAMI_CHECK_RETURN_VALUE_IS(err, EINVAL, "(2) mami_locate");
  free(ptr);

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
