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

#include "mami.h"

#if defined(MAMI_ENABLED)

int marcel_main(int argc, char **argv) {
  marcel_memory_manager_t memory_manager;
  int i, err;
  int minsource, maxsource, mindest, maxdest;

  marcel_init(&argc,argv);
  marcel_memory_init(&memory_manager);

  minsource = 0;
  maxsource = memory_manager.nb_nodes-1;
  mindest = 0;
  maxdest = memory_manager.nb_nodes-1;

  for(i=1 ; i<argc ; i++) {
    if (!strcmp(argv[i], "-src")) {
      if (i+1 >= argc) abort();
      minsource = atoi(argv[i+1]);
      maxsource = atoi(argv[i+1]);
    }
    else if (!strcmp(argv[i], "-dest")) {
      if (i+1 >= argc) abort();
      mindest = atoi(argv[i+1]);
      maxdest = atoi(argv[i+1]);
    }
  }

  err = marcel_memory_sampling_of_memory_access(&memory_manager, minsource, maxsource, mindest, maxdest);
  if (err < 0) perror("marcel_memory_sampling_of_memory_migration");

  // Finish marcel
  marcel_memory_exit(&memory_manager);
  marcel_end();
  return err;
}

#else
int marcel_main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
