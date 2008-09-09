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

#ifdef MARCEL_MAMI_ENABLED

#include "marcel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <numa.h>
#include <numaif.h>
#include <sys/mman.h>
#include <asm/unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

#define LOOPS 1000

void ma_memory_sampling_check_location(void **pageaddrs, int pages, int node) {
  int *pagenodes;
  int i;
  int err;

  pagenodes = malloc(pages * sizeof(int));
  err = move_pages(0, pages, pageaddrs, NULL, pagenodes, 0);
  if (err < 0) {
    perror("move_pages (check_location)");
    exit(-1);
  }

  for(i=0; i<pages; i++) {
    if (pagenodes[i] != node) {
      printf("  page #%d is not located on node #%d\n", i, node);
      exit(-1);
    }
  }
  free(pagenodes);
}

void ma_memory_sampling_migrate(void **pageaddrs, int pages, int *nodes, int *status) {
  int err;

  //printf("binding on numa node #%d\n", nodes[0]);

  err = move_pages(0, pages, pageaddrs, nodes, status, MPOL_MF_MOVE);

  if (err < 0) {
    perror("move_pages (set_bind)");
    exit(-1);
  }
}

void ma_memory_sampling(unsigned long source, unsigned long dest, int pages, unsigned long maxnode, unsigned long pagesize, FILE *f) {
  void *buffer;
  void **pageaddrs;
  unsigned long nodemask;
  int i, err;
  int *sources, *dests, *status;
  struct timeval tv1, tv2;
  unsigned long us, ns;

  // Allocate the pages
  buffer = mmap(NULL, pages * pagesize, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
  if (buffer < 0) {
    perror("mmap");
    exit(-1);
  }

  // Set the page addresses
  pageaddrs = malloc(pages * sizeof(void *));
  for(i=0; i<pages; i++)
    pageaddrs[i] = buffer + i*pagesize;

  // Set the memory policy on node source
  nodemask = (1<<source);
  err = set_mempolicy(MPOL_BIND, &nodemask, maxnode+1);
  if (err < 0) {
    perror("set_mempolicy");
    exit(-1);
  }

  // Fill in the whole memory
  memset(buffer, 0, pages*pagesize);

  // Check the location of the pages
  ma_memory_sampling_check_location(pageaddrs, pages, source);

  // Move all the pages on node dest
  status = malloc(pages * sizeof(int));
  sources = malloc(pages * sizeof(int));
  dests = malloc(pages * sizeof(int));
  for(i=0; i<pages ; i++) dests[i] = dest;
  for(i=0; i<pages ; i++) sources[i] = source;

  gettimeofday(&tv1, NULL);
  for(i=0 ; i<LOOPS ; i++) {
    ma_memory_sampling_migrate(pageaddrs, pages, dests, status);
    ma_memory_sampling_migrate(pageaddrs, pages, sources, status);
  }
  ma_memory_sampling_migrate(pageaddrs, pages, dests, status);
  gettimeofday(&tv2, NULL);

  // Check the location of the pages
  for(i=0; i<pages; i++) {
    if (status[i] != dest) {
      printf("  page #%d has not been moved on node #%ld\n", i, dest);
      exit(-1);
    }
  }

  free(pageaddrs);
  free(status);
  free(sources);
  free(dests);
  munmap(buffer, pages * pagesize);

  us = (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);
  ns = us * 1000;
  ns /= (LOOPS * 2);
  fprintf(f, "%ld\t%ld\t%d\t%ld\t%ld\n", source, dest, pages, pagesize*pages, ns);
  printf("%ld\t%ld\t%d\t%ld\t%ld\n", source, dest, pages, pagesize*pages, ns);
}

void ma_memory_sampling_get_filename(char *filename) {
  char directory[1024];
  char hostname[1024];
  int rc = 0;
  const char* pm2_conf_dir;

  if (gethostname(hostname, 1023) < 0) {
    perror("gethostname");
    exit(1);
  }
  pm2_conf_dir = getenv("PM2_CONF_DIR");
  if (pm2_conf_dir) {
    rc = snprintf(directory, 1024, "%s/marcel", pm2_conf_dir);
  }
  else {
    const char* home = getenv("PM2_HOME");
    if (!home) {
      home = getenv("HOME");
    }
    assert(home != NULL);
    rc = snprintf(filename, 1024, "%s/.pm2/marcel", home);
  }
  assert(rc < 1024);
  snprintf(filename, 1024, "%s/sampling_for_memory_migration_%s.txt", directory, hostname);
  assert(rc < 1024);
  //printf("File %s\n", filename);

  mkdir(directory, 0755);
}

void ma_memory_insert_cost(p_tbx_slist_t migration_costs, unsigned long maxsize, unsigned long cost) {
  marcel_memory_migration_cost_t *migration_cost;

  migration_cost = malloc(sizeof(marcel_memory_migration_cost_t));
  migration_cost->nbpages_max = maxsize;
  migration_cost->cost = cost;

  tbx_slist_push(migration_costs, migration_cost);
}


void ma_memory_load_sampling_of_migration_cost(marcel_memory_manager_t *memory_manager) {
  char filename[1024];
  FILE *out;
  char line[1024];
  unsigned long source;
  unsigned long dest;
  unsigned long maxsize;
  unsigned long cost;
  int pages;

  ma_memory_sampling_get_filename(filename);
  out = fopen(filename, "r");
  if (!out) {
    printf("Sampling information is not available\n");
    return;
  }
  mdebug_heap("Reading file %s\n", filename);
  fgets(line, 1024, out);
  while (!feof(out)) {
    fscanf(out, "%ld\t%ld\t%d\t%ld\t%ld\n", &source, &dest, &pages, &maxsize, &cost);

    ma_memory_insert_cost(memory_manager->migration_costs[source][dest], maxsize, cost);
    ma_memory_insert_cost(memory_manager->migration_costs[dest][source], maxsize, cost);
  }
  fclose(out);
}

void marcel_memory_sampling_of_migration_cost() {
  unsigned long pagesize;
  unsigned long maxnode;
  unsigned long source, dest;
  char filename[1024];
  FILE *out;
  //int nbpages[] = { 200, 400, 500, 1000, 1500, 2000, 3000, 4000, 5000, 10000, 15000, 20000, 25000, -1 };

  pagesize = getpagesize();
  maxnode = numa_max_node()+1;

  ma_memory_sampling_get_filename(filename);
  out = fopen(filename, "w");
  fprintf(out, "Source\tDest\tPages\tSize\tMigration_Time\n");
  printf("Source\tDest\tPages\tSize\tMigration_Time\n");

  for(source=0; source<maxnode ; source++) {
    for(dest=0; dest<maxnode ; dest++) {
      if (source >= dest) continue;

      {
        int pages;
        for(pages=1; pages<=100 ; pages++) {
          ma_memory_sampling(source, dest, pages, maxnode, pagesize, out);
        }
        fflush(out);
      }
      {
        int pages;
        for(pages=200; pages<5000 ; pages+=100) {
          ma_memory_sampling(source, dest, pages, maxnode, pagesize, out);
        }
        fflush(out);
      }
      {
        int pages;
        for(pages=5000; pages<25000 ; pages+=1000) {
          ma_memory_sampling(source, dest, pages, maxnode, pagesize, out);
        }
        fflush(out);
      }
    }
  }
  fclose(out);
}

#endif /* MAMI_ENABLED */
