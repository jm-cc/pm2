
#ifdef WIN_SYS

#define __WIN_MMAP_KERNEL__

#include "marcel.h"

#include <stdio.h>

void *ISOADDR_AREA_TOP;
static void *ISOADDR_AREA_BOTTOM;

caddr_t win_mmap(caddr_t __addr, size_t __len, int __prot, int __flags,
		    int __fd, off_t __off)
{
  return __addr;
}

int win_munmap(caddr_t __addr, size_t __len)
{
  return 0;
}

void marcel_for_win_init(int *argc, char *argv[])
{
  ISOADDR_AREA_BOTTOM = mmap(NULL,
	     ISOADDR_AREA_SIZE,
	     PROT_READ | PROT_WRITE,
	     MMAP_MASK,
	     FILE_TO_MAP, 0);

  if(ISOADDR_AREA_BOTTOM == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }

  ISOADDR_AREA_TOP = ISOADDR_AREA_BOTTOM + ISOADDR_AREA_SIZE;

  // Ajustements pour aligner sur une frontière de SLOT_SIZE
  ISOADDR_AREA_BOTTOM = (void *)(((unsigned long)ISOADDR_AREA_BOTTOM + SLOT_SIZE-1)
				 & ~(SLOT_SIZE-1));
  ISOADDR_AREA_TOP = (void *)((unsigned long)ISOADDR_AREA_TOP & ~(SLOT_SIZE-1));
}

#endif
