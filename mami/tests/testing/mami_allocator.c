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

#if defined(MAMI_ENABLED)

#warning a finir ....
typedef struct mami_record_s mami_record_t;
struct mami_record_s {
  float real;
  float imaginary;
};

static mami_manager_t *memory_manager;
static void *mami_record_malloc(void *arg);
static void mami_record_free(void *obj, void *foo TBX_UNUSED);

int main(int argc, char * argv[]) {
  ma_allocator_t *mami_records;
  mami_record_t *record;

  marcel_init(&argc,argv);
  mami_init(&memory_manager);

  mami_records = ma_new_obj_allocator(0,
                                      mami_record_malloc, (void *) sizeof(mami_record_t),
                                      mami_record_free, NULL, POLICY_HIERARCHICAL_MEMORY, 0);

  record = ma_obj_alloc(mami_records);
  record->real = 12.0;
  record->imaginary = 13.0;
  ma_obj_free(mami_records, record);

  ma_obj_allocator_fini(mami_records);
  mami_exit(&memory_manager);

  // Finish marcel
  marcel_end();
  return 0;
}

void *mami_record_malloc(void *arg) {
  size_t size = (size_t) (intptr_t) arg;

  marcel_printf("Allocating object\n");
  return mami_malloc(memory_manager, size, MAMI_MEMBIND_POLICY_DEFAULT, 0);
}

void mami_record_free(void *obj, void *foo TBX_UNUSED) {
  marcel_printf("Freeing object\n");
  mami_free(memory_manager, obj);
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
