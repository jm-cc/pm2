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

marcel_memory_manager_t memory_manager;

int *b;

any_t writer(any_t arg) {
  int *nbpages = (int *) arg;
  b = marcel_memory_malloc(&memory_manager, (*nbpages)*memory_manager.normalpagesize);
  marcel_memory_migrate_on_next_touch(&memory_manager, b);
  return 0;
}

any_t reader(any_t arg) {
  int snode, dnode;
  int *nbpages = (int *) arg;
  struct timeval tv1, tv2;
  unsigned long us, ns, bandwidth;

  marcel_memory_locate(&memory_manager, b, &snode);

  gettimeofday(&tv1, NULL);
  b[1] = 42;
  gettimeofday(&tv2, NULL);

  marcel_memory_locate(&memory_manager, b, &dnode);

  us = (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);
  ns = us * 1000;

  marcel_memory_free(&memory_manager, b);
  marcel_printf("%d\t%d\t%d\t%ld\n", snode, dnode, *nbpages, ns);
  return 0;
}

int marcel_main(int argc, char * argv[]) {
  marcel_t threads[2];
  marcel_attr_t attr;
  int i;
  int source=-1, dest=-1, nbpages=-1;

  marcel_init(&argc,argv);
  marcel_memory_init(&memory_manager);
  marcel_attr_init(&attr);

  for(i=1 ; i<argc ; i+=2) {
    if (!strcmp(argv[i], "-src")) {
      if (i+1 >= argc) abort();
      source = atoi(argv[i+1]);
    }
    else if (!strcmp(argv[i], "-dest")) {
      if (i+1 >= argc) abort();
      dest = atoi(argv[i+1]);
    }
    else if (!strcmp(argv[i], "-pages")) {
      if (i+1 >= argc) abort();
      nbpages = atoi(argv[i+1]);
    }
  }
  if (source == -1 || dest == -1 || nbpages == -1) {
    marcel_printf("Error. Argument missing\n");
    marcel_end();
    return  -1;
  }

  // Start the thread on the numa node #source
  marcel_attr_settopo_level(&attr, &marcel_topo_node_level[source]);
  marcel_create(&threads[0], &attr, writer, (any_t)&nbpages);
  marcel_join(threads[0], NULL);

  // Start the thread on the numa node #1
  marcel_attr_settopo_level(&attr, &marcel_topo_node_level[dest]);
  marcel_create(&threads[1], &attr, reader, (any_t)&nbpages);
  marcel_join(threads[1], NULL);

  // Finish marcel
  marcel_memory_exit(&memory_manager);
  marcel_end();
  return 0;
}

#else
int marcel_main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MAMI to be enabled\n");
}
#endif
