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
*/

#ifndef BLOCK_ALLOC_IS_DEF
#define BLOCK_ALLOC_IS_DEF

#include <sys/types.h>
#include <isomalloc.h>

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

void *block_alloc(block_descr_t *descr, size_t size);

void block_free(block_descr_t *descr, void * addr);

void block_flush_list(block_descr_t *descr);

void block_exit();

void block_print_list(block_descr_t *descr);

void block_pack_all(block_descr_t *descr);

void block_unpack_all();

block_descr_t *block_merge_lists(block_descr_t *src, block_descr_t *dest);

#endif

