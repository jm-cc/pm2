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
#include <errno.h>
#include <malloc.h>

#if defined(MM_MAMI_ENABLED)

static marcel_memory_manager_t memory_manager;

static void split(void *ptr, size_t size);
static void attach(void *ptr, size_t size);

any_t my_thread(any_t arg) {
  int err;
  void *ptr;

  marcel_fprintf(stderr, "Spliting memory area allocated by MaMI\n");
  ptr = marcel_memory_malloc(&memory_manager, 10*getpagesize(), MARCEL_MEMORY_MEMBIND_POLICY_SPECIFIC_NODE, marcel_nbnodes-1);
  split(ptr, 10*getpagesize());
  marcel_memory_free(&memory_manager, ptr);

  marcel_fprintf(stderr, "\nSpliting unknown memory area\n");
  ptr = memalign(getpagesize(), 50*getpagesize());
  memset(ptr, 0, 50*getpagesize());
  split(ptr, 50*getpagesize());

  err = marcel_memory_register(&memory_manager, ptr, 50*getpagesize());
  if (err < 0) {
    perror("marcel_memory_register unexpectedly failed");
  }

  marcel_fprintf(stderr, "\nSpliting memory area (%ld) not allocated by MaMI\n", 50*getpagesize());
  split(ptr, 50*getpagesize());
  err = marcel_memory_unregister(&memory_manager, ptr);
  if (err < 0) {
    perror("marcel_memory_unregister unexpectedly failed");
  }
  free(ptr);
}

int marcel_main(int argc, char * argv[]) {
  marcel_t thread;
  marcel_attr_t attr;

  marcel_init(&argc,argv);
  marcel_attr_init(&attr);
  marcel_memory_init(&memory_manager);

  marcel_attr_settopo_level(&attr, &marcel_topo_node_level[marcel_nbnodes-1]);
  marcel_create(&thread, &attr, my_thread, NULL);
  marcel_join(thread, NULL);

  marcel_memory_exit(&memory_manager);

  // Finish marcel
  marcel_end();
  return 0;
}

static void split(void *ptr, size_t size) {
  int err, i;
  void **newptrs;

  newptrs = malloc(10 * sizeof(void **));
  err = marcel_memory_split(&memory_manager, ptr, 10, newptrs);
  if (err < 0) {
    marcel_fprintf(stderr, "marcel_memory_split fails %d <%m>\n", errno, errno);
  }
  else {
    err=0;
    for(i=0 ; i<10 ; i++) {
      if (newptrs[i] != ptr + i*(size/10)) err=1;
    }
    if (!err) {
      marcel_fprintf(stderr, "Success when splitting memory area\n");
    }
    else {
      marcel_fprintf(stderr, "Error when splitting memory area\n");
      for(i=0 ; i<10 ; i++) marcel_fprintf(stderr, "New ptr[%d] = %p\n", i, newptrs[i]);
    }
    attach(ptr, size);
    for(i=1 ; i<10 ; i++) marcel_memory_free(&memory_manager, newptrs[i]);
  }
  free(newptrs);
}

static void attach(void *ptr, size_t size) {
  marcel_t self;
  int err, i, node;
  long *stats;

  self = marcel_self();
  err = marcel_memory_task_attach(&memory_manager, ptr, size, self, &node);
  if (err < 0) {
    perror("marcel_memory_task_attach unexpectedly failed");
  }

  stats = (long *) (ma_task_stats_get (self, ma_stats_memnode_offset));
  err = 0;
  for(i=0 ; i<(marcel_nbnodes-1); i++) if (stats[i] != 0) err=1;
  if (!err) {
    if (stats[marcel_nbnodes-1] != size/10) err=1;
  }
  if (!err) {
    marcel_fprintf(stderr, "Success when attaching memory area\n");
  }
  else {
    marcel_fprintf(stderr, "Error when attaching memory area\n");
    for(i=0 ; i<marcel_nbnodes; i++)
      marcel_fprintf(stderr, "Stat for node #%d = %ld\n", i, stats[i]);
  }

  err = marcel_memory_task_unattach(&memory_manager, ptr, self);
  if (err < 0) {
    perror("marcel_memory_task_unattach unexpectedly failed");
  }
}

#else
int marcel_main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
