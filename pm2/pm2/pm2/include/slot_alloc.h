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
$Log: slot_alloc.h,v $
Revision 1.2  2000/07/14 16:17:08  gantoniu
Merged with branch dsm3

Revision 1.1.6.1  2000/06/13 16:44:06  gantoniu
New dsm branch.

Revision 1.1.4.1  2000/06/07 09:19:36  gantoniu
Merging new dsm with current PM2 : first try.

Revision 1.1.2.4  2000/05/10 19:38:02  gantoniu
- Implemented pm2_malloc option "ISO_DEFAULT" (= private which may become
shared); ISO_PRIVATE data can no longare be shared;

- Added an attribute to all allocation functions, to pass status, protocol
and atomic info;

- Attributes are stores in the slot header (exception: status);

- Allocation of non shared data is always atomic; when it becomes shared,
  atomicity is set according to the init attribute;

- Added TRACE options.

- Modified block_merge_lists to update the ptr to the thread slot descr.

Revision 1.1.2.3  2000/05/07 17:26:18  gantoniu
Fixed some bugs in the new code for PRIVATE to SHARED transform.
Atomicity and protocol can be specified for private allocated
data that may become shared. Tests with and without negociation ok.

Revision 1.1.2.2  2000/05/05 14:06:22  gantoniu
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

Revision 1.1.2.1  2000/04/10 18:53:53  gantoniu
Ajout modules isoaddr + slot_alloc

Revision 1.2  2000/01/31 15:49:34  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

#ifndef SLOT_ALLOC_IS_DEF
#define SLOT_ALLOC_IS_DEF

/*#define DEBUG*/

#include <sys/types.h>
#include "isoaddr.h"

typedef struct _slot_header_t {
  size_t size;
  int magic_number;
  struct _slot_header_t *next;
  struct _slot_header_t *prev;
  struct _slot_descr_t *thread_slot_descr;
  int prot;
  int atomic;
} slot_header_t;


typedef struct _slot_descr_t {
  slot_header_t *slots;
  int magic_number;
  slot_header_t *last_slot;
} slot_descr_t;


void slot_slot_busy(void *addr);

void slot_list_busy(slot_descr_t *descr);

#define slot_init(myrank, confsize) isoaddr_init(myrank, confsize)

void slot_init_list(slot_descr_t *descr);

void slot_flush_list(slot_descr_t *descr);

void *slot_general_alloc(slot_descr_t *descr, size_t req_size, size_t *granted_size, void *addr, isoaddr_attr_t *attr);

void slot_free(slot_descr_t *descr, void * addr);

#define slot_exit() isoaddr_exit()

void slot_print_list(slot_descr_t *descr);

void slot_print_header(slot_header_t *ptr);

void slot_set_shared(void *addr);

#define ALIGN_UNIT 32

#define ALIGN(x, unit) ((((unsigned long)x) + ((unit) - 1)) & ~((unit) - 1))

#define SLOT_HEADER_SIZE (ALIGN(sizeof(slot_header_t), ALIGN_UNIT))

#define ISOMALLOC_USE_MACROS

#ifdef ISOMALLOC_USE_MACROS

#define slot_get_first(d) ((d)->slots)

#define slot_get_next(s) ((s)->next)

#define slot_get_prev(s) ((s)->prev)

#define slot_get_usable_address(s) ((void *)((char *)(s) + SLOT_HEADER_SIZE))

#define slot_get_header_address(usable_address) ((slot_header_t *)((char *)(usable_address) - SLOT_HEADER_SIZE))

#define slot_get_usable_size(s) ((s)->size)

#define slot_get_overall_size(s) ((s)->size + SLOT_HEADER_SIZE)

#define slot_get_header_size() SLOT_HEADER_SIZE

#else

slot_header_t *slot_get_first(slot_descr_t *descr);

slot_header_t *slot_get_next(slot_header_t *slot_ptr);

slot_header_t *slot_get_prev(slot_header_t *slot_ptr);

void *slot_get_usable_address(slot_header_t *slot_ptr);

slot_header_t *slot_get_header_address(void *usable_address);

size_t slot_get_usable_size(slot_header_t *slot_ptr);

size_t slot_get_overall_size(slot_header_t *slot_ptr);

size_t slot_get_header_size();

#endif

#endif
