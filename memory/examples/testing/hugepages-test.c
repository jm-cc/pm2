#include "marcel.h"
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <numaif.h>

void print_pages(void **pageaddrs, int nbpages) {
  int *statuses;
  int i, err;

  statuses = malloc(nbpages * sizeof(int));
  err = move_pages(0, nbpages, pageaddrs, NULL, statuses, 0);
  if (err < 0) {
    perror("move_pages (print_pages)");
    exit(-1);
  }

  // Display information
  for(i=0; i<nbpages; i++) {
    if (statuses[i] == -ENOENT)
      printf("  page #%d is not allocated\n", i);
    else
      printf("  page #%d is on node #%d\n", i, statuses[i]);
  }
}

int check_pages_location(void **pageaddrs, int nbpages, int node) {
  int *statuses;
  int i, err;

  statuses = malloc(nbpages * sizeof(int));
  err = move_pages(0, nbpages, pageaddrs, NULL, statuses, 0);
  if (err < 0) {
    perror("move_pages (check_pages_location)");
    exit(-1);
  }

  // Display information
  for(i=0; i<nbpages; i++) {
    if (statuses[i] != node) {
      printf("  page #%d is NOT on node #%d but on node #%d\n", i, node, statuses[i]);
      return statuses[i];
    }
  }
  return node;
}

int my_move_pages(void **pageaddrs, int pages, int *nodes, int *status) {
  int err=0;

  err = move_pages(0, pages, pageaddrs, nodes, status, MPOL_MF_MOVE);
  if (err < 0) perror("move_pages");
  return err;
}

static unsigned long hugepagesize=-1;
unsigned long gethugepagesize(void) {
  if (hugepagesize == -1) {
    char line[1024];
    FILE *f;
    unsigned long size=-1;

    printf("Reading /proc/meminfo\n");
    f = fopen("/proc/meminfo", "r");
    while (!(feof(f))) {
      fgets(line, 1024, f);
      if (!strncmp(line, "Hugepagesize:", 13)) {
        char *c, *endptr;

        c = strchr(line, ':') + 1;
        size = strtol(c, &endptr, 0);
        hugepagesize = size * 1024;
      }
    }
    fclose(f);
    if (size == -1) {
      printf("Hugepagesize information not available.");
      exit(-1);
    }
  }
  return hugepagesize;
}

typedef struct huge_memory_s huge_memory_t;

struct huge_memory_s {
  int *buffer;
  int file;
  void **pageaddrs;
  int nbpages;
};

void free_with_huge_pages(huge_memory_t *memory) {
  int err;

  err = close(memory->file);
  if (err < 0) {
    perror("close");
    exit(-1);
  }
}

int unlinked_fd() {
  const char *path;
  char name[1024];
  int fd;

  path = "/hugetlbfs";
  name[sizeof(name)-1] = '\0';

  strcpy(name, path);
  strncat(name, "/libhugetlbfs.tmp.XXXXXX", sizeof(name)-1);
  /* FIXME: deal with overflows */

  fd = mkstemp64(name);
  if (fd < 0) {
    perror("mkstemp");
    exit(-1);
  }
  unlink(name);
  return fd;
}

huge_memory_t *malloc_with_huge_pages(size_t size, int node) {
  huge_memory_t *memory;
  int i, err;
  unsigned long maxnode;
  unsigned long nodemask;
  struct iovec *iov;

  memory = malloc(sizeof(huge_memory_t));
  maxnode = numa_max_node();
  // Set the buffers on the nodes
  nodemask = (1<<node);

  memory->file = unlinked_fd();
  if (memory->file == -1) {
    perror("open");
    exit(-1);
  }
  printf("File opened\n");
  memory->buffer = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE, memory->file, 0);
  if (memory->buffer == MAP_FAILED) {
    perror("mmap");
    exit(-1);
  }
  printf("Memory map\n");
  err = set_mempolicy(MPOL_PREFERRED, &nodemask, maxnode+2);
  if (err < 0) {
    perror("set_mempolicy");
    exit(-1);
  }
  printf("Memory policy set\n");

  memory->nbpages = size / gethugepagesize();
  if (memory->nbpages * gethugepagesize() != size) memory->nbpages++;
  memory->pageaddrs = malloc(memory->nbpages * sizeof(void *));
  for(i=0; i<memory->nbpages ; i++) memory->pageaddrs[i] = memory->buffer + i*gethugepagesize();

  printf("Nombre de pages: %d\n", memory->nbpages);

  iov = malloc(memory->nbpages * sizeof(struct iovec));
  for(i = 0; i<memory->nbpages; i++) {
    iov[i].iov_base = memory->pageaddrs[i];
    iov[i].iov_len = 1;
  }

  {
    struct stat st;
    fstat(memory->file, &st);
    printf("Size %ld\n", st.st_size);
  }

#if 0
  err = readv(memory->file, iov, memory->nbpages);
  if (err < 0) {
    perror("readv");
    exit(-1);
  }
  if (err != memory->nbpages) {
    printf("Failed to reserve %ld hugepages (%d != %d)\n", memory->nbpages, err, memory->nbpages);
    exit(-1);
  }
#else
  memset(memory->buffer, 0, size);
#endif
  printf("Memory prefault\n");
  return memory;
}

int marcel_main(int argc, char **argv) {
  int i, err;
  huge_memory_t *memory;
  int nbpages, dest, node, realnode;
  size_t size;

  marcel_init(&argc, argv);

  if (argc < 3) {
    marcel_printf("Error. Syntax: hugepages-test <nbpages> <node> <dest>\n");
    marcel_end();
    exit(1);
  }

  nbpages = atoi(argv[1]);
  node = atoi(argv[2]);
  dest = atoi(argv[3]);

  for(i=0 ; i<marcel_nbnodes ; i++) {
    printf("Hugepages on node #%d = %d\n", i, marcel_topo_node_level[i].huge_page_free);
  }

  size = nbpages * gethugepagesize();
  memory = malloc_with_huge_pages(size, node);

  realnode = check_pages_location(memory->pageaddrs, memory->nbpages, node);
  if (realnode == node) {
    printf("All pages are located on node #%d\n", node);
  }
  else {
    node = realnode;
    realnode = check_pages_location(memory->pageaddrs, memory->nbpages, node);
    if (realnode == node) {
      printf("All pages are located on node #%d\n", node);
    }
    else {
      printf("Not all the pages are located on node #%d\n", node);
    }
  }

  {
    int *dests = malloc(memory->nbpages * sizeof(int));
    int *statuses = malloc(memory->nbpages * sizeof(int));
    for(i=0 ; i<memory->nbpages ; i++) dests[i] = dest;
    my_move_pages(memory->pageaddrs, memory->nbpages, dests, statuses);
    //    print_pages(memory->pageaddrs, memory->nbpages);
  }

  for(i=0 ; i<size/sizeof(int) ; i++) memory->buffer[i] = 42;

  free_with_huge_pages(memory);
}
