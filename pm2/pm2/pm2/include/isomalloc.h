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
$Log: isomalloc.h,v $
Revision 1.3  2000/07/14 16:17:04  gantoniu
Merged with branch dsm3

Revision 1.2.10.1  2000/06/13 16:44:05  gantoniu
New dsm branch.

Revision 1.2.8.1  2000/06/07 09:19:35  gantoniu
Merging new dsm with current PM2 : first try.

Revision 1.2.4.1  2000/04/10 18:50:42  gantoniu
Ajout couche isoaddr + integration d'une table generale des pages
(isomalloc + dsm), avec les fonctions de gestion associees.

Tests ok avec les examples isomalloc.

Revision 1.2  2000/01/31 15:49:34  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

#ifndef ISOMALLOC_IS_DEF
#define ISOMALLOC_IS_DEF


/*#define DEBUG*/

#include <sys/types.h>

#include "slot_alloc.h"

/*
typedef struct _slot_header_t {
  size_t size;
  int magic_number;
  struct _slot_header_t *next;
  struct _slot_header_t *prev;
} slot_header_t;


typedef struct _slot_descr {
  slot_header_t *slots;
  int magic_number;
  slot_header_t *last_slot;
} slot_descr_t;


void slot_slot_busy(void *addr);

void slot_list_busy(slot_descr_t *descr);

void slot_init(unsigned myrank, unsigned confsize);

void slot_init_list(slot_descr_t *descr);

void slot_flush_list(slot_descr_t *descr);

void *slot_general_alloc(slot_descr_t *descr, size_t req_size, size_t *granted_size, void *addr);

void slot_free(slot_descr_t *descr, void * addr);

void slot_exit();

void slot_print_list(slot_descr_t *descr);

void slot_print_header(slot_header_t *ptr);

void slot_pack_all(slot_descr_t *descr);

void slot_unpack_all();

#define ALIGN_UNIT 32

#define ALIGN(x, unit) ((((unsigned long)x) + ((unit) - 1)) & ~((unit) - 1))

#define SLOT_HEADER_SIZE (ALIGN(sizeof(slot_header_t), ALIGN_UNIT))

typedef void (*pm2_isomalloc_config_hook)(int *arg);

void pm2_set_isomalloc_config_func(pm2_isomalloc_config_hook f, int *arg);

void pm2_set_uniform_slot_distribution(unsigned int nb_contiguous_slots, int nb_cycles);

void pm2_set_non_uniform_slot_distribution(int node, unsigned int offset, unsigned int nb_contiguous_slots, unsigned int period, int nb_cycles);

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

*/
#endif
