#include "marcel.h"

#include <sys/mman.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

/* used to get node pages by calling move_pages syscall */
#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <numaif.h>

#define PAGES 4

#ifdef LINUX_SYS

int move_pages(const pid_t pid, const unsigned long count,
	       const unsigned long *pages, const int *nodes,
	       int *status, int flags) {
  return syscall(__NR_move_pages, pid, count, pages, nodes, status, flags);
}

void print_pagenodes(unsigned long *pageaddrs) {
  int pagenodes[PAGES];
  int i;
  int err;

  err = move_pages(0, PAGES, pageaddrs, NULL, pagenodes, 0);
  if (err < 0) {
    perror("move_pages");
    exit(-1);
  }

  for(i=0; i<PAGES; i++) {
    if (pagenodes[i] == -ENOENT)
      printf("  page #%d is not allocated\n", i);
    else
      printf("  page #%d is on node #%d\n", i, pagenodes[i]);
  }
}

void move_pagenodes(unsigned long *pageaddrs, const int *nodes) {
  int pagenodes[PAGES];
  int i;
  int err;

  err = move_pages(0, PAGES, pageaddrs, nodes, pagenodes, MPOL_MF_MOVE);
  if (err < 0) {
    perror("move_pages");
    exit(-1);
  }

  for(i=0; i<PAGES; i++) {
    if (pagenodes[i] == -ENOENT)
      printf("  page #%d is not allocated\n", i);
    else
      printf("  page #%d is on node #%d\n", i, pagenodes[i]);
  }
}

int main(int argc, char * argv[])
{
  unsigned long *buffer;
  unsigned long pageaddrs[PAGES];
  int i, status[PAGES];
  unsigned long pagesize;
  unsigned long maxnode;
  int nodes[PAGES];

  marcel_init(&argc,argv);
#ifdef PROFILE
  profile_activate(FUT_ENABLE, MARCEL_PROF_MASK, 0);
#endif

  maxnode = numa_max_node();
  printf("numa_max_node %d\n", maxnode);
  pagesize = getpagesize();
  buffer = malloc(PAGES * pagesize * sizeof(unsigned long));
  for(i=0; i<PAGES; i++)
    pageaddrs[i] = (unsigned long) (buffer + i*pagesize);

  printf("before touching the pages\n");
  print_pagenodes(pageaddrs);

  printf("move pages to node %d\n", maxnode);
  for(i=0; i<PAGES ; i++)
    nodes[i] = maxnode;
  move_pagenodes(pageaddrs, nodes);

  printf("filling in the buffer\n");
  for(i=0 ; i<PAGES*pagesize ; i++) {
    buffer[i] = (unsigned long)42;
  }

  print_pagenodes(pageaddrs);

  printf("move pages to node %d\n", 1);
  for(i=0; i<PAGES ; i++)
    nodes[i] = 1;
  move_pagenodes(pageaddrs, nodes);

#ifdef PROFILE
  profile_stop();
#endif

  return 0;
}

#endif
