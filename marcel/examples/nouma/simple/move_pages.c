#include "marcel.h"


#include <sys/mman.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

/* used to get node pages by calling move_pages syscall */
#include <sys/syscall.h>
#include <numaif.h>


#ifdef LINUX_SYS

#ifndef __NR_move_pages
#warning __NR_move_pages not defined

#ifdef X86_64_ARCH
#define __NR_move_pages 279
#elif IA64_ARCH
#define __NR_move_pages 1276
#elif X86_ARCH
#define __NR_move_pages 317
#endif

#endif /* __NR_move_pages */

int my_move_pages(const pid_t pid, const unsigned long count,
                  const unsigned long *pages, const int *nodes,
                  int *status, int flags) {
  return syscall(__NR_move_pages, pid, count, pages, nodes, status, flags);
}

int main(int argc, char * argv[])
{
  marcel_init(&argc,argv);
#ifdef PROFILE
  profile_activate(FUT_ENABLE, MARCEL_PROF_MASK, 0);
#endif

  void* addr;
  int status;
  addr = malloc(sizeof(int));
  int ret = my_move_pages(0, 1, addr, NULL, &status, MPOL_MF_MOVE);
  fprintf(stderr,"_NR_move_pages : retour %d, status %d\n", ret, status);	
  ret = my_move_pages(0, 1, addr, NULL, &status, MPOL_MF_MOVE);
  fprintf(stderr,"_NR_move_pages : retour %d, status %d\n", ret, status);	
  ret = my_move_pages(0, 1, addr, NULL, &status, MPOL_MF_MOVE);
  fprintf(stderr,"_NR_move_pages : retour %d, status %d\n", ret, status);	
  ret = my_move_pages(0, 1, addr, NULL, &status, MPOL_MF_MOVE);
  fprintf(stderr,"_NR_move_pages : retour %d, status %d\n", ret, status);	
  ret = my_move_pages(0, 1, addr, NULL, &status, MPOL_MF_MOVE);
  fprintf(stderr,"_NR_move_pages : retour %d, status %d\n", ret, status);	
  ret = my_move_pages(0, 1, addr, NULL, &status, MPOL_MF_MOVE);
  fprintf(stderr,"_NR_move_pages : retour %d, status %d\n", ret, status);

#ifdef PROFILE
  profile_stop();
#endif

  return 0;
}

#endif
