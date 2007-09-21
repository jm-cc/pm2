
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

#section marcel_variables
extern ma_allocator_t *marcel_mapped_slot_allocator, *marcel_unmapped_slot_allocator;
#ifdef MA__PROVIDE_TLS
extern ma_allocator_t *marcel_tls_slot_allocator;
#endif
extern ma_allocator_t *marcel_thread_seed_allocator;

#section functions
#depend "tbx_compiler.h"

TBX_FMALLOC void *marcel_slot_alloc(void);
TBX_FMALLOC void *marcel_tls_slot_alloc(void);
void marcel_slot_free(void *addr);
void marcel_tls_slot_free(void *addr);
void marcel_slot_exit(void);

#section marcel_macros
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

#section marcel_functions
static __tbx_inline__ void ma_free_task_stack(marcel_t task);
#section marcel_inline
static __tbx_inline__ void ma_free_task_stack(marcel_t task) {
	switch (task->stack_kind) {
	case MA_DYNAMIC_STACK:
		marcel_tls_slot_free(marcel_stackbase(task));
		break;
	case MA_STATIC_STACK:
		break;
	case MA_NO_STACK:
		ma_obj_free(marcel_thread_seed_allocator, task);
		break;
	}
}

/* ======= MT-Safe functions from standard library ======= */

#section macros
#define tmalloc(size)          marcel_malloc(size, __FILE__, __LINE__)
#define trealloc(ptr, size)    marcel_realloc(ptr, size, __FILE__, __LINE__)
#define tcalloc(nelem, elsize) marcel_calloc(nelem, elsize, __FILE__, __LINE__)
#define tfree(ptr)             marcel_free(ptr, __FILE__, __LINE__)

#section functions
TBX_FMALLOC void *marcel_malloc(unsigned size, char *file, unsigned line);
TBX_FMALLOC void *marcel_realloc(void *ptr, unsigned size, char * __restrict file, unsigned line);
TBX_FMALLOC void *marcel_calloc(unsigned nelem, unsigned elsize, char *file, unsigned line);
void marcel_free(void *ptr, char * __restrict file, unsigned line);

TBX_FMALLOC void *__marcel_malloc(unsigned size);
TBX_FMALLOC void *__marcel_realloc(void *ptr, unsigned size);
TBX_FMALLOC void *__marcel_calloc(unsigned nelem, unsigned elsize);
void __marcel_free(void *ptr);

void ma_memory_attach(marcel_entity_t *e, void *data, size_t size, int level);
void ma_memory_detach(marcel_entity_t *e, void *data, int level);
#define marcel_task_memory_attach(t,d,s,l) ma_memory_attach(t?&((marcel_t)t)->sched.internal.entity:NULL, (d), (s), (l))
#define marcel_bubble_memory_attach(t,d,s,l) ma_memory_attach(&(b)->sched, (d), (s), (l))
