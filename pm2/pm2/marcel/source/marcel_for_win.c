/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.

______________________________________________________________________________
$Log: marcel_for_win.c,v $
Revision 1.1  2001/01/03 14:12:24  rnamyst
Added support for Win2K. For now, only Marcel is available under Cygwin...

______________________________________________________________________________
*/

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
