
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2006 "the PM2 team" (see AUTHORS file)
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

#section common
#depend "marcel_container.h[marcel_types]"
#depend "linux_spinlock.h[types]"

#section types
typedef struct _ma_allocator_t ma_allocator_t;
#ifdef MA__LWPS
#define POLICY_EQUIV
#else
#define POLICY_EQUIV = POLICY_GLOBAL
#endif
enum ma_policy_t {
	POLICY_GLOBAL,
	POLICY_LOCAL POLICY_EQUIV,
	POLICY_HIERARCHICAL_MEMORY POLICY_EQUIV,
	POLICY_HIERARCHICAL POLICY_EQUIV,
	POLICY_BALANCE POLICY_EQUIV
};

#section marcel_structures

struct _ma_allocator_t {
	enum ma_policy_t policy;
	union {
		ma_container_t *obj;
		unsigned long offset;
	} container;
	void *(*create) (void *);
	void *create_arg;

	void (*destroy) (void *, void *);
	void *destroy_arg;

	int conservative;
	int max_size;
	int init;
};

typedef struct {
	unsigned long cur;
	ma_spinlock_t lock;
	unsigned long max;
} ma_per_sth_cur_t;

#section marcel_macros
#define MA_PER_STH_CUR_INITIALIZER(_max) { .cur = 0, .lock = MA_SPIN_LOCK_UNLOCKED, .max = (_max) }

#section functions
/* crée un allocateur distribué d'objets, créés au besoin à */
/* l'aide de create(create_arg). */
/* si conservative == 1, préserver le contenu des objets */
/* avec un choix de politiques d'allocation */

TBX_FMALLOC
ma_allocator_t *ma_new_obj_allocator(int conservative,
				      void *(*create)(void *), 
				      void *create_arg,
				      void (*destroy)(void *, void *), 
				      void *destroy_arg,
				      enum ma_policy_t policy,
				      int max_size
				      );
void ma_obj_allocator_init(ma_allocator_t * allocator);
TBX_FMALLOC void *ma_obj_alloc(ma_allocator_t *allocator);
void ma_obj_free(ma_allocator_t *allocator, void *obj);
void ma_obj_allocator_print(ma_allocator_t * allocator);
void ma_obj_allocator_fini(ma_allocator_t *allocator);

/* helpful */
TBX_FMALLOC void * ma_obj_allocator_malloc(void * arg);
void ma_obj_allocator_free(void * obj, void * foo);
TBX_FMALLOC void * __ma_obj_allocator_malloc(void * arg);
void __ma_obj_allocator_free(void * obj, void * foo);

#section marcel_functions
void ma_allocator_init(void);
void ma_allocator_exit(void);

unsigned long ma_per_sth_alloc(ma_per_sth_cur_t *cur, size_t size);
unsigned long ma_per_lwp_alloc(size_t size);
#define ma_per_lwp_alloc(size) ma_per_sth_alloc(&ma_per_lwp_cur, size)
unsigned long ma_per_level_alloc(size_t size);
#define ma_per_level_alloc(size) ma_per_sth_alloc(&ma_per_level_cur, size)
#define ma_per_sth_data(sth, off) ((void*)(&(sth)->data[off]))
#define ma_per_lwp_data(lwp, off) ma_per_sth_data(lwp, off)
#define ma_per_lwp_self_data(off) ma_per_lwp_data(LWP_SELF, off)
#define ma_per_level_data(level, off) ma_per_sth_data(level, off)

#section marcel_variables
extern ma_allocator_t * ma_node_allocator;
extern ma_per_sth_cur_t ma_per_lwp_cur, ma_per_level_cur;
