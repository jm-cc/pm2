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

#include <stdio.h>

#if defined(MM_MAMI_ENABLED)

#include <mm_mami.h>
#include <mm_mami_private.h>
#include "helper.h"

int check_stats(mami_manager_t *memory_manager, long *expected_stats, int print) {
  int err=0;
  int node;
  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    long stat = ((long *) marcel_task_stats_get(marcel_self(), MEMNODE))[node];
    if (stat != expected_stats[node]) {
      fprintf(stderr, "Warning: Stat[%d]=%ld should be %ld\n", node, stat, expected_stats[node]);
      err=1;
    }
  }
  if (print) {
    for(node=0 ; node<memory_manager->nb_nodes ; node++) {
      fprintf(stderr, "stats=%ld\n", ((long *) marcel_task_stats_get (marcel_self(), MEMNODE))[node]);
    }
  }
  return err;
}

int attach_distribute_stats(mami_manager_t *memory_manager, void *ptr, size_t size, int print) {
  int err, node, i, *nodes;
  long *expected_stats;

  expected_stats = malloc(memory_manager->nb_nodes * sizeof(long));
  for(i=0 ; i<memory_manager->nb_nodes ; i++) expected_stats[i] = 0;
  err = check_stats(memory_manager, expected_stats, print);
  if (err) return err;

  fprintf(stderr, "... Attaching current thread to memory area\n");
  err = mami_task_attach(memory_manager, ptr, size, th_mami_self(), &node);
  MAMI_CHECK_RETURN_VALUE(err, "mami_task_attach");

  if (node >= 0 && node < memory_manager->nb_nodes) expected_stats[node] += size;
  err = check_stats(memory_manager, expected_stats, print);
  if (err) return err;

  memset(ptr, 0, size);

  fprintf(stderr, "... Moving memory area to node #0\n");
  err = mami_migrate_on_node(memory_manager, ptr, 0);
  MAMI_CHECK_RETURN_VALUE(err, "mami_migrate_on_node");

  expected_stats[node] -= size;
  expected_stats[0] += size;
  err = check_stats(memory_manager, expected_stats, print);
  if (err) return err;

  fprintf(stderr, "... Distributing pages with a round-robin policy\n");
  nodes = malloc(sizeof(int) * memory_manager->nb_nodes);
  for(i=0 ; i<memory_manager->nb_nodes ; i++) {
    nodes[i] = i;
    expected_stats[i] = 4*memory_manager->normal_page_size;
  }
  expected_stats[0] += memory_manager->normal_page_size;
  err = mami_distribute(memory_manager, ptr, nodes, memory_manager->nb_nodes);
  MAMI_CHECK_RETURN_VALUE(err, "mami_distribute");

  err = mami_locate(memory_manager, ptr, size, &node);
  MAMI_CHECK_RETURN_VALUE(err, "mami_locate");
  if (node != MAMI_MULTIPLE_LOCATION_NODE) {
    fprintf(stderr, "Node is NOT <MAMI_MULTIPLE_LOCATION_NODE> as expected but %d\n", node);
    return 1;
  }

  err = check_stats(memory_manager, expected_stats, print);
  if (err) return err;

  fprintf(stderr, "... Distributing pages evenly on nodes #0 and #1\n");
  for(i=0 ; i<memory_manager->nb_nodes ; i++) nodes[i] = (i%2);
  err = mami_distribute(memory_manager, ptr, nodes, memory_manager->nb_nodes);
  MAMI_CHECK_RETURN_VALUE(err, "mami_distribute");

  err = mami_locate(memory_manager, ptr, size, &node);
  MAMI_CHECK_RETURN_VALUE(err, "mami_locate");
  if (node != MAMI_MULTIPLE_LOCATION_NODE) {
    fprintf(stderr, "Node is NOT <MAMI_MULTIPLE_LOCATION_NODE> as expected but %d\n", node);
    return 1;
  }

  memset(expected_stats, 0, memory_manager->nb_nodes*sizeof(long));
  expected_stats[0] = size / 2;
  expected_stats[1] = size / 2;
  if ((size / memory_manager->normal_page_size) % 2) {
    expected_stats[0] += (memory_manager->normal_page_size/2);
    expected_stats[1] -= (memory_manager->normal_page_size/2);
  }
  err = check_stats(memory_manager, expected_stats, print);
  if (err) return err;

  fprintf(stderr, "... Unattaching current thread from memory area\n");
  err = mami_task_unattach(memory_manager, ptr, th_mami_self());
  MAMI_CHECK_RETURN_VALUE(err, "mami_task_unattach");

  memset(expected_stats, 0, memory_manager->nb_nodes*sizeof(long));
  err = check_stats(memory_manager, expected_stats, print);
  if (err) return err;

  fprintf(stderr, "... Re-attaching current thread to memory area\n");
  err = mami_task_attach(memory_manager, ptr, size, th_mami_self(), &node);
  MAMI_CHECK_RETURN_VALUE(err, "mami_task_attach");

  memset(expected_stats, 0, memory_manager->nb_nodes*sizeof(long));
  expected_stats[0] = size / 2;
  expected_stats[1] = size / 2;
  if ((size / memory_manager->normal_page_size) % 2) {
    expected_stats[0] += (memory_manager->normal_page_size/2);
    expected_stats[1] -= (memory_manager->normal_page_size/2);
  }
  err = check_stats(memory_manager, expected_stats, print);
  if (err) return err;

  fprintf(stderr, "... Gathering memory area to node #2\n");
  err = mami_migrate_on_node(memory_manager, ptr, 2);
  MAMI_CHECK_RETURN_VALUE(err, "mami_migrate_on_node");

  memset(expected_stats, 0, memory_manager->nb_nodes*sizeof(long));
  expected_stats[2] = size;
  err = check_stats(memory_manager, expected_stats, print);
  if (err) return err;

  fprintf(stderr, "... Unattaching current thread from memory area\n");
  err = mami_task_unattach(memory_manager, ptr, th_mami_self());
  MAMI_CHECK_RETURN_VALUE(err, "mami_task_unattach");

  memset(expected_stats, 0, memory_manager->nb_nodes*sizeof(long));
  err = check_stats(memory_manager, expected_stats, print);
  if (err) return err;

  free(nodes);
  return 0;
}

int main(int argc, char *argv[]) {
  int err=0, i, node, print;
  void *ptr;
  mami_manager_t *memory_manager;
  size_t size;

  common_init(&argc, argv, NULL);
  mami_init(&memory_manager, argc, argv);

  if (memory_manager->nb_nodes < 3) {
    printf("This application needs at least three NUMA nodes.\n");
  }
  else {
    print=1;
    if (argc > 1 && !strcmp(argv[1], "--no-print")) print=0;

    size = ((memory_manager->nb_nodes * 4) + 1) * memory_manager->normal_page_size;

    ptr = mami_malloc(memory_manager, size, MAMI_MEMBIND_POLICY_FIRST_TOUCH, 1);
    MAMI_CHECK_MALLOC(ptr);

    fprintf(stderr, "... Allocating memory on first touch node\n");
    err = mami_locate(memory_manager, ptr, size, &node);
    MAMI_CHECK_RETURN_VALUE(err, "mami_locate");
    if (node != MAMI_FIRST_TOUCH_NODE) {
      fprintf(stderr, "Node is NOT <MAMI_FIRST_TOUCH_NODE> as expected but %d\n", node);
      return 1;
    }
    err = attach_distribute_stats(memory_manager, ptr, size, print);
    MAMI_CHECK_RETURN_VALUE(err, "attach_distribute_stats");
    mami_free(memory_manager, ptr);

    ptr = mami_malloc(memory_manager, size, MAMI_MEMBIND_POLICY_SPECIFIC_NODE, 1);
    MAMI_CHECK_MALLOC(ptr);

    fprintf(stderr, "... Allocating memory on node #1\n");
    err = mami_locate(memory_manager, ptr, size, &node);
    MAMI_CHECK_RETURN_VALUE(err, "mami_locate");
    if (node != 1) {
      fprintf(stderr, "Node is NOT 1 as expected but %d\n", node);
      return 1;
    }
    err = attach_distribute_stats(memory_manager, ptr, size, print);
    MAMI_CHECK_RETURN_VALUE(err, "attach_distribute_stats");
    mami_free(memory_manager, ptr);
  }

  mami_exit(&memory_manager);
  common_exit(NULL);
  return 0;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
