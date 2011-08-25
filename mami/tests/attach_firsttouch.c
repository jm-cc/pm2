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

#include <mami.h>
#include <mami_private.h>
#include "helper.h"

#if defined(MARCEL)

mami_manager_t *memory_manager;
any_t firsttouch(any_t arg);

int main(int argc, char * argv[]) {
  int err;
  marcel_t thread;
  marcel_attr_t attr;
  any_t status;

  marcel_init(&argc,argv);
  marcel_attr_init(&attr);
  mami_init(&memory_manager, argc, argv);

  err = mami_membind(memory_manager, MAMI_MEMBIND_POLICY_FIRST_TOUCH, 0);
  MAMI_CHECK_RETURN_VALUE(err, "mami_membind");

  marcel_attr_settopo_level_on_node(&attr, marcel_nbnodes-1);
  marcel_create(&thread, &attr, firsttouch, NULL);
  marcel_join(thread, &status);
  if (status == ALL_IS_NOT_OK) return 1;

  // Finish marcel
  mami_exit(&memory_manager);
  marcel_end();
  return 0;
}

any_t firsttouch(any_t arg) {
  int *ptr;
  int err, node;

  ptr = mami_malloc(memory_manager, 100, MAMI_MEMBIND_POLICY_DEFAULT, 0);
  MAMI_CHECK_MALLOC_THREAD(ptr);
  mami_free(memory_manager, ptr);

  ptr = mami_malloc(memory_manager, 100, MAMI_MEMBIND_POLICY_DEFAULT, 0);
  MAMI_CHECK_MALLOC_THREAD(ptr);
  err = mami_locate(memory_manager, ptr, 100, &node);
  MAMI_CHECK_RETURN_VALUE_THREAD(err, "mami_locate");

  if (node != MAMI_FIRST_TOUCH_NODE) {
    mprintf(stderr, "Address is NOT located on node %d but on node %d\n", MAMI_FIRST_TOUCH_NODE, node);
    return ALL_IS_NOT_OK;
  }

  ptr[0] = 42;

  err = mami_task_attach(memory_manager, ptr, 100, marcel_self(), &node);
  MAMI_CHECK_RETURN_VALUE_THREAD(err, "mami_task_attach");

  err = mami_locate(memory_manager, ptr, 100, &node);
  MAMI_CHECK_RETURN_VALUE_THREAD(err, "mami_locate");

  if (node != marcel_nbnodes-1) {
    mprintf(stderr, "Address is NOT located on node %d but on node %d\n", marcel_nbnodes-1, node);
    return ALL_IS_NOT_OK;
  }

  err = mami_task_unattach(memory_manager, ptr, marcel_self());
  MAMI_CHECK_RETURN_VALUE_THREAD(err, "mami_task_unattach");
  mami_free(memory_manager, ptr);
  return ALL_IS_OK;
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

