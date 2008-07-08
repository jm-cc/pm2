#include "marcel.h"
#include "marcel_topology.h"

#include <errno.h>
#include <sys/syscall.h>
#include <numaif.h>

#define PAGES 4

typedef struct thread_memory_s {
  void *address;
  size_t size;
  //marcel_t *thread;

  int *nodes;
  unsigned long pagesize;
  unsigned long *pageaddrs;
  int nbpages;
} thread_memory_t;

int thread_memory_attach(void *address, size_t size, thread_memory_t* thread_memory) {
  int i;

  thread_memory->address = address;
  thread_memory->size = size;

  // Count the number of pages
  thread_memory->pagesize = getpagesize();
  thread_memory->nbpages = size / thread_memory->pagesize;
  if (thread_memory->nbpages * thread_memory->pagesize != size) thread_memory->nbpages++;

  // Set the page addresses
  thread_memory->pageaddrs = malloc(thread_memory->nbpages * sizeof(unsigned long));
  for(i=0; i<thread_memory->nbpages ; i++)
    thread_memory->pageaddrs[i] = (unsigned long) (address + i*thread_memory->pagesize);

  // fill in the nodes
  print_pagenodes(-1, thread_memory->pageaddrs, thread_memory->nbpages);
  return 0;
}

int thread_memory_move(thread_memory_t* thread_memory, int node) {
}

int thread_memory_detach(thread_memory_t* thread_memory) {
}

int move_pages(const pid_t pid, const unsigned long count,
	       const unsigned long *pages, const int *nodes,
	       int *status, int flags) {
  return syscall(__NR_move_pages, pid, count, pages, nodes, status, flags);
}

void print_pagenodes(int id, unsigned long *pageaddrs, int nbpages) {
  int status[nbpages];
  int i;
  int err;

  err = move_pages(0, nbpages, pageaddrs, NULL, status, 0);
  if (err < 0) {
    perror("move_pages");
    exit(-1);
  }

  for(i=0; i<nbpages; i++) {
    if (status[i] == -ENOENT)
      marcel_printf("[%d]  page #%d is not allocated\n", id, i);
    else
      marcel_printf("[%d]  page #%d is on node #%d\n", id, i, status[i]);
  }
}

void move_pagenodes(unsigned long *pageaddrs, const int *nodes, int nbpages) {
  int status[nbpages];
  int i;
  int err;

  err = move_pages(0, nbpages, pageaddrs, nodes, status, MPOL_MF_MOVE);
  if (err < 0) {
    perror("move_pages");
    exit(-1);
  }

  for(i=0; i<nbpages; i++) {
    if (status[i] == -ENOENT)
      printf("  page #%d is not allocated\n", i);
    else
      printf("  page #%d is on node #%d\n", i, status[i]);
  }
}

any_t memory(any_t arg) {
  int id = (intptr_t) arg;
  unsigned long *buffer;
  int nodes[PAGES];
  thread_memory_t thread_memory;

  marcel_fprintf(stderr,"[%d] launched on VP #%u\n", id, marcel_current_vp());

  buffer = malloc(PAGES * getpagesize() * sizeof(unsigned long));
  memset(buffer, 0, PAGES * getpagesize() * sizeof(unsigned long));

  marcel_fprintf(stderr,"[%d] allocating buffer %p of size %u with pagesize %u\n", id, buffer, PAGES * getpagesize() * sizeof(unsigned long),
                 getpagesize());

  //  for(i=0; i<PAGES ; i++)
  //    nodes[i] = marcel_current_vp();
  //  move_pagenodes(pageaddrs, nodes);

  thread_memory_attach(buffer, PAGES * getpagesize() * sizeof(unsigned long), &thread_memory);
}

int marcel_main(int argc, char * argv[]) {
  marcel_t threads[2];
  marcel_attr_t attr;

  marcel_init(&argc,argv);

  marcel_attr_init(&attr);
  // Start the 1st thread on the first VP 
  //  marcel_print_level(&marcel_topo_vp_level[0], stderr, 1, 1, " ", "#", ":", "");
  marcel_attr_settopo_level(&attr, &marcel_topo_vp_level[0]);
  marcel_create(&threads[0], &attr, memory, (any_t) (intptr_t) 0);

  // Start the 2nd thread on the last VP 
  //  marcel_print_level(&marcel_topo_vp_level[marcel_nbvps()-1], stderr, 1, 1, " ", "#", ":", "");
  marcel_attr_settopo_level(&attr, &marcel_topo_vp_level[marcel_nbvps()-1]);
  marcel_create(&threads[1], &attr, memory, (any_t) (intptr_t) 1);

  // Wait for the threads to complete
  marcel_join(threads[0], NULL);
  marcel_join(threads[1], NULL);
}

