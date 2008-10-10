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
#include <assert.h>
#include <numa.h>
#include <numaif.h>
#include <sys/mman.h>

#define LOOPS_FOR_MEMORY_MIGRATION  1000
#define LOOPS_FOR_MEMORY_ACCESS     100000

void ma_memory_sampling_check_pages_location(void **pageaddrs, int pages, int node) {
  int *pagenodes;
  int i;
  int err;

  mdebug_heap("check location is #%d\n", node);

  pagenodes = malloc(pages * sizeof(int));
  err = move_pages(0, pages, pageaddrs, NULL, pagenodes, 0);
  if (err < 0) {
    perror("move_pages (check_pages_location)");
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

void ma_memory_sampling_migrate_pages(void **pageaddrs, int pages, int *nodes, int *status) {
  int err;

  mdebug_heap("binding on numa node #%d\n", nodes[0]);

  err = move_pages(0, pages, pageaddrs, nodes, status, MPOL_MF_MOVE);

  if (err < 0) {
    perror("move_pages (set_bind)");
    exit(-1);
  }
}

static
void ma_memory_sampling_of_memory_migration(unsigned long source, unsigned long dest, void *buffer, int pages,
                                            void **pageaddrs, int *sources, int *dests, int *status,
                                            unsigned long pagesize, FILE *f) {
  int i;
  struct timeval tv1, tv2;
  unsigned long us, ns, bandwidth;

  // Check the location of the pages
  ma_memory_sampling_check_pages_location(pageaddrs, pages, source);

  // Migrate the pages back and forth between the nodes dest and source
  gettimeofday(&tv1, NULL);
  for(i=0 ; i<LOOPS_FOR_MEMORY_MIGRATION ; i++) {
    ma_memory_sampling_migrate_pages(pageaddrs, pages, dests, status);
    ma_memory_sampling_migrate_pages(pageaddrs, pages, sources, status);
  }
  ma_memory_sampling_migrate_pages(pageaddrs, pages, dests, status);
  gettimeofday(&tv2, NULL);

  // Check the location of the pages
  ma_memory_sampling_check_pages_location(pageaddrs, pages, dest);

  // Move the pages back to the node source
  ma_memory_sampling_migrate_pages(pageaddrs, pages, sources, status);

  us = (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);
  ns = us * 1000;
  ns /= (LOOPS_FOR_MEMORY_MIGRATION * 2);
  bandwidth = ns / pages;
  fprintf(f, "%ld\t%ld\t%d\t%ld\t%ld\t%ld\n", source, dest, pages, pagesize*pages, ns, bandwidth);
  printf("%ld\t%ld\t%d\t%ld\t%ld\t%ld\n", source, dest, pages, pagesize*pages, ns, bandwidth);
}

void ma_memory_get_filename(char *type, char *filename, long source, long dest) {
  char directory[1024];
  char hostname[1024];
  int rc = 0;
  const char* pathname;

  if (gethostname(hostname, 1023) < 0) {
    perror("gethostname");
    exit(1);
  }
  pathname = getenv("PM2_SAMPLING_DIR");
  if (pathname) {
    rc = snprintf(directory, 1024, "%s/marcel", pathname);
  }
  else {
    rc = snprintf(directory, 1024, "/var/local/pm2/marcel");
  }
  assert(rc < 1024);

  if (source == -1) {
    if (dest == -1)
      snprintf(filename, 1024, "%s/%s_%s.dat", directory, type, hostname);
    else
      snprintf(filename, 1024, "%s/%s_%s_dest_%ld.dat", directory, type, hostname, dest);
  }
  else {
    if (dest == -1)
      snprintf(filename, 1024, "%s/%s_%s_source_%ld.dat", directory, type, hostname, source);
    else
      snprintf(filename, 1024, "%s/%s_%s_source_%ld_dest_%ld.dat", directory, type, hostname, source, dest);
  }
  assert(rc < 1024);
}

void ma_memory_insert_migration_cost(p_tbx_slist_t migration_costs, size_t size_min, size_t size_max, float slope, float intercept, float correlation) {
  marcel_memory_migration_cost_t *migration_cost;

  migration_cost = malloc(sizeof(marcel_memory_migration_cost_t));
  migration_cost->size_min = size_min;
  migration_cost->size_max = size_max;
  migration_cost->slope = slope;
  migration_cost->intercept = intercept;
  migration_cost->correlation = correlation;

  tbx_slist_push(migration_costs, migration_cost);
}


void ma_memory_load_model_for_memory_migration(marcel_memory_manager_t *memory_manager) {
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
  float bandwidth;

  ma_memory_get_filename("model_for_memory_migration", filename, -1, -1);
  out = fopen(filename, "r");
  if (!out) {
    printf("The model for the cost of the memory migration is not available\n");
    return;
  }
  mdebug_heap("Reading file %s\n", filename);
  fgets(line, 1024, out);
  mdebug_heap("Reading line %s\n", line);
  while (!(feof(out))) {
    if (fscanf(out, "%ld\t%ld\t%ld\t%ld\t%f\t%f\t%f\t%f\n", &source, &dest, &min_size, &max_size, &slope, &intercept, &correlation, &bandwidth) == EOF) {
      break;
    }
#ifdef PM2DEBUG
    if (marcel_heap_debug.show > PM2DEBUG_STDLEVEL) {
      marcel_printf("%ld\t%ld\t%ld\t%ld\t%f\t%f\t%f\t%f\n", source, dest, min_size, max_size, slope, intercept, correlation, bandwidth);
    }
#endif /* PM2DEBUG */

    ma_memory_insert_migration_cost(memory_manager->migration_costs[source][dest], min_size, max_size, slope, intercept, correlation);
    ma_memory_insert_migration_cost(memory_manager->migration_costs[dest][source], min_size, max_size, slope, intercept, correlation);
  }
  fclose(out);
}

void marcel_memory_sampling_of_memory_migration(unsigned long minsource, unsigned long maxsource, unsigned long mindest, unsigned long maxdest) {
  char filename[1024];
  FILE *out;
  int i;
  unsigned long source;
  unsigned long dest;
  marcel_t thread;
  marcel_attr_t attr;

  any_t migration(any_t arg) {
    void *buffer;
    unsigned long nodemask;
    void **pageaddrs;
    int *status, *sources, *dests;
    int maxnode;
    int pages;
    int err;
    unsigned long pagesize;

    pagesize = getpagesize();
    maxnode = numa_max_node();

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
      ma_memory_sampling_of_memory_migration(source, dest, buffer, pages, pageaddrs, sources, dests, status, pagesize, out);
    }
    fflush(out);

    for(pages=10; pages<100 ; pages+=10) {
      ma_memory_sampling_of_memory_migration(source, dest, buffer, pages, pageaddrs, sources, dests, status, pagesize, out);
      fflush(out);
    }

    for(pages=100; pages<1000 ; pages+=100) {
      ma_memory_sampling_of_memory_migration(source, dest, buffer, pages, pageaddrs, sources, dests, status, pagesize, out);
      fflush(out);
    }

    for(pages=1000; pages<10000 ; pages+=1000) {
      ma_memory_sampling_of_memory_migration(source, dest, buffer, pages, pageaddrs, sources, dests, status, pagesize, out);
      fflush(out);
    }

    for(pages=10000; pages<=25000 ; pages+=5000) {
      ma_memory_sampling_of_memory_migration(source, dest, buffer, pages, pageaddrs, sources, dests, status, pagesize, out);
      fflush(out);
    }

    munmap(buffer, 25000 * pagesize);
    free(pageaddrs);
    free(status);
    free(sources);
    free(dests);
    return 0;
  }

  {
    long source = -1;
    long dest = -1;
    if (minsource == maxsource) source = minsource;
    if (mindest == maxdest) dest = mindest;
    ma_memory_get_filename("sampling_for_memory_migration", filename, source, dest);
  }
  out = fopen(filename, "w");
  if (!out) {
    printf("Error when opening file <%s>\n", filename);
    return;
  }
  fprintf(out, "Source\tDest\tPages\tSize\tMigration_Time\n");
  printf("Source\tDest\tPages\tSize\tMigration_Time\n");

  marcel_attr_init(&attr);
  for(source=minsource; source<=maxsource ; source++) {
    for(dest=mindest; dest<=maxdest ; dest++) {

      if (dest == source) continue;

      // Create a thread on node source
      marcel_attr_settopo_level(&attr, &marcel_topo_node_level[source]);
      marcel_create(&thread, &attr, migration, NULL);
      marcel_join(thread, NULL);
    }
  }
  fclose(out);
  printf("Sampling saved in <%s>\n", filename);
}

void ma_memory_load_model_for_memory_access(marcel_memory_manager_t *memory_manager) {
  char filename[1024];
  FILE *out;
  char line[1024];
  unsigned long source, dest;
  long long int size, wtime, rtime;
  float rcacheline, wcacheline;

  ma_memory_get_filename("sampling_for_memory_access", filename, -1, -1);
  out = fopen(filename, "r");
  if (!out) {
    printf("The model for the cost of the memory access is not available\n");
    return;
  }
  mdebug_heap("Reading file %s\n", filename);
  fgets(line, 1024, out);
  while (!feof(out)) {
    fscanf(out, "%ld\t%ld\t%lld\t%lld\t%f\t%lld\t%f\n", &source, &dest, &size, &rtime, &rcacheline, &wtime, &wcacheline);

#ifdef PM2DEBUG
    if (marcel_heap_debug.show > PM2DEBUG_STDLEVEL) {
      marcel_printf("%ld\t%ld\t%lld\t%lld\t%f\t%lld\t%f\n", source, dest, size, rtime, rcacheline, wtime, wcacheline);
    }
#endif /* PM2DEBUG */
    memory_manager->writing_access_costs[source][dest].cost = wcacheline;
    memory_manager->reading_access_costs[source][dest].cost = rcacheline;
  }
}

void marcel_memory_sampling_of_memory_access(marcel_memory_manager_t *memory_manager) {
  char filename[1024];
  FILE *out;
  unsigned long t, node;
  unsigned long long **rtimes, **wtimes;
  marcel_t thread;
  marcel_attr_t attr;
  int **buffers;
  long long int size=100;

  any_t writer(any_t arg) {
    int *buffer;
    int i, j, node;
    unsigned int gold = 1.457869;

    node = marcel_self()->id;
    buffer = buffers[node];

    for(j=0 ; j<LOOPS_FOR_MEMORY_ACCESS ; j++) {
      for(i=0 ; i<size ; i++) {
        __builtin_ia32_movnti((void*) &buffer[i], gold);
      }
    }
    return NULL;
  }
  any_t reader(any_t arg) {
    int *buffer;
    int i, j, node;

    node = marcel_self()->id;
    buffer = buffers[node];

    for(j=0 ; j<LOOPS_FOR_MEMORY_ACCESS ; j++) {
      for(i=0 ; i<size ; i+=memory_manager->cache_line_size/4) {
        __builtin_prefetch((void*)&buffer[i]);
        __builtin_ia32_clflush((void*)&buffer[i]);
      }
    }
    return NULL;
  }

  marcel_attr_init(&attr);

  ma_memory_get_filename("sampling_for_memory_access", filename, -1, -1);
  out = fopen(filename, "w");
  if (!out) {
    printf("Error when opening file <%s>\n", filename);
    return;
  }

  rtimes = (unsigned long long **) malloc(marcel_nbnodes * sizeof(unsigned long long *));
  wtimes = (unsigned long long **) malloc(marcel_nbnodes * sizeof(unsigned long long *));
  for(node=0 ; node<marcel_nbnodes ; node++) {
    rtimes[node] = (unsigned long long *) malloc(marcel_nbnodes * sizeof(unsigned long long));
    wtimes[node] = (unsigned long long *) malloc(marcel_nbnodes * sizeof(unsigned long long));
  }

  buffers = (int **) malloc(marcel_nbnodes * sizeof(int *));
  // Allocate memory on each node
  for(node=0 ; node<marcel_nbnodes ; node++) {
    buffers[node] = marcel_memory_allocate_on_node(memory_manager, size*sizeof(int), node);
  }

  // Create a thread on node t to work on memory allocated on node node
  for(t=0 ; t<marcel_nbnodes ; t++) {
    for(node=0 ; node<marcel_nbnodes ; node++) {
      struct timeval tv1, tv2;
      unsigned long us;

      marcel_attr_setid(&attr, node);
      marcel_attr_settopo_level(&attr, &marcel_topo_node_level[t]);

      gettimeofday(&tv1, NULL);
      marcel_create(&thread, &attr, writer, NULL);
      // Wait for the thread to complete
      marcel_join(thread, NULL);
      gettimeofday(&tv2, NULL);

      us = (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);
      wtimes[node][t] = us*1000;
    }
  }

  // Create a thread on node t to work on memory allocated on node node
  for(t=0 ; t<marcel_nbnodes ; t++) {
    for(node=0 ; node<marcel_nbnodes ; node++) {
      struct timeval tv1, tv2;
      unsigned long us;

      marcel_attr_setid(&attr, node);
      marcel_attr_settopo_level(&attr, &marcel_topo_node_level[t]);

      gettimeofday(&tv1, NULL);
      marcel_create(&thread, &attr, reader, NULL);
      // Wait for the thread to complete
      marcel_join(thread, NULL);
      gettimeofday(&tv2, NULL);

      us = (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);
      rtimes[node][t] = us*1000;
    }
  }

  printf("Thread\tNode\tBytes\t\tReader (ns)\tCache Line (ns)\tWriter (ns)\tCache Line (ns)\n");
  fprintf(out, "Thread\tNode\tBytes\t\tReader (ns)\tCache Line (ns)\tWriter (ns)\tCache Line (ns)\n");
  for(t=0 ; t<marcel_nbnodes ; t++) {
    for(node=0 ; node<marcel_nbnodes ; node++) {
      printf("%ld\t%ld\t%lld\t%lld\t%f\t%lld\t%f\n",
             t, node, LOOPS_FOR_MEMORY_ACCESS*size*4,
             rtimes[node][t],
             (float)(rtimes[node][t]) / (float)(LOOPS_FOR_MEMORY_ACCESS*size*4) / (float)memory_manager->cache_line_size,
             wtimes[node][t],
             (float)(wtimes[node][t]) / (float)(LOOPS_FOR_MEMORY_ACCESS*size*4) / (float)memory_manager->cache_line_size);
      fprintf(out, "%ld\t%ld\t%lld\t%lld\t%f\t%lld\t%f\n",
              t, node, LOOPS_FOR_MEMORY_ACCESS*size*4,
              rtimes[node][t],
              (float)(rtimes[node][t]) / (float)(LOOPS_FOR_MEMORY_ACCESS*size*4) / (float)memory_manager->cache_line_size,
              wtimes[node][t],
              (float)(wtimes[node][t]) / (float)(LOOPS_FOR_MEMORY_ACCESS*size*4) / (float)memory_manager->cache_line_size);
    }
  }

  fclose(out);
  printf("Sampling saved in <%s>\n", filename);
}

#endif /* MAMI_ENABLED */
