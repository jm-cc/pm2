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

int main(int argc, char * argv[]) {
  mami_manager_t *memory_manager;
  void *ptr;
  int node;
  tbx_tick_t t1, t2;
  unsigned long temps;
  size_t size=1000;

  common_init(&argc, argv, NULL);
  mami_init(&memory_manager, argc, argv);

  if(argc > 1) {
    size = atoi(argv[1]);
  }

  TBX_GET_TICK(t1);
  ptr = mami_malloc(memory_manager, size, MAMI_MEMBIND_POLICY_SPECIFIC_NODE, 0);
  mami_locate(memory_manager, ptr, size, &node);
  if (node != 0) {
    printf("Memory allocated on current node\n");
  }
  mami_free(memory_manager, ptr);
  TBX_GET_TICK(t2);
  temps = TBX_TIMING_DELAY(t1, t2);
  printf("time = %ld.%03ldms\n", temps/1000, temps%1000);

  mami_exit(&memory_manager);
  common_exit(NULL);
  return 0;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
