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
$Log: isoaddr.h,v $
Revision 1.2  2000/07/14 16:17:04  gantoniu
Merged with branch dsm3

Revision 1.1.6.1  2000/06/13 16:44:04  gantoniu
New dsm branch.

Revision 1.1.4.1  2000/06/07 09:19:34  gantoniu
Merging new dsm with current PM2 : first try.

Revision 1.1.2.6  2000/05/10 19:38:01  gantoniu
- Implemented pm2_malloc option "ISO_DEFAULT" (= private which may become
shared); ISO_PRIVATE data can no longare be shared;

- Added an attribute to all allocation functions, to pass status, protocol
and atomic info;

- Attributes are stores in the slot header (exception: status);

- Allocation of non shared data is always atomic; when it becomes shared,
  atomicity is set according to the init attribute;

- Added TRACE options.

- Modified block_merge_lists to update the ptr to the thread slot descr.

Revision 1.1.2.5  2000/05/05 14:06:22  gantoniu
Added support for status change (PRIVATE -> SHARED) and for the option
ATOMIC_SLOT:
- added a 'master' field in the page info table, set when allocating
  pages (including on negociation);
- new attribute 'atomic' for pm2_malloc; always true for private data;
  for shared data it is user-specified; multi-page objects are processed as
  as single page in consistency actions when this option is used;
- when changing a private slot to shared, all pages in the possibly
  multi-page slot change their status;
- on migration, slots change owner;
- tests without migration, with single-slot allocation data ok;

Revision 1.1.2.4  2000/05/02 17:37:29  gantoniu
Transformed all negociation comm primitives so as to
use raw RPCs.

Revision 1.1.2.3  2000/04/30 18:54:03  gantoniu
Extended negociation procedure to allow multi-slot data allocation.
Negociation ok (old RPCs).

Revision 1.1.2.2  2000/04/11 18:13:58  gantoniu
Modified dsm page table to use pointer to structures instead of structures.
Tests ok.

Revision 1.1.2.1  2000/04/10 18:53:52  gantoniu
Added modules isoaddr + slot_alloc

______________________________________________________________________________
*/

#ifndef ISOADDR_IS_DEF
#define ISOADDR_IS_DEF

#include <sys/types.h>
#include "sys/isoaddr_const.h"

#define ALIGN_UNIT 32
#define ALIGN(x, unit) ((((unsigned long)x) + ((unit) - 1)) & ~((unit) - 1))
#define ISOMALLOC_USE_MACROS

#define THREAD_STACK_SIZE 0

/*#define DEBUG*/

void isoaddr_init(unsigned myrank, unsigned confsize);

void isoaddr_init_rpc(void);

void isoaddr_exit();

int isoaddr_addr(void *addr);

int isoaddr_page_index(void *addr);

void *isoaddr_page_addr(int index);

void *isoaddr_malloc(size_t size, size_t *granted_size, void *addr, isoaddr_attr_t *attr);

void isoaddr_free(void *addr, size_t size);

void isoaddr_add_busy_slot(void *addr);

void isoaddr_set_area_size(int pages);

void isoaddr_set_distribution(int mode, ...);

/********************************************************************************/

/* page info:  */

void isoaddr_page_set_owner(int index, node_t owner);

node_t isoaddr_page_get_owner(int index);

#define ISO_UNUSED 0
#define ISO_SHARED 1
#define ISO_PRIVATE 2
#define ISO_DEFAULT 3
#define ISO_AVAILABLE 4

void isoaddr_page_set_status(int index, int status);

int isoaddr_page_get_status(int index);

void isoaddr_page_set_master(int index, int master);

int isoaddr_page_get_master(int index);

void isoaddr_page_info_unlock(int index);

void isoaddr_page_info_unlock(int index);

#define CENTRALIZED 0
#define BLOCK       1
#define CYCLIC      2
#define CUSTOM      3

void isoaddr_page_set_distribution(int mode, ...);

void isoaddr_page_display_info();

#endif
