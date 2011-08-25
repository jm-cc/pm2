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
#include <mami_private.h>
#include "helper.h"

int main(int argc, char * argv[]) {
  int *ptr, *ptr2, err;
  size_t bigsize;
  mami_manager_t *memory_manager;

  mami_init(&memory_manager, argc, argv);

  if (memory_manager->huge_page_free[0] == 0) {
    fprintf(stderr,"No huge page available.\n");
    mami_exit(&memory_manager);
    return MAMI_TEST_SKIPPED;
  }
  else {
    err = mami_membind(memory_manager, MAMI_MEMBIND_POLICY_HUGE_PAGES, 0);
    MAMI_CHECK_RETURN_VALUE(err, "mami_membind");

    bigsize = memory_manager->huge_page_free[0] * memory_manager->huge_page_size;
    ptr = mami_malloc(memory_manager, bigsize+1, MAMI_MEMBIND_POLICY_DEFAULT, 0);
    MAMI_CHECK_MALLOC_HAS_FAILED(ptr);

    ptr = mami_malloc(memory_manager, bigsize/2, MAMI_MEMBIND_POLICY_DEFAULT, 0);
    MAMI_CHECK_MALLOC(ptr);

    ptr[10] = 10;
    mprintf(stderr, "ptr[10]=%d\n", ptr[10]);

    ptr2 =  mami_malloc(memory_manager, bigsize/2, MAMI_MEMBIND_POLICY_DEFAULT, 0);
    MAMI_CHECK_MALLOC(ptr2);

    ptr2[20] = 20;
    mprintf(stderr, "ptr2[20]=%d\n", ptr2[20]);

    mami_free(memory_manager, ptr2);
    mami_free(memory_manager, ptr);
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
