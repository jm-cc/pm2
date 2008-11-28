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

#include "marcel.h"
#include <sys/mman.h>
#include <numaif.h>

#define LOOPS 10000000
#define SIZE  1000

int marcel_main(int argc, char * argv[]) {
  void *buffer;
  marcel_memory_manager_t memory_manager;
  int i, err, node;

  marcel_init(&argc,argv);
  marcel_memory_init(&memory_manager);

  // Allocate the buffer
  buffer = marcel_memory_allocate_on_node(&memory_manager, SIZE*getpagesize(), 0);
  marcel_memory_locate(&memory_manager, buffer, SIZE*getpagesize(), &node);
  marcel_printf("Buffer located on node %d\n", node);

  // Migrate the buffer
  for(i=0 ; i<LOOPS ; i++) {
    if (i%2 == 0) {
      err = marcel_memory_migrate_pages(&memory_manager, buffer, 1);
    }
    else {
      err = marcel_memory_migrate_pages(&memory_manager, buffer, 0);
    }
    if (err < 0) {
      perror("mami_migrate_pages");
      return 1;
    }
  }

  // Finish marcel
  marcel_memory_free(&memory_manager, buffer);
  marcel_memory_exit(&memory_manager);
  marcel_end();
  return 0;
}
