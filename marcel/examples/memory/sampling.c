#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
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
      printf("  page #%d is not located on node #%d\n", node);
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
  if (err < 0) {
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
      printf("  page #%d has not been moved on node #%d\n", dest);
      exit(-1);
    }
  }

  free(pageaddrs);
  free(status);
  free(sources);
  free(dests);

  us = (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);
  ns = us * 1000;
  ns /= (LOOPS * 2);
  fprintf(f, "%d\t%d\t%d\t%ld\n", source, dest, pages, ns);
  printf("%d\t%d\t%d\t%ld\n", source, dest, pages, ns);
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
  snprintf(filename, 1024, "%s/sampling_%s.txt", directory, hostname);
  assert(rc < 1024);
  printf("File %s\n", filename);

  mkdir(directory, 0755);
}

void marcel_memory_sampling() {
  unsigned long pagesize;
  unsigned long maxnode;
  unsigned long source, dest;
  char filename[1024];
  FILE *out;
  int nbpages[] = { 10, 50, 100, 500, 1000, 5000, -1 };

  pagesize = getpagesize();
  maxnode = numa_max_node()+1;

  ma_memory_sampling_get_filename(filename);
  out = fopen(filename, "w");
  fprintf(out, "Source\tDest\tNb_pages\tMigration_Time\n");

  for(source=0; source<maxnode ; source++) {
    for(dest=0; dest<maxnode ; dest++) {
      int *pages = nbpages;

      if (source >= dest) continue;

      while (*pages != -1) {
        ma_memory_sampling(source, dest, *pages, maxnode, pagesize, out);
        pages ++;
      }
    }
  }
  fclose(out);
}

int main(int argc, char **argv) {
  marcel_memory_sampling();
}
