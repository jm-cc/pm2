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

void scatter(void *ptr);
void attach(void *ptr);

int marcel_main(int argc, char * argv[]) {
  int err;
  void *ptr;

  marcel_init(&argc,argv);
  marcel_memory_init(&memory_manager);

  marcel_printf("Scattering memory area allocated by MAMI\n");
  ptr = marcel_memory_malloc(&memory_manager, 10*getpagesize());
  scatter(ptr);
  marcel_memory_free(&memory_manager, ptr);

  marcel_printf("\nScattering unknown memory area\n");
  ptr = malloc(50*getpagesize());
  scatter(ptr);

  err = marcel_memory_register(&memory_manager, ptr, 50*getpagesize(), 0);
  if (err < 0) {
    perror("marcel_memory_register");
  }

  marcel_printf("\nScattering memory area not allocated by MAMI\n");
  scatter(ptr);
  err = marcel_memory_unregister(&memory_manager, ptr);
  if (err < 0) {
    perror("marcel_memory_unregister");
  }
  free(ptr);

  marcel_memory_exit(&memory_manager);

  // Finish marcel
  marcel_end();
  return 0;
}

void scatter(void *ptr) {
  int err, i;
  void **newptrs;

  attach(ptr);
  err = marcel_memory_scatter(&memory_manager, ptr, 10, &newptrs);
  if (err < 0) {
    perror("marcel_memory_scatter");
  }
  else {
    for(i=0 ; i<10 ; i++) marcel_printf("New ptr[%d] = %p\n", i, newptrs[i]);
    attach(ptr);
    for(i=1 ; i<10 ; i++) marcel_memory_free(&memory_manager, newptrs[i]);
  }
}

void attach(void *ptr) {
  marcel_t self;
  int err, i;
  long *stats;

  self = marcel_self();
  err = marcel_memory_attach(&memory_manager, ptr, &self);
  if (err < 0) {
    perror("marcel_memory_attach");
  }

  stats = (long *) (ma_task_stats_get (self, ma_stats_memnode_offset));
  for(i=0 ; i<marcel_nbnodes; i++) marcel_printf("Stat for node #%d = %ld\n", i, stats[i]);

  err = marcel_memory_unattach(&memory_manager, ptr, &self);
  if (err < 0) {
    perror("marcel_memory_unattach");
  }
}

#else
int marcel_main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MAMI to be enabled\n");
}
#endif
