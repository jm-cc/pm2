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
#include <string.h>
#include "mami.h"
#include "mami_private.h"

#if defined(MAMI_ENABLED)

int main(int argc, char **argv) {
  mami_manager_t *memory_manager;
  int i, err;
  int minsource, maxsource, mindest, maxdest;

  mami_init(&memory_manager, argc, argv);

  minsource = 0;
  maxsource = memory_manager->nb_nodes-1;
  mindest = 0;
  maxdest = memory_manager->nb_nodes-1;

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

  err = mami_sampling_of_memory_access(memory_manager, minsource, maxsource, mindest, maxdest);
  if (err < 0) perror("mami_sampling_of_memory_migration");

  mami_exit(&memory_manager);
  return err;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
