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
#include <mm_mami_private.h>
#include "helper.h"

int main(int argc, char * argv[]) {
  void *ptr1, *ptr2, *ptr3, *ptr4, *ptr5;
  size_t size;
  mami_manager_t *memory_manager;

  common_init(&argc, argv, NULL);
  mami_init(&memory_manager, argc, argv);

  size = memory_manager->initially_preallocated_pages * getpagesize();
  ptr1 = mami_malloc(memory_manager, size+1, MAMI_MEMBIND_POLICY_DEFAULT, 0);
  MAMI_CHECK_MALLOC(ptr1);
  ptr2 = mami_malloc(memory_manager, size/2, MAMI_MEMBIND_POLICY_DEFAULT, 0);
  MAMI_CHECK_MALLOC(ptr2);
  ptr3 = mami_malloc(memory_manager, size/2, MAMI_MEMBIND_POLICY_DEFAULT, 0);
  MAMI_CHECK_MALLOC(ptr3);
  ptr4 = mami_malloc(memory_manager, 4, MAMI_MEMBIND_POLICY_DEFAULT, 0);
  MAMI_CHECK_MALLOC(ptr4);
  ptr5 = mami_malloc(memory_manager, size, MAMI_MEMBIND_POLICY_DEFAULT, 0);
  MAMI_CHECK_MALLOC(ptr5);

  mami_free(memory_manager, ptr1);
  mami_free(memory_manager, ptr2);
  mami_free(memory_manager, ptr3);
  mami_free(memory_manager, ptr4);
  mami_free(memory_manager, ptr5);

  mami_exit(&memory_manager);
  common_exit(NULL);
  return 0;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
