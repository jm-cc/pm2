#define _GNU_SOURCE /* a garder TOUT EN HAUT du fichier */

#include <ucontext.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <numaif.h>
#include <errno.h>
#include <string.h>

extern long move_pages(int pid, unsigned long count,
                       void **pages, const int *nodes, int *status, int flags);

void check_pages_location(void **pageaddrs, int nbpages, int node) {
  int *status;
  int i;
  int err;

  status = malloc(nbpages * sizeof(int));
  err = move_pages(0, nbpages, pageaddrs, NULL, status, 0);
  if (err < 0) {
    perror("move_pages");
    exit(-1);
  }

  for(i=0; i<nbpages; i++) {
    if (status[i] != node) {
      printf("  page #%d is NOT on node #%d (#%d)\n", i, node, status[i]);
      exit(1);
    }
  }
}

void move_pagenodes(void **pageaddrs, int nbpages, const int *nodes) {
  int *status;
  int i;
  int err;

  status = malloc(nbpages * sizeof(int));
  err = move_pages(0, nbpages, pageaddrs, nodes, status, MPOL_MF_MOVE);
  if (err < 0) {
    if (errno == ENOENT) {
      printf("warning. cannot move pages which have not been allocated\n");
    }
    else {
      perror("move_pages");
      exit(-1);
    }
  }

  for(i=0; i<nbpages; i++) {
    if (status[i] == -ENOENT)
      printf("  page #%d is not allocated\n", i);
    else
      printf("  page #%d is on node #%d\n", i, status[i]);
  }
}

void segv_handler(int sig, siginfo_t *info, void *_context) {
  struct sigaction act;
  ucontext_t *context = _context;
  int write = (context->uc_mcontext.gregs[REG_ERR] & 2);
#ifdef __x86_64__
  void *addr = (void *)(context->uc_mcontext.gregs[REG_CR2]);
#elif __i386__
  void *addr = (void *)(context->uc_mcontext.cr2);
#else
#error Architecture non supportee
#endif
  printf("Addr: %p\n", addr);

  void **pageaddrs;
  pageaddrs = malloc(sizeof(void *));
  pageaddrs[0] = addr;

  mprotect(addr, 1000, PROT_READ|PROT_WRITE|PROT_EXEC);

  int dest=1;
  move_pagenodes(pageaddrs, 1, &dest);

//  act.sa_sigaction = SIG_DFL;
//  sigaction(SIGSEGV, &act, NULL);
}

int main(int argc, char **argv) {
  struct sigaction act;
  int err;
  int pagesize;
  int *buffer;
  unsigned long nodemask;
  int maxnode;
  void **pageaddrs;

  act.sa_flags = SA_SIGINFO;
  act.sa_sigaction = segv_handler;
  sigaction(SIGSEGV, &act, NULL);

  pagesize = getpagesize();
  maxnode = numa_max_node();

  buffer = mmap(NULL, pagesize, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
  nodemask = (1<<0);
  mbind(buffer, pagesize, MPOL_BIND, &nodemask, maxnode+1, MPOL_MF_MOVE);
  memset(buffer, 0, pagesize);

  pageaddrs = malloc(sizeof(void *));
  pageaddrs[0] = buffer;
  check_pages_location(pageaddrs, 1, 0);

  printf("Buffer: %p\n", buffer);

  err = mprotect(buffer, pagesize, PROT_NONE);
  if (err < 0) perror("mprotect");
  buffer[0] = 1;
  check_pages_location(pageaddrs, 1, 1);
  buffer[1] = 1;

  printf("Buffer[0] = %d - Buffer[1] = %d - Buffer[2] = %d\n", buffer[0], buffer[1], buffer[2]);
}

