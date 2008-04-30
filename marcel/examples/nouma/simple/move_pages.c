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


int main(int argc, char * argv[])
{
	marcel_init(&argc,argv);
#ifdef PROFILE
   profile_activate(FUT_ENABLE, MARCEL_PROF_MASK, 0);
#endif

	void * addr;
	int node;
	addr = malloc(sizeof(int));
	int ret = syscall(__NR_move_pages, 0, 1, addr, NULL, &node, MPOL_MF_MOVE);
	fprintf(stderr,"_NR_move_pages : retour %d, node %d\n", ret, node);	
	ret = syscall(__NR_move_pages, 0, 1, addr, NULL, &node, MPOL_MF_MOVE);
	fprintf(stderr,"_NR_move_pages : retour %d, node %d\n", ret, node);	
	ret = syscall(__NR_move_pages, 0, 1, addr, NULL, &node, MPOL_MF_MOVE);
	fprintf(stderr,"_NR_move_pages : retour %d, node %d\n", ret, node);	
	ret = syscall(__NR_move_pages, 0, 1, addr, NULL, &node, MPOL_MF_MOVE);
	fprintf(stderr,"_NR_move_pages : retour %d, node %d\n", ret, node);	
	ret = syscall(__NR_move_pages, 0, 1, addr, NULL, &node, MPOL_MF_MOVE);
	fprintf(stderr,"_NR_move_pages : retour %d, node %d\n", ret, node);	
	ret = syscall(__NR_move_pages, 0, 1, addr, NULL, &node, MPOL_MF_MOVE);
	fprintf(stderr,"_NR_move_pages : retour %d, node %d\n", ret, node);

#ifdef PROFILE
	profile_stop();
#endif

	return 0;
}

#endif
