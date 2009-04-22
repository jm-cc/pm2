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

#if defined(MM_MAMI_ENABLED)

int marcel_main(int argc, char * argv[]) {
  marcel_memory_manager_t memory_manager;
  void *b;
  int err;

  marcel_init(&argc,argv);
  marcel_memory_init(&memory_manager);

  b = malloc(100);
  memory_manager.kernel_nexttouch_migration = 0;
  err = marcel_memory_migrate_on_next_touch(&memory_manager, b);
  if (err < 0) {
    perror("marcel_memory_migrate_on_next_touch");
  }
  else {
    marcel_printf("marcel_memory_migrate_on_next_touch should have failed\n");
  }

  // Finish marcel
  marcel_memory_exit(&memory_manager);
  marcel_end();
  return 0;
}

#else
int marcel_main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
