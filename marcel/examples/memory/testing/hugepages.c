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

    printf("Reading /proc/meminfo\n");
    f = fopen("/proc/meminfo", "r");
    while (!(feof(f))) {
      fgets(line, 1024, f);
      if (!strncmp(line, "Hugepagesize:", 13)) {
        char *c, *endptr;
        unsigned long size;

        c = strchr(line, ':') + 1;
        size = strtol(c, &endptr, 0);
        hugepagesize = size * 1024;
      }
    }
  }
  return hugepagesize;
}

int marcel_main(int argc, char **argv) {
  int i, err, file;
  void *buffer;
  size_t size;
  void **pageaddrs;
  int nbpages;
  unsigned long maxnode;
  unsigned long nodemask;
  int dest, node, realnode;

  node = atoi(argv[1]);
  dest = atoi(argv[2]);
  maxnode = numa_max_node();
  size = 5 * gethugepagesize();

  // Set the buffers on the nodes
  nodemask = (1<<node);

  file = open("/hugetlbfs/mami", O_CREAT, S_IRWXU);
  if (file == -1) {
    perror("open");
    return -1;
  }
  printf("File opened\n");
  err = set_mempolicy(MPOL_PREFERRED, &nodemask, maxnode+2);
  if (err < 0) {
    perror("set_mempolicy");
    return -1;
  }
  printf("Memory policy set\n");
  buffer = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE, file, 0);
  if (buffer == MAP_FAILED) {
    perror("mmap");
    return -1;
  }
  printf("Memory map\n");

  memset(buffer, 0, size);
  printf("Memory memset\n");

  // Set the page addresses
  nbpages = size / gethugepagesize();
  if (nbpages * gethugepagesize() != size) nbpages++;
  pageaddrs = malloc(nbpages * sizeof(void *));
  for(i=0; i<nbpages ; i++) pageaddrs[i] = buffer + i*gethugepagesize();

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

  err = close(file);
  if (err < 0) {
    perror("close");
    return -1;
  }
  err = unlink("/hugetlbfs/mami");
  if (err < 0) {
    perror("unlink");
    return -1;
  }
}
