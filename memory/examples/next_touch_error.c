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
  mami_manager_t *memory_manager;
  void *b;
  int err;

  common_init(&argc, argv, NULL);
  mami_init(&memory_manager, argc, argv);

  b = malloc(100);
  mami_unset_kernel_migration(memory_manager);
  err = mami_migrate_on_next_touch(memory_manager, b);
  MAMI_CHECK_RETURN_VALUE_IS(err, EINVAL, "mami_migrate_on_next_touch");

  mami_exit(&memory_manager);
  common_exit(NULL);
  return 0;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
