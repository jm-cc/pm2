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

static marcel_memory_manager_t memory_manager;

static void split(void *ptr, size_t size);
static void attach(void *ptr, size_t size);

int marcel_main(int argc, char * argv[]) {
  int err;
  void *ptr;

  marcel_init(&argc,argv);
  marcel_memory_init(&memory_manager);

  marcel_printf("Spliting memory area allocated by MAMI\n");
  ptr = marcel_memory_malloc(&memory_manager, 10*getpagesize());
  split(ptr, 10*getpagesize());
  marcel_memory_free(&memory_manager, ptr);

  marcel_printf("\nSpliting unknown memory area\n");
  ptr = malloc(50*getpagesize());
  split(ptr, 50*getpagesize());

  err = marcel_memory_register(&memory_manager, ptr, 50*getpagesize());
  if (err < 0) {
    perror("marcel_memory_register unexpectedly failed");
  }

  marcel_printf("\nSpliting memory area not allocated by MAMI\n");
  split(ptr, 50*getpagesize());
  err = marcel_memory_unregister(&memory_manager, ptr);
  if (err < 0) {
    perror("marcel_memory_unregister unexpectedly failed");
  }
  free(ptr);

  marcel_memory_exit(&memory_manager);

  // Finish marcel
  marcel_end();
  return 0;
}

static void split(void *ptr, size_t size) {
  int err, i;
  void **newptrs;

  attach(ptr, size);
  newptrs = malloc(10 * sizeof(void **));
  err = marcel_memory_split(&memory_manager, ptr, 10, newptrs);
  if (err < 0) {
    perror("marcel_memory_split unexpectedly failed");
  }
  else {
    for(i=0 ; i<10 ; i++) marcel_printf("New ptr[%d] = %p\n", i, newptrs[i]);
    attach(ptr, size);
    for(i=1 ; i<10 ; i++) marcel_memory_free(&memory_manager, newptrs[i]);
  }
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
  for(i=0 ; i<marcel_nbnodes; i++) marcel_printf("Stat for node #%d = %ld\n", i, stats[i]);

  err = marcel_memory_task_unattach(&memory_manager, ptr, self);
  if (err < 0) {
    perror("marcel_memory_task_unattach unexpectedly failed");
  }
}

#else
int marcel_main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MAMI to be enabled\n");
}
#endif
