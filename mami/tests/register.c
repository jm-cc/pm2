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

#include "mami.h"
#include "helper.h"

int main(int argc, char * argv[]) {
  int err;
  void *ptr;
  mami_manager_t *memory_manager;

  mami_init(&memory_manager, argc, argv);

  ptr = memalign(getpagesize(), 50*getpagesize());
  err = mami_register(memory_manager, ptr, 50000);
  MAMI_CHECK_RETURN_VALUE(err, "mami_register(1)");

  err = mami_register(memory_manager, ptr, 50000);
  MAMI_CHECK_RETURN_VALUE_IS(err, EINVAL, "mami_register(2)");

  err = mami_unregister(memory_manager, ptr);
  MAMI_CHECK_RETURN_VALUE(err, "mami_unregister(1)");

  err = mami_unregister(memory_manager, ptr);
  MAMI_CHECK_RETURN_VALUE_IS(err, EINVAL, "mami_unregister(2)");

  free(ptr);
  mami_exit(&memory_manager);
  mprintf(stdout, "Success\n");
  return 0;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
  return MAMI_TEST_SKIPPED;
}
#endif
