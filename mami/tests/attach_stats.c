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
#include "mami.h"
#include "helper.h"

#if defined(MAMI_ENABLED)

#if defined(MARCEL)

static mami_manager_t *memory_manager;
void stats(void *ptr);

int main(int argc, char * argv[]) {
  int *ptr;

  marcel_init(&argc,argv);
  mami_init(&memory_manager, argc, argv);

  ptr = mami_malloc(memory_manager, 100, MAMI_MEMBIND_POLICY_SPECIFIC_NODE, 0);
  stats(ptr);
  mami_free(memory_manager, ptr);

  ptr = mami_malloc(memory_manager, 100, MAMI_MEMBIND_POLICY_FIRST_TOUCH, 0);
  stats(ptr);
  mami_free(memory_manager, ptr);

  mami_exit(&memory_manager);

  // Finish marcel
  marcel_end();
  return 0;
}

void stats(void *ptr) {
  int err, node;

  mprintf(stderr, "[before attach]  stats=%ld\n", ((long *) marcel_task_stats_get (marcel_self(), MEMNODE))[0]);

  err = mami_task_attach(memory_manager, ptr, 100, marcel_self(), &node);
  if (err < 0) perror("mami_task_attach unexpectedly failed");

  mprintf(stderr, "[after attach]   stats=%ld\n", ((long *) marcel_task_stats_get (marcel_self(), MEMNODE))[0]);

  err = mami_task_unattach(memory_manager, ptr, marcel_self());
  if (err < 0) perror("mami_task_unattach unexpectedly failed");

  mprintf(stderr, "[after unattach] stats=%ld\n", ((long *) marcel_task_stats_get (marcel_self(), MEMNODE))[0]);
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs marcel to be enabled\n");
  return MAMI_TEST_SKIPPED;
}
#endif

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
  return MAMI_TEST_SKIPPED;
}
#endif
