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
  void *ptr;
  mami_manager_t *memory_manager;
  int i, node, err;

  marcel_init(&argc,argv);
  mami_init(&memory_manager, argc, argv);

  ptr = mami_malloc(memory_manager, 9*getpagesize(), MAMI_MEMBIND_POLICY_DEFAULT, 0);
  MAMI_CHECK_MALLOC(ptr);

  for(i=0 ; i<9 ; i+=3 ) {
    err = mami_task_attach(memory_manager, ptr+(i*getpagesize()), 3*getpagesize(), marcel_self(), &node);
    MAMI_CHECK_RETURN_VALUE(err, "mami_task_attach");
  }
  for(i=0 ; i<9 ; i+=3 ) {
    err = mami_task_unattach(memory_manager, ptr+(i*getpagesize()), marcel_self());
    MAMI_CHECK_RETURN_VALUE(err, "mami_task_unattach");
  }

  mami_free(memory_manager, ptr);

  // Finish marcel
  mami_exit(&memory_manager);
  marcel_end();
  return 0;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
