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
#include "helper.h"

#if defined(MARCEL)

int main(int argc, char * argv[]) {
  int err, node;
  void *ptr=NULL;
  void *ptr2;
  marcel_t self;
  mami_manager_t *memory_manager;

  marcel_init(&argc,argv);
  mami_init(&memory_manager, argc, argv);
  self = marcel_self();

  err = mami_task_attach(memory_manager, ptr, 100, self, &node);
  MAMI_CHECK_RETURN_VALUE_IS(err, EINVAL, "mami_task_attach");

  ptr = mami_malloc(memory_manager, 1000, MAMI_MEMBIND_POLICY_SPECIFIC_NODE, 0);
  MAMI_CHECK_MALLOC(ptr);
  ptr2 = mami_malloc(memory_manager, 1000, MAMI_MEMBIND_POLICY_SPECIFIC_NODE, 0);
  MAMI_CHECK_MALLOC(ptr2);

  err = mami_task_attach(memory_manager, ptr, 1000, self, &node);
  MAMI_CHECK_RETURN_VALUE(err, "mami_task_attach");
  err = mami_task_unattach(memory_manager, ptr, self);
  MAMI_CHECK_RETURN_VALUE(err, "mami_task_unattach");

  err = mami_task_attach(memory_manager, ptr, 1000, self, &node);
  MAMI_CHECK_RETURN_VALUE(err, "mami_task_attach");
  err = mami_task_attach(memory_manager, ptr2, 1000, self, &node);
  MAMI_CHECK_RETURN_VALUE(err, "mami_task_attach");
  err = mami_task_unattach_all(memory_manager, self);
  MAMI_CHECK_RETURN_VALUE(err, "mami_task_unattach_all");

  mami_free(memory_manager, ptr);
  mami_free(memory_manager, ptr2);
  mami_exit(&memory_manager);

  // Finish marcel
  marcel_end();
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
