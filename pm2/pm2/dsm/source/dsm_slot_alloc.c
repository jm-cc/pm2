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
$Log: dsm_slot_alloc.c,v $
Revision 1.3  2000/09/25 12:05:08  gantoniu
Added double mapping after tests.

Revision 1.2  2000/07/14 16:16:56  gantoniu
Merged with branch dsm3

Revision 1.1.6.1  2000/06/13 16:44:03  gantoniu
New dsm branch.

Revision 1.1.4.2  2000/06/09 17:55:49  gantoniu
Added support for alignment of dynamic allocated data. Thread stacks are now
guaranteed to be aligned with respect to THREAD_SLOT_SIZE whatever the
isoaddress page distribution may be.

Revision 1.1.4.1  2000/06/07 09:19:33  gantoniu
Merging new dsm with current PM2 : first try.

Revision 1.1.2.1  2000/05/31 17:37:56  gantoniu
added some files

Revision 1.1.2.1  2000/04/10 18:53:56  gantoniu
Added modules isoaddr + slot_alloc
______________________________________________________________________________
*/

#include "marcel.h"
#include "dsm_slot_alloc.h"
#include "isoaddr.h"
#include "dsm_page_manager.h"
#include "dsm_page_size.h"

//#define DEBUG_SLOT

void *dsm_slot_alloc(size_t size, size_t *granted_size, void *addr, isoaddr_attr_t *attr)
{
  int i, base;
  size_t gsize;
  void *ptr;

#ifdef DEBUG_SLOT
  fprintf(stderr, "-----> dsm_slot_alloc called: size = %d\n", size);
#endif
  if (attr->align == 0)
    attr->align = 1;
  ptr = isoaddr_malloc(size, &gsize, addr, attr);
#ifdef DEBUG_SLOT
  fprintf(stderr, "-----> isomalloc_addr returned: ptr = %p, gsize = %d\n", ptr, gsize);
#endif
  if (!ptr)
    RAISE(STORAGE_ERROR);
  
  base = dsm_isoaddr_page_index(isoaddr_page_index(ptr));

  if (attr->atomic)
    dsm_enable_page_entry(base, dsm_self(), attr->protocol, ptr, gsize, TRUE);
  else
    /* Add an entry in the page table for each alloacted page. */
    for (i = 0; i < gsize/DSM_PAGE_SIZE; i++)
      dsm_enable_page_entry(base - i, dsm_self(), attr->protocol, (char *)ptr + i*DSM_PAGE_SIZE, DSM_PAGE_SIZE, TRUE);
  
  if (granted_size != NULL)
    *granted_size = gsize;
  
  return ptr;
}

void dsm_slot_free(void *addr)
{
}
