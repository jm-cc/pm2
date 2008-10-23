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
    perror("move_pages");
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
    perror("move_pages");
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
  void *buffer;
  char *filename;
  int file;
};

char *getfilename(int node) {
  char *filename;
  pid_t pid;

  filename = malloc(1024 * sizeof(char));
  pid = getpid();
  sprintf(filename, "/hugetlbfs/mami_pid_%d_node_%d", pid, node);
  return filename;
}

void free_with_huge_pages(huge_memory_t *memory) {
  int err;

  err = close(memory->file);
  if (err < 0) {
    perror("close");
    exit(-1);
  }
  err = unlink(memory->filename);
  if (err < 0) {
    perror("unlink");
    exit(-1);
  }
}

huge_memory_t *malloc_with_huge_pages(size_t size, int node) {
  huge_memory_t *memory;
  int err;
  unsigned long maxnode;
  unsigned long nodemask;

  memory = malloc(sizeof(huge_memory_t));
  maxnode = numa_max_node();
  memory->filename = getfilename(node);
  // Set the buffers on the nodes
  nodemask = (1<<node);

  memory->file = open(memory->filename, O_CREAT|O_RDWR, S_IRWXU);
  if (memory->file == -1) {
    perror("open");
    exit(-1);
  }
  printf("File opened\n");
  err = set_mempolicy(MPOL_PREFERRED, &nodemask, maxnode+2);
  if (err < 0) {
    perror("set_mempolicy");
    exit(-1);
  }
  printf("Memory policy set\n");
  memory->buffer = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE, memory->file, 0);
  if (memory->buffer == MAP_FAILED) {
    perror("mmap");
    exit(-1);
  }
  printf("Memory map\n");

  memset(memory->buffer, 0, size);
  printf("Memory memset\n");
  return memory;
}

int marcel_main(int argc, char **argv) {
  int i, err;
  huge_memory_t *memory;
  void **pageaddrs;
  int nbpages;
  int dest, node, realnode;
  size_t size;

  marcel_init(&argc, argv);

  node = atoi(argv[1]);
  dest = atoi(argv[2]);

  for(i=0 ; i<marcel_nbnodes ; i++) {
    printf("Hugepages on node #%d = %d\n", i, marcel_topo_node_level[i].huge_page_free);
  }

  size = 5 * gethugepagesize();
  memory = malloc_with_huge_pages(size, node);

  // Set the page addresses
  nbpages = size / gethugepagesize();
  if (nbpages * gethugepagesize() != size) nbpages++;
  pageaddrs = malloc(nbpages * sizeof(void *));
  for(i=0; i<nbpages ; i++) pageaddrs[i] = memory->buffer + i*gethugepagesize();

  realnode = check_pages_location(pageaddrs, nbpages, node);
  if (realnode == node) {
    printf("All pages are located on node #%d\n", node);
  }
  else {
    node = realnode;
    realnode = check_pages_location(pageaddrs, nbpages, node);
    if (realnode == node) {
      printf("All pages are located on node #%d\n", node);
    }
  }

  {
    int *dests = malloc(nbpages * sizeof(int));
    int *statuses = malloc(nbpages * sizeof(int));
    int i;
    for(i=0 ; i<nbpages ; i++) dests[i] = dest;
    my_move_pages(pageaddrs, nbpages, dests, statuses);
    print_pages(pageaddrs, nbpages);
  }

  free_with_huge_pages(memory);
}
