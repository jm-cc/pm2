#include <stdio.h>

#if !defined(MM_HEAP_ENABLED)
int main(int argc, char *argv[]) {
  fprintf(stderr, "This application needs 'Heap allocator' to be enabled\n");
}
#else

#include <marcel.h>
#include <numaif.h>

int main(int argc, char * argv[])
{
  void* addr;
  int status, ret;
  void **pageaddr;

  marcel_init(&argc,argv);
#ifdef PROFILE
  profile_activate(FUT_ENABLE, MARCEL_PROF_MASK, 0);
#endif

  addr = malloc(sizeof(int));
  pageaddr = malloc(sizeof(void *));
  pageaddr[0] = addr;

  ret = move_pages(0, 1, pageaddr, NULL, &status, MPOL_MF_MOVE);
  fprintf(stderr,"_NR_move_pages : retour %d, status %d\n", ret, status);
  ret = move_pages(0, 1, pageaddr, NULL, &status, MPOL_MF_MOVE);
  fprintf(stderr,"_NR_move_pages : retour %d, status %d\n", ret, status);
  ret = move_pages(0, 1, pageaddr, NULL, &status, MPOL_MF_MOVE);
  fprintf(stderr,"_NR_move_pages : retour %d, status %d\n", ret, status);
  ret = move_pages(0, 1, pageaddr, NULL, &status, MPOL_MF_MOVE);
  fprintf(stderr,"_NR_move_pages : retour %d, status %d\n", ret, status);
  ret = move_pages(0, 1, pageaddr, NULL, &status, MPOL_MF_MOVE);
  fprintf(stderr,"_NR_move_pages : retour %d, status %d\n", ret, status);
  ret = move_pages(0, 1,pageaddr, NULL, &status, MPOL_MF_MOVE);
  fprintf(stderr,"_NR_move_pages : retour %d, status %d\n", ret, status);

#ifdef PROFILE
  profile_stop();
#endif

  return 0;
}

#endif
