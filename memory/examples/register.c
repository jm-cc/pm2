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
#include <malloc.h>

#if defined(MM_MAMI_ENABLED)

#include "mm_mami.h"
#include "helper.h"

int main(int argc, char * argv[]) {
  int err;
  void *ptr;
  mami_manager_t *memory_manager;

  common_init(&argc, argv, NULL);
  mami_init(&memory_manager, argc, argv);

  ptr = memalign(getpagesize(), 50*getpagesize());
  err = mami_register(memory_manager, ptr, 50000);
  MAMI_CHECK_RETURN_VALUE(err, "mami_register(1)");

  err = mami_register(memory_manager, ptr, 50000);
  MAMI_CHECK_RETURN_VALUE_IS(err, EINVAL, "mami_register(2)");

  err = mami_unregister(memory_manager, ptr);
  MAMI_CHECK_RETURN_VALUE(err, "mami_unregister(1)");

  err = mami_unregister(memory_manager, ptr);
  MAMI_CHECK_RETURN_VALUE_IS(err, EINVAL, "mami_unregister(2)");

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
