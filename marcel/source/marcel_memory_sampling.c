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

void ma_memory_sampling(unsigned long source, unsigned long dest, void *buffer, int pages,
                        void **pageaddrs, int *sources, int *dests, int *status,
                        unsigned long pagesize, FILE *f) {
  int i;
  struct timeval tv1, tv2;
  unsigned long us, ns, bandwidth;

  // Check the location of the pages
  ma_memory_sampling_check_location(pageaddrs, pages, source);

  // Migrate the pages back and forth between the nodes dest and source
  gettimeofday(&tv1, NULL);
  for(i=0 ; i<LOOPS ; i++) {
    ma_memory_sampling_migrate(pageaddrs, pages, dests, status);
    ma_memory_sampling_migrate(pageaddrs, pages, sources, status);
  }
  ma_memory_sampling_migrate(pageaddrs, pages, dests, status);
  gettimeofday(&tv2, NULL);

  // Check the location of the pages
  ma_memory_sampling_check_location(pageaddrs, pages, dest);

  // Move the pages back to the node source
  ma_memory_sampling_migrate(pageaddrs, pages, sources, status);

  us = (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);
  ns = us * 1000;
  ns /= (LOOPS * 2);
  bandwidth = ns / pages;
  fprintf(f, "%ld\t%ld\t%d\t%ld\t%ld\t%ld\n", source, dest, pages, pagesize*pages, ns, bandwidth);
  printf("%ld\t%ld\t%d\t%ld\t%ld\t%ld\n", source, dest, pages, pagesize*pages, ns, bandwidth);
}

void ma_memory_get_filename(char *type, char *filename, long source, long dest) {
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

  if (source == -1) {
    if (dest == -1)
      snprintf(filename, 1024, "%s/%s_for_memory_migration_%s.txt", directory, type, hostname);
    else
      snprintf(filename, 1024, "%s/%s_for_memory_migration_%s_dest_%ld.txt", directory, type, hostname, dest);
  }
  else {
    if (dest == -1)
      snprintf(filename, 1024, "%s/%s_for_memory_migration_%s_source_%ld.txt", directory, type, hostname, source);
    else
      snprintf(filename, 1024, "%s/%s_for_memory_migration_%s_source_%ld_dest_%ld.txt", directory, type, hostname, source, dest);
  }
  assert(rc < 1024);

  //printf("File %s\n", filename);

  mkdir(directory, 0755);
}

void ma_memory_insert_cost(p_tbx_slist_t migration_costs, size_t size_min, size_t size_max, float slope, float intercept, float correlation) {
  marcel_memory_migration_cost_t *migration_cost;

  migration_cost = malloc(sizeof(marcel_memory_migration_cost_t));
  migration_cost->size_min = size_min;
  migration_cost->size_max = size_max;
  migration_cost->slope = slope;
  migration_cost->intercept = intercept;
  migration_cost->correlation = correlation;

  tbx_slist_push(migration_costs, migration_cost);
}


void ma_memory_load_model_for_migration_cost(marcel_memory_manager_t *memory_manager) {
  char filename[1024];
  FILE *out;
  char line[1024];
  unsigned long source;
  unsigned long dest;
  unsigned long min_size;
  unsigned long max_size;
  float slope;
  float intercept;
  float correlation;

  ma_memory_get_filename("model", filename, -1, -1);
  out = fopen(filename, "r");
  if (!out) {
    printf("The model for the cost of the memory migration is not available\n");
    return;
  }
  mdebug_heap("Reading file %s\n", filename);
  fgets(line, 1024, out);
  while (!feof(out)) {
    fscanf(out, "%ld\t%ld\t%ld\t%ld\t%f\t%f\t%f\n", &source, &dest, &min_size, &max_size, &slope, &intercept, &correlation);

#ifdef PM2DEBUG
    if (marcel_heap_debug.show > PM2DEBUG_STDLEVEL) {
      marcel_printf("%ld\t%ld\t%ld\t%ld\t%f\t%f\t%f\n", source, dest, min_size, max_size, slope, intercept, correlation);
    }
#endif /* PM2DEBUG */

    ma_memory_insert_cost(memory_manager->migration_costs[source][dest], min_size, max_size, slope, intercept, correlation);
    ma_memory_insert_cost(memory_manager->migration_costs[dest][source], min_size, max_size, slope, intercept, correlation);
  }
  fclose(out);
}

void marcel_memory_sampling_of_migration_cost(unsigned long minsource, unsigned long maxsource, unsigned long mindest, unsigned long maxdest) {
  unsigned long pagesize;
  char filename[1024];
  FILE *out;
  void *buffer;
  int i, err;
  unsigned long source;
  unsigned long dest;
  unsigned long nodemask;
  int *status, *sources, *dests;
  void **pageaddrs;
  unsigned long maxnode;
  int pages;

  pagesize = getpagesize();
  maxnode = numa_max_node();

  {
    long source = -1;
    long dest = -1;
    if (minsource == maxsource) source = minsource;
    if (mindest == maxdest) dest = mindest;
    ma_memory_get_filename("sampling", filename, source, dest);
  }
  out = fopen(filename, "w");
  fprintf(out, "Source\tDest\tPages\tSize\tMigration_Time\n");
  printf("Source\tDest\tPages\tSize\tMigration_Time\n");

  for(source=minsource; source<=maxsource ; source++) {
    for(dest=mindest; dest<=maxdest ; dest++) {
      if (source >= dest) continue;

      // Allocate the pages
      buffer = mmap(NULL, 25000 * pagesize, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
      if (buffer < 0) {
        perror("mmap");
        exit(-1);
      }

      // Set the memory policy on node source
      nodemask = (1<<source);
      err = set_mempolicy(MPOL_BIND, &nodemask, maxnode+2);
      if (err < 0) {
        perror("set_mempolicy");
        exit(-1);
      }

      // Fill in the whole memory
      memset(buffer, 0, 25000*pagesize);

      // Set the page addresses
      pageaddrs = malloc(25000 * sizeof(void *));
      for(i=0; i<25000; i++)
        pageaddrs[i] = buffer + i*pagesize;

      // Set the other variables
      status = malloc(25000 * sizeof(int));
      sources = malloc(25000 * sizeof(int));
      dests = malloc(25000 * sizeof(int));
      for(i=0; i<25000 ; i++) dests[i] = dest;
      for(i=0; i<25000 ; i++) sources[i] = source;

      for(pages=1; pages<10 ; pages++) {
	ma_memory_sampling(source, dest, buffer, pages, pageaddrs, sources, dests, status, pagesize, out);
      }
      fflush(out);

      for(pages=10; pages<100 ; pages+=10) {
	ma_memory_sampling(source, dest, buffer, pages, pageaddrs, sources, dests, status, pagesize, out);
	fflush(out);
      }

      for(pages=100; pages<1000 ; pages+=100) {
	ma_memory_sampling(source, dest, buffer, pages, pageaddrs, sources, dests, status, pagesize, out);
	fflush(out);
      }

      for(pages=1000; pages<10000 ; pages+=1000) {
	ma_memory_sampling(source, dest, buffer, pages, pageaddrs, sources, dests, status, pagesize, out);
	fflush(out);
      }

      for(pages=10000; pages<=25000 ; pages+=5000) {
	ma_memory_sampling(source, dest, buffer, pages, pageaddrs, sources, dests, status, pagesize, out);
	fflush(out);
      }

      munmap(buffer, 25000 * pagesize);
      free(pageaddrs);
      free(status);
      free(sources);
      free(dests);
    }
  }
  fclose(out);
}

#endif /* MAMI_ENABLED */
