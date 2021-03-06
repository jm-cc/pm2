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

#if defined(MAMI_ENABLED)

#include <errno.h>
#include <malloc.h>
#include <sys/mman.h>
#include <mami.h>
#include <mami_helper.h>
#include "helper.h"

#if defined(MARCEL)

static mami_manager_t *memory_manager;

static int split(void *ptr, size_t size, int err);
static int attach(void *ptr, size_t size);
any_t my_thread(any_t arg);

int main(int argc, char * argv[]) {
  marcel_t thread;
  marcel_attr_t attr;
  any_t status;

  marcel_init(&argc,argv);
  marcel_attr_init(&attr);
  mami_init(&memory_manager, argc, argv);

  marcel_attr_settopo_level_on_node(&attr, marcel_nbnodes-1);
  marcel_create(&thread, &attr, my_thread, NULL);
  marcel_join(thread, &status);
  if (status == ALL_IS_NOT_OK) return 1;

  mami_exit(&memory_manager);
  marcel_end();
  return 0;
}

any_t my_thread(any_t arg) {
  int err;
  void *ptr;
  size_t size;
  unsigned long nodemask;

  mprintf(stderr, "Spliting memory area allocated by MaMI\n");
  ptr = mami_malloc(memory_manager, 10*getpagesize(), MAMI_MEMBIND_POLICY_SPECIFIC_NODE, marcel_nbnodes-1);
  MAMI_CHECK_MALLOC_THREAD(ptr);

  err = split(ptr, 10*getpagesize(), 0);
  if (err) return ALL_IS_NOT_OK;
  mami_free(memory_manager, ptr);

  size = 50*getpagesize();
  nodemask = (1<<(marcel_nbnodes-1));

  mprintf(stderr, "\nSpliting unknown memory area\n");
  ptr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
  err = _mami_mbind(ptr, size, MPOL_BIND, &nodemask, marcel_nbnodes+1, MPOL_MF_MOVE);
  MAMI_CHECK_RETURN_VALUE_THREAD(err, "_mami_mbind");

  memset(ptr, 0, size);
  err = split(ptr, size, EINVAL);
  if (err) return ALL_IS_NOT_OK;

  err = mami_register(memory_manager, ptr, size);
  MAMI_CHECK_RETURN_VALUE_THREAD(err, "mami_register");

  mprintf(stderr, "\nSpliting memory area not allocated by MaMI\n");
  err = split(ptr, size, 0);
  if (err) return ALL_IS_NOT_OK;

  err = mami_unregister(memory_manager, ptr);
  MAMI_CHECK_RETURN_VALUE_THREAD(err, "mami_unregister");

  munmap(ptr, size);

  return ALL_IS_OK;
}

static int split(void *ptr, size_t size, int ret) {
  int err, i;
  void **newptrs;

  newptrs = malloc(10 * sizeof(void **));
  err = mami_split(memory_manager, ptr, 10, newptrs);
  if (ret) {
    MAMI_CHECK_RETURN_VALUE_IS(err, ret, "mami_split");
  }
  else {
    MAMI_CHECK_RETURN_VALUE(err, "mami_split");
  }

  if (err >= 0) {
    err=0;
    for(i=0 ; i<10 ; i++) {
      if (newptrs[i] != ptr + i*(size/10)) err=1;
    }
    if (err) {
      mprintf(stderr, "Error when splitting memory area\n");
      for(i=0 ; i<10 ; i++) mprintf(stderr, "New ptr[%d] = %p\n", i, newptrs[i]);
      return 1;
    }
    err = attach(ptr, size);
    MAMI_CHECK_RETURN_VALUE(err, "attach");

    for(i=1 ; i<10 ; i++) mami_free(memory_manager, newptrs[i]);
    free(newptrs);
  }
  return 0;
}

static int attach(void *ptr, size_t size) {
  marcel_t self;
  int err, i, node;
  long *stats;

  self = marcel_self();
  err = mami_task_attach(memory_manager, ptr, size, self, &node);
  MAMI_CHECK_RETURN_VALUE(err, "mami_task_attach");

  stats = (long *) (marcel_task_stats_get (self, MEMNODE));
  err = 0;
  for(i=0 ; i<(marcel_nbnodes-1); i++) if (stats[i] != 0) err=1;
  if (!err) {
    if (stats[marcel_nbnodes-1] != size/10) err=1;
  }
  if (err) {
    mprintf(stderr, "Error when attaching memory area\n");
    for(i=0 ; i<marcel_nbnodes; i++)
      mprintf(stderr, "Stat for node #%d = %ld\n", i, stats[i]);
    return 1;
  }

  err = mami_task_unattach(memory_manager, ptr, self);
  MAMI_CHECK_RETURN_VALUE(err, "mami_task_unattach");

  return 0;
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
