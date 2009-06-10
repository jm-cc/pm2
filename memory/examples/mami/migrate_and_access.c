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
#include <sys/mman.h>

#if defined(MM_MAMI_ENABLED)

#define SIZE  100000

int *buffer;
mami_manager_t *memory_manager;

any_t t_migrate(any_t arg) {
  int i;
  int *loops = (int *) arg;

  for(i=0 ; i<*loops ; i++) {
    if (i%2 == 0) {
      mami_migrate_pages(memory_manager, buffer, 1);
    }
    else {
      mami_migrate_pages(memory_manager, buffer, 0);
    }
    //if (i %1000) marcel_printf("Migrate ...\n");
  }
  return 0;
}

any_t t_access(any_t arg) {
  int i;
  int *loops = (int *) arg;
  int res=0;

  for(i=0 ; i<*loops ; i++) {
    if (i%2 == 0) {
      res += buffer[i];
    }
    else {
      res -= buffer[i];
    }
    //if (i %1000) marcel_printf("Access ...\n");
  }
  return 0;
}

int main(int argc, char * argv[]) {
  marcel_t threads[2];
  int loops=1000;

  marcel_init(&argc,argv);
  mami_init(&memory_manager);

  if (argc == 2) {
    loops = atoi(argv[1]);
  }

  // Allocate the buffer
  buffer = mami_malloc(memory_manager, SIZE*sizeof(int), MAMI_MEMBIND_POLICY_DEFAULT, 0);

  // Start the threads
  marcel_create(&threads[0], NULL, t_migrate, (any_t) &loops);
  marcel_create(&threads[1], NULL, t_access, (any_t) &loops);

  // Wait for the threads to complete
  marcel_join(threads[0], NULL);
  marcel_join(threads[1], NULL);

  // Finish marcel
  mami_free(memory_manager, buffer);
  mami_exit(&memory_manager);
  marcel_end();
  return 0;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
