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
#include <tbx_timing.h>
#include "mami.h"

#if defined(MAMI_ENABLED)

int main(int argc, char * argv[]) {
  mami_manager_t *memory_manager;
  void *ptr;
  int node;
  tbx_tick_t t1, t2;
  unsigned long temps;
  size_t size=1000;

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
  printf("size=%d time=%ld.%03ldms\n", (int)size, temps/1000, temps%1000);

  mami_exit(&memory_manager);
  return 0;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
