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
$Log: marcel_alloc.c,v $
Revision 1.4  2000/04/11 09:07:20  rnamyst
Merged the "reorganisation" development branch.

Revision 1.3.2.1  2000/03/15 15:55:05  vdanjean
réorganisation de marcel : commit pour CVS

Revision 1.3  2000/02/28 10:25:00  rnamyst
Changed #include <> into #include "".

Revision 1.2  2000/01/31 15:57:11  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "marcel.h"
#include "marcel_alloc.h"
#include "safe_malloc.h"

static void *next_slot = (void *)SLOT_AREA_TOP;

static marcel_lock_t alloc_lock = MARCEL_LOCK_INIT;

#if defined(SOLARIS_SYS) || defined(IRIX_SYS) || defined(FREEBSD_SYS)
static int __zero_fd;
#endif

static struct {
   unsigned last;
   void *stacks[MAX_STACK_CACHE];
} stack_cache = { 0, };

void marcel_slot_init(void)
{
#if defined(SOLARIS_SYS) || defined(IRIX_SYS) || defined(FREEBSD_SYS)
  __zero_fd = open("/dev/zero", O_RDWR);
#endif
  stack_cache.last = 0;
}

extern volatile unsigned long threads_created_in_cache;

void *marcel_slot_alloc(void)
{
  register void *ptr;

  marcel_lock_acquire(&alloc_lock);

  if(stack_cache.last) {

    ptr = stack_cache.stacks[--stack_cache.last];

    threads_created_in_cache++;

  } else {

    next_slot -= SLOT_SIZE;

    ptr = mmap(next_slot,
	       SLOT_SIZE,
	       PROT_READ | PROT_WRITE,
	       MMAP_MASK,
	       FILE_TO_MAP, 0);

    if(ptr == MAP_FAILED) {
      perror("mmap");
      RAISE(CONSTRAINT_ERROR);
    }
  }

  marcel_lock_release(&alloc_lock);

  return ptr;
}

void marcel_slot_free(void *addr)
{
  marcel_lock_acquire(&alloc_lock);

  if(stack_cache.last < MAX_STACK_CACHE) /* Si le cache n'est pas plein */
    stack_cache.stacks[stack_cache.last++] = addr;
  else
    if(munmap(addr, SLOT_SIZE) == -1)
      RAISE(CONSTRAINT_ERROR);

  marcel_lock_release(&alloc_lock);
}

void marcel_slot_exit(void)
{
  while(stack_cache.last) {
    stack_cache.last--;
    if(munmap(stack_cache.stacks[stack_cache.last], SLOT_SIZE) == -1)
      RAISE(CONSTRAINT_ERROR);
  }
}
