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
#include "mm_mami_private.h"

#if defined(MM_MAMI_ENABLED)

int main(int argc, char * argv[]) {
  int *ptr, *ptr2;
  size_t bigsize;
  mami_manager_t *memory_manager;

  common_init(&argc, argv, NULL);
  mami_init(&memory_manager);

  if (memory_manager->huge_page_free[0] == 0) {
    printf("No huge page available.\n");
  }
  else {
    mami_membind(memory_manager, MAMI_MEMBIND_POLICY_HUGE_PAGES, 0);
    bigsize = memory_manager->huge_page_free[0] * memory_manager->huge_page_size;
    ptr = mami_malloc(memory_manager, bigsize+1, MAMI_MEMBIND_POLICY_DEFAULT, 0);
    perror("mami_malloc succesfully failed");

    ptr = mami_malloc(memory_manager, bigsize/2, MAMI_MEMBIND_POLICY_DEFAULT, 0);
    if (!ptr) {
      perror("mami_malloc unexpectedly failed");
    }
    else {
      ptr[10] = 10;
      printf("ptr[10]=%d\n", ptr[10]);

      ptr2 =  mami_malloc(memory_manager, bigsize/2, MAMI_MEMBIND_POLICY_DEFAULT, 0);
      if (!ptr2) {
	perror("mami_malloc unexpectedly failed");
      }
      else {
	ptr2[20] = 20;
	printf("ptr2[20]=%d\n", ptr2[20]);
	mami_free(memory_manager, ptr2);
      }
      mami_free(memory_manager, ptr);
    }
  }

  mami_exit(&memory_manager);

  common_exit(NULL);
  return 0;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
