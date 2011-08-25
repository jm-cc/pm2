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
#include <malloc.h>

#if defined(MAMI_ENABLED)

#include <mami.h>
#include <mami_private.h>
#include "helper.h"

#if defined(MARCEL)

static mami_manager_t *memory_manager;
static int do_first_touch(void *buffer, size_t size, int node1, int node2);
static any_t first_touch(any_t arg);

int main(int argc, char * argv[]) {
  marcel_t thread;
  marcel_attr_t attr;
  any_t status;

  mami_init(&memory_manager, argc, argv);

  if (memory_manager->nb_nodes < 2) {
    fprintf(stderr,"This application needs at least two NUMA nodes.\n");
    mami_exit(&memory_manager);
    return MAMI_TEST_SKIPPED;
  }

  marcel_init(&argc,argv);
  marcel_attr_init(&attr);
  mami_unset_kernel_migration(memory_manager);

  marcel_attr_settopo_level_on_node(&attr, marcel_nbnodes-1);
  marcel_create(&thread, &attr, first_touch, NULL);
  marcel_join(thread, &status);
  if (status == ALL_IS_NOT_OK) return 1;

  // Finish marcel
  mami_exit(&memory_manager);
  marcel_end();
  return 0;
}

static any_t first_touch(any_t arg) {
  void *buffer;
  size_t size;
  int err;

  size = 10000*sizeof(int);

  // Case with user-allocated memory
  mprintf(stderr, "... Testing user-allocated memory\n");
  posix_memalign(&buffer, getpagesize(), size);
  err = do_first_touch(buffer, size, -100, marcel_nbnodes-1);
  if (err) return ALL_IS_NOT_OK;
  err = mami_unregister(memory_manager, buffer);
  MAMI_CHECK_RETURN_VALUE_THREAD(err, "mami_unregister");
  free(buffer);

  // Case with mami-allocated memory
  err = mami_membind(memory_manager, MAMI_MEMBIND_POLICY_FIRST_TOUCH, 0);
  MAMI_CHECK_RETURN_VALUE_THREAD(err, "mami_membind");
  mprintf(stderr, "... Testing MaMI-allocated memory\n");
  buffer = mami_malloc(memory_manager, size, MAMI_MEMBIND_POLICY_DEFAULT, 0);
  MAMI_CHECK_MALLOC_THREAD(buffer);
  err = do_first_touch(buffer, size, MAMI_FIRST_TOUCH_NODE, marcel_nbnodes-1);
  if (err) return ALL_IS_NOT_OK;
  mami_free(memory_manager, buffer);

  return ALL_IS_OK;
}

static int do_first_touch(void *buffer, size_t size, int node1, int node2) {
  int err;
  int node;

  err = mami_task_attach(memory_manager, buffer, size, marcel_self(), &node);
  MAMI_CHECK_RETURN_VALUE(err, "mami_task_attach");

  err = mami_locate(memory_manager, buffer, size, &node);
  MAMI_CHECK_RETURN_VALUE(err, "mami_locate");

  if (!((node1 < 0 && node < 0) || (node1 >= 0 && node == node1))) {
    mprintf(stderr, "Address is NOT located on node %d but on node %d\n", node1, node);
    return 1;
  }

  err = mami_migrate_on_next_touch(memory_manager, buffer);
  MAMI_CHECK_RETURN_VALUE(err, "mami_migrate_on_next_touch");

  memset(buffer, 0, size);

  err = mami_locate(memory_manager, buffer, size, &node);
  MAMI_CHECK_RETURN_VALUE(err, "mami_locate");

  if (node != node2) {
    mprintf(stderr, "Address is NOT located on node %d but on node %d\n", node1, node);
    return 1;
  }

  err = mami_check_pages_location(memory_manager, buffer, size, node);
  MAMI_CHECK_RETURN_VALUE(err, "mami_check_pages_location");

  err = mami_task_unattach(memory_manager, buffer, marcel_self());
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
