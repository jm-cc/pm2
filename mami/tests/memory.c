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

//#define PRINT
#define PAGES 2
mami_manager_t *memory_manager;
any_t memory(any_t arg);
any_t memory2(any_t arg);
int memory_bis(int argc, char * argv[]);

int main(int argc, char * argv[]) {
  th_mami_t threads[2];
  int args[2];
  any_t status;

  mami_init(&memory_manager, argc, argv);

  if (memory_manager->nb_nodes < 2) {
    fprintf(stderr,"This application needs at least two NUMA nodes.\n");
    mami_exit(&memory_manager);
    return MAMI_TEST_SKIPPED;
  }
  else {
    args[0] = 0;
    args[1] = 1;
    th_mami_create(&threads[0], NULL, memory, (any_t) &args[0]);
    th_mami_create(&threads[1], NULL, memory, (any_t) &args[1]);

    th_mami_join(threads[0], &status);
    if (status == ALL_IS_NOT_OK) return 1;
    th_mami_join(threads[1], &status);
    if (status == ALL_IS_NOT_OK) return 1;

    th_mami_create(&threads[1], NULL, memory2, NULL);
    th_mami_join(threads[1], &status);
    if (status == ALL_IS_NOT_OK) return 1;
  }

  mami_exit(&memory_manager);

#ifndef MARCEL
  int ret = memory_bis(argc, argv);
  if (ret) return ret;
#endif

  return 0;
}

any_t memory(any_t arg) {
  int *b, *c, *d, *e;
  char *buffer;
  int err, node, *id;

  id = (int *) arg;
  b = mami_malloc(memory_manager, 100*sizeof(int), MAMI_MEMBIND_POLICY_SPECIFIC_NODE, *id);
  MAMI_CHECK_MALLOC_THREAD(b);
  c = mami_malloc(memory_manager, 100*sizeof(int), MAMI_MEMBIND_POLICY_SPECIFIC_NODE, *id);
  MAMI_CHECK_MALLOC_THREAD(c);
  d = mami_malloc(memory_manager, 100*sizeof(int), MAMI_MEMBIND_POLICY_SPECIFIC_NODE, *id);
  MAMI_CHECK_MALLOC_THREAD(d);
  e = mami_malloc(memory_manager, 100*sizeof(int), MAMI_MEMBIND_POLICY_SPECIFIC_NODE, *id);
  MAMI_CHECK_MALLOC_THREAD(e);
  buffer = mami_calloc(memory_manager, 1, PAGES * getpagesize(), MAMI_MEMBIND_POLICY_SPECIFIC_NODE, *id);
  MAMI_CHECK_MALLOC_THREAD(buffer);

  err = mami_locate(memory_manager, &(buffer[0]), 0, &node);
  MAMI_CHECK_RETURN_VALUE_THREAD(err, "mami_locate");

  if (node == *id) {
    mprintf(stdout, "[%d] Address is located on the correct node %d\n", *id, node);
  }
  else {
    mprintf(stdout, "[%d] Address is NOT located on the correct node but on node %d\n", *id, node);
    return ALL_IS_NOT_OK;
  }

#ifdef PRINT
  mami_print(memory_manager);
#endif /* PRINT */

  mami_free(memory_manager, c);
  mami_free(memory_manager, b);
  mami_free(memory_manager, d);
  mami_free(memory_manager, e);
  mami_free(memory_manager, buffer);
  return ALL_IS_OK;
}

any_t memory2(any_t arg) {
  int i, node;
  void **buffers;
  void **buffers2;
  int maxnode = memory_manager->nb_nodes;

  buffers = malloc(maxnode * sizeof(void *));
  buffers2 = malloc(maxnode * sizeof(void *));
  for(i=1 ; i<=5 ; i++) {
    for(node=0 ; node<maxnode ; node++) {
      buffers[node] = mami_malloc(memory_manager, i*getpagesize(), MAMI_MEMBIND_POLICY_SPECIFIC_NODE, node);
      MAMI_CHECK_MALLOC_THREAD(buffers[node]);
    }
    for(node=0 ; node<maxnode ; node++)
      mami_free(memory_manager, buffers[node]);
  }
  for(node=0 ; node<maxnode ; node++) {
    buffers[node] = mami_malloc(memory_manager, getpagesize(), MAMI_MEMBIND_POLICY_SPECIFIC_NODE, node);
    MAMI_CHECK_MALLOC_THREAD(buffers[node]);
    buffers2[node] = mami_malloc(memory_manager, getpagesize(), MAMI_MEMBIND_POLICY_SPECIFIC_NODE, node);
    MAMI_CHECK_MALLOC_THREAD(buffers2[node]);
  }
  for(node=0 ; node<maxnode ; node++) {
    mami_free(memory_manager, buffers[node]);
    mami_free(memory_manager, buffers2[node]);
  }
  free(buffers);
  free(buffers2);
  return ALL_IS_OK;
}

int memory_bis(int argc, char * argv[]) {
  mami_manager_t *memory_manager;
  void *b[7];
  int i;

  mami_init(&memory_manager, argc, argv);
  memory_manager->initially_preallocated_pages = 2;
  b[0] = mami_malloc(memory_manager, 1*getpagesize(), MAMI_MEMBIND_POLICY_DEFAULT, 0);
  MAMI_CHECK_MALLOC(b[0]);
  b[1] = mami_malloc(memory_manager, 1*getpagesize(), MAMI_MEMBIND_POLICY_DEFAULT, 0);
  MAMI_CHECK_MALLOC(b[1]);
  b[2] = mami_malloc(memory_manager, 2*getpagesize(), MAMI_MEMBIND_POLICY_DEFAULT, 0);
  MAMI_CHECK_MALLOC(b[2]);
  b[3] = mami_malloc(memory_manager, 2*getpagesize(), MAMI_MEMBIND_POLICY_DEFAULT, 0);
  MAMI_CHECK_MALLOC(b[3]);
  b[4] = mami_malloc(memory_manager, 1*getpagesize(), MAMI_MEMBIND_POLICY_DEFAULT, 0);
  MAMI_CHECK_MALLOC(b[4]);
  b[5] = mami_malloc(memory_manager, 1*getpagesize(), MAMI_MEMBIND_POLICY_DEFAULT, 0);
  MAMI_CHECK_MALLOC(b[5]);
  b[6] = mami_malloc(memory_manager, 1*getpagesize(), MAMI_MEMBIND_POLICY_DEFAULT, 0);
  MAMI_CHECK_MALLOC(b[6]);

  for(i=0 ; i<7 ; i++) mami_free(memory_manager, b[i]);
  mami_exit(&memory_manager);

  return 0;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
  return MAMI_TEST_SKIPPED;
}
#endif
