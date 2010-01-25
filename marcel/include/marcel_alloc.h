/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 the PM2 team (see AUTHORS file)
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


#ifndef __MARCEL_ALLOC_H__
#define __MARCEL_ALLOC_H__


#include <string.h>
#include <unistd.h>
#include "tbx_compiler.h"
#include "sys/marcel_flags.h"
#include "marcel_config.h"
#include "marcel_types.h"
#include "marcel_allocator.h"
#ifdef MM_HEAP_ENABLED
#include "scheduler/marcel_sched_types.h"
#include "marcel_spin.h"
#include "mm_heap_alloc.h"
#include "mm_heap_numa_alloc.h"
#endif


/** Public macros **/
#define tmalloc(size)          ma_malloc_nonuma(size, __FILE__, __LINE__)
#define trealloc(ptr, size)    marcel_realloc(ptr, size, __FILE__, __LINE__)
#define tcalloc(nelem, elsize) marcel_calloc(nelem, elsize, __FILE__, __LINE__)
#define tfree(data)            ma_free_nonuma(data, __FILE__, __LINE__)
/* access type weight */
#define HW 1000
#define MW 100
#define LW 10
#define MA_CACHE_SIZE 16384
#define LOCAL_NODE 1


/** Public data types **/
#ifdef MA__NUMA
typedef struct nodtab ma_nodtab_t;
//typedef struct pageinfo ma_pinfo_t;
#endif


/** Public data structures **/
#ifdef MA__NUMA
struct nodtab {
		size_t array[MARCEL_NBMAXNODES];
};
#endif


/** Public functions **/
TBX_FMALLOC void *marcel_slot_alloc(void);
TBX_FMALLOC void *marcel_tls_slot_alloc(void);
void marcel_slot_free(void *addr);
void marcel_tls_slot_free(void *addr);
void marcel_slot_exit(void);

#ifdef MM_HEAP_ENABLED
/* heap allocator */
TBX_FMALLOC void* marcel_malloc_customized(size_t size, enum heap_pinfo_weight weight, int local, int node, int level);
void marcel_free_customized(void *data);

enum heap_pinfo_weight ma_mem_access(enum heap_pinfo_weight access, int size);

int ma_bubble_memory_affinity(marcel_bubble_t *bubble);
int ma_entity_memory_volume(marcel_entity_t *entity, int recurse);

void ma_attraction_inentity(marcel_entity_t *entity, ma_nodtab_t *allocated, int weight_coef, enum heap_pinfo_weight access_min);
int ma_most_attractive_node(marcel_entity_t *entity, ma_nodtab_t *allocated, int weight_coef, enum heap_pinfo_weight access_min, int recurse);
int ma_compute_total_attraction(marcel_entity_t *entity, int weight_coef, int access_min, int attraction, int *pnode);
void ma_move_entity_alldata(marcel_entity_t *entity, int newnode);

void marcel_see_allocated_memory(marcel_entity_t *entity);
void heap_pinfo_init(heap_pinfo_t *pinfo, enum heap_mem_policy policy, int node_mask, enum heap_pinfo_weight type);
int heap_pinfo_isok(heap_pinfo_t *pinfo, enum heap_mem_policy policy, int node_mask, enum heap_pinfo_weight type);
#endif /* MM_HEAP_ENABLED */

/* malloc */
TBX_FMALLOC void* marcel_malloc(size_t size, const char *file, unsigned line);
TBX_FMALLOC void *marcel_realloc(void *ptr, unsigned size, const char * __restrict file, unsigned line);
TBX_FMALLOC void *marcel_calloc(unsigned nelem, unsigned elsize, const char *file, unsigned line);
void marcel_free(void *data);

TBX_FMALLOC void *__marcel_malloc(unsigned size);
TBX_FMALLOC void *__marcel_realloc(void *ptr, unsigned size);
TBX_FMALLOC void *__marcel_calloc(unsigned nelem, unsigned elsize);
void __marcel_free(void *ptr);

/* malloc internes */
TBX_FMALLOC void* ma_malloc(size_t size, const char * file, unsigned line);
void ma_free(void *data, const char * file, unsigned line);

/* ancien malloc pour archi nonnuma */
TBX_FMALLOC void* ma_malloc_nonuma(size_t size, const char *file, unsigned line);
void ma_free_nonuma(void *data, const char * __restrict file, unsigned line);


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
#ifdef MA__PROVIDE_TLS
#  define marcel_tls_slot_alloc() ma_obj_alloc(marcel_tls_slot_allocator)
#  define marcel_tls_slot_free(addr) ma_obj_free(marcel_tls_slot_allocator, addr)
#else
#  define marcel_tls_slot_alloc marcel_slot_alloc
#  define marcel_tls_slot_free marcel_slot_free
#endif
#define marcel_slot_alloc() ma_obj_alloc(marcel_mapped_slot_allocator)
#define marcel_slot_free(addr) ma_obj_free(marcel_mapped_slot_allocator, addr)

#define ma_slot_task(slot) ((marcel_task_t *)((unsigned long) (slot) + THREAD_SLOT_SIZE - MAL(sizeof(marcel_task_t))))
#define ma_slot_top_task(top) ((marcel_task_t *)((unsigned long) (top) - MAL(sizeof(marcel_task_t))))
#define ma_slot_sp_task(sp) ma_slot_task((sp) & ~(THREAD_SLOT_SIZE-1))
#define ma_task_slot_top(t) ((unsigned long) (t) + MAL(sizeof(marcel_task_t)))


/** Internal global variables **/
extern ma_allocator_t *marcel_mapped_slot_allocator, *marcel_unmapped_slot_allocator;
#ifdef MA__PROVIDE_TLS
extern ma_allocator_t *marcel_tls_slot_allocator;
void marcel_tls_attach(marcel_t t);
void marcel_tls_detach(marcel_t t);
#else
#define marcel_tls_attach(t) ((void) 0)
#define marcel_tls_detach(t) ((void) 0)
#endif
extern ma_allocator_t *marcel_thread_seed_allocator;


/** Internal functions **/
static __tbx_inline__ void ma_free_task_stack(marcel_t task);


#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_ALLOC_H__ **/
