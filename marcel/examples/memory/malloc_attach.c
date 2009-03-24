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

#if defined(MARCEL_MAMI_ENABLED)

int marcel_main(int argc, char * argv[]) {
  void *ptr;
  marcel_memory_manager_t memory_manager;
  int i, node;

  marcel_init(&argc,argv);
  marcel_memory_init(&memory_manager);

  ptr = marcel_memory_malloc(&memory_manager, 9*memory_manager.normalpagesize, MARCEL_MEMORY_MEMBIND_POLICY_DEFAULT, 0);

  for(i=0 ; i<9 ; i+=3 ) {
    marcel_memory_task_attach(&memory_manager, ptr+(i*memory_manager.normalpagesize), 3*memory_manager.normalpagesize, marcel_self(), &node);
  }
  for(i=0 ; i<9 ; i+=3 ) {
    marcel_memory_task_unattach(&memory_manager, ptr+(i*memory_manager.normalpagesize), marcel_self());
  }

  marcel_memory_free(&memory_manager, ptr);

  // Finish marcel
  marcel_printf("Success\n");
  marcel_memory_exit(&memory_manager);
  marcel_end();
  return 0;
}

#else
int marcel_main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
