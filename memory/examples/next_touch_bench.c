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
#include "mm_mami_thread.h"

#if defined(MM_MAMI_ENABLED)

mami_manager_t *memory_manager;

int *b;

any_t writer(any_t arg) {
  int *nbpages = (int *) arg;
  b = mami_malloc(memory_manager, (*nbpages)*getpagesize(), MAMI_MEMBIND_POLICY_DEFAULT, 0);
  mami_unset_kernel_migration(memory_manager);
  mami_migrate_on_next_touch(memory_manager, b);
  return 0;
}

any_t reader(any_t arg) {
  int snode, dnode;
  int *nbpages = (int *) arg;
  struct timeval tv1, tv2;
  unsigned long us, ns;

  mami_locate(memory_manager, b, 0, &snode);

  gettimeofday(&tv1, NULL);
  b[1] = 42;
  gettimeofday(&tv2, NULL);

  mami_locate(memory_manager, b, 0, &dnode);

  us = (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);
  ns = us * 1000;

  mami_free(memory_manager, b);
  printf("%d\t%d\t%d\t%ld\n", snode, dnode, *nbpages, ns);
  return 0;
}

int main(int argc, char * argv[]) {
  th_mami_t threads[2];
  th_mami_attr_t attr;
  int i;
  int source=-1, dest=-1, nbpages=-1;

  common_init(&argc, argv, NULL);
  mami_init(&memory_manager, argc, argv);
  th_mami_attr_init(&attr);

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
    printf("Error. Argument missing\n");
    mami_exit(&memory_manager);
    common_exit(NULL);
    return  -1;
  }

  // Start the thread on the numa node #source
  th_mami_attr_setnode_level(&attr, source);
  th_mami_create(&threads[0], &attr, writer, (any_t)&nbpages);
  th_mami_join(threads[0], NULL);

  // Start the thread on the numa node #1
  th_mami_attr_setnode_level(&attr, dest);
  th_mami_create(&threads[1], &attr, reader, (any_t)&nbpages);
  th_mami_join(threads[1], NULL);

  mami_exit(&memory_manager);
  common_exit(NULL);
  return 0;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
