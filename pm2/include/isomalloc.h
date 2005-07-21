
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#ifndef ISOMALLOC_IS_DEF
#define ISOMALLOC_IS_DEF


/*#define DEBUG*/

#include <sys/types.h>

#include "slot_alloc.h"
#include "tbx_compiler.h"

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

TBX_FMALLOC
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
