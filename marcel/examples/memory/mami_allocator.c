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

#warning a finir ....
typedef struct mami_record_s mami_record_t;
struct mami_record_s {
  float real;
  float imaginary;
};

static marcel_memory_manager_t memory_manager;
static void *mami_record_malloc(void *arg);
static void mami_record_free(void *obj, void *foo TBX_UNUSED);

int marcel_main(int argc, char * argv[]) {
  ma_allocator_t *mami_records;
  mami_record_t *record;

  marcel_init(&argc,argv);
  marcel_memory_init(&memory_manager);

  mami_records = ma_new_obj_allocator(0,
                                      mami_record_malloc, (void *) sizeof(mami_record_t),
                                      mami_record_free, NULL, POLICY_HIERARCHICAL_MEMORY, 0);

  record = ma_obj_alloc(mami_records);
  record->real = 12.0;
  record->imaginary = 13.0;
  ma_obj_free(mami_records, record);

  marcel_memory_exit(&memory_manager);

  // Finish marcel
  marcel_end();
  return 0;
}

void *mami_record_malloc(void *arg) {
  size_t size = (size_t) (intptr_t) arg;

  marcel_printf("Allocating object\n");
  return marcel_memory_malloc(&memory_manager, size, MARCEL_MEMORY_MEMBIND_POLICY_DEFAULT, 0);
}

void mami_record_free(void *obj, void *foo TBX_UNUSED) {
  marcel_printf("Freeing object\n");
  marcel_memory_free(&memory_manager, obj);
}

#else
int marcel_main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MAMI to be enabled\n");
}
#endif
