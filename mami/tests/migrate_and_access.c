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
#include <sys/mman.h>
#include "mami.h"
#include "mami_thread.h"
#include "mami_private.h"
#include "helper.h"

#if defined(MAMI_ENABLED)

#define SIZE  100000

int *buffer;
mami_manager_t *memory_manager;

any_t t_migrate(any_t arg);
any_t t_access(any_t arg);

int main(int argc, char * argv[]) {
  th_mami_t threads[2];
  int loops=1000;

  mami_init(&memory_manager, argc, argv);

  if (memory_manager->nb_nodes < 2) {
    fprintf(stderr, "This application needs at least two NUMA nodes.\n");
    mami_exit(&memory_manager);
    return MAMI_TEST_SKIPPED;
  }
  else {
    if (argc == 2) {
      loops = atoi(argv[1]);
    }

    // Allocate the buffer
    buffer = mami_malloc(memory_manager, SIZE*sizeof(int), MAMI_MEMBIND_POLICY_DEFAULT, 0);

    // Start the threads
    th_mami_create(&threads[0], NULL, t_migrate, (any_t) &loops);
    th_mami_create(&threads[1], NULL, t_access, (any_t) &loops);

    // Wait for the threads to complete
    th_mami_join(threads[0], NULL);
    th_mami_join(threads[1], NULL);

    mami_free(memory_manager, buffer);
  }

  mami_exit(&memory_manager);
  return 0;
}

any_t t_migrate(any_t arg) {
  int i;
  int *loops = (int *) arg;

  for(i=0 ; i<*loops ; i++) {
    if (i%2 == 0) {
      mami_migrate_on_node(memory_manager, buffer, 1);
    }
    else {
      mami_migrate_on_node(memory_manager, buffer, 0);
    }
    //if (i %1000) printf("Migrate ...\n");
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
    //if (i %1000) printf("Access ...\n");
  }
  return 0;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
  return MAMI_TEST_SKIPPED;
}
#endif

