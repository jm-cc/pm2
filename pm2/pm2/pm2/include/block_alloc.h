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
$Log: block_alloc.h,v $
Revision 1.4  2000/07/14 16:17:03  gantoniu
Merged with branch dsm3

Revision 1.3.10.1  2000/06/13 16:44:04  gantoniu
New dsm branch.

Revision 1.3.8.1  2000/06/07 09:19:34  gantoniu
Merging new dsm with current PM2 : first try.

Revision 1.3.4.3  2000/05/10 19:38:01  gantoniu
- Implemented pm2_malloc option "ISO_DEFAULT" (= private which may become
shared); ISO_PRIVATE data can no longare be shared;

- Added an attribute to all allocation functions, to pass status, protocol
and atomic info;

- Attributes are stores in the slot header (exception: status);

- Allocation of non shared data is always atomic; when it becomes shared,
  atomicity is set according to the init attribute;

- Added TRACE options.

- Modified block_merge_lists to update the ptr to the thread slot descr.

Revision 1.3.4.2  2000/05/07 17:26:17  gantoniu
Fixed some bugs in the new code for PRIVATE to SHARED transform.
Atomicity and protocol can be specified for private allocated
data that may become shared. Tests with and without negociation ok.

Revision 1.3.4.1  2000/05/05 14:06:21  gantoniu
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

Revision 1.3  2000/02/28 11:17:55  rnamyst
Changed #include <> into #include "".

Revision 1.2  2000/01/31 15:49:32  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

#ifndef BLOCK_ALLOC_IS_DEF
#define BLOCK_ALLOC_IS_DEF

#include <sys/types.h>

#include "isomalloc.h"

typedef struct _block_header_t {
  size_t size;
  int magic_number;
  struct _block_header_t *next;
  struct _block_header_t *prev;
} block_header_t;


typedef struct _block_descr_t {
  slot_descr_t slot_descr;
  int magic_number;
  block_header_t *last_block_set_free;
  slot_header_t *last_slot_used;
} block_descr_t;


void block_init(unsigned myrank, unsigned confsize);

void block_init_list(block_descr_t *descr);

void *block_alloc(block_descr_t *descr, size_t size, isoaddr_attr_t *attr);

void block_free(block_descr_t *descr, void * addr);

void block_flush_list(block_descr_t *descr);

void block_exit();

void block_print_list(block_descr_t *descr);

void block_pack_all(block_descr_t *descr, int dest);

void block_unpack_all();

block_descr_t *block_merge_lists(block_descr_t *src, block_descr_t *dest);

#endif

