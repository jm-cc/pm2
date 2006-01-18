
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

#include "marcel.h"

#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

static void *next_slot;

static marcel_lock_t alloc_lock = MARCEL_LOCK_INIT;

#if defined(SOLARIS_SYS) || defined(IRIX_SYS) || defined(FREEBSD_SYS) || defined(DARWIN_SYS)
static int __zero_fd;
#endif

struct cache_head {
	struct cache_head *prev;
	unsigned nb_max_cached;
	unsigned nb_cached;
	void* stacks[1]; /* en fait plus, mais stacks[] accepte uniquement dans C99 (donc pas gcc-2.95) */
};

static struct cache_head *stack_cache_mapped = NULL;
static struct cache_head *stack_cache_unmapped = NULL;

static unsigned stack_in_use=0;

static void __marcel_init marcel_slot_init(void)
{
	LOG_IN();
#if defined(SOLARIS_SYS) || defined(IRIX_SYS) || defined(FREEBSD_SYS) || defined(DARWIN_SYS)
	__zero_fd = open("/dev/zero", O_RDWR);
#endif
	stack_cache_mapped = NULL;
	stack_cache_unmapped = NULL;
	
	next_slot = (void *)SLOT_AREA_TOP;
	/* SLOT_AREA_TOP doit être un multiple de THREAD_SLOT_SIZE
	 * et THREAD_SLOT_SIZE doit être une puissance de deux (non vérifié
	 * ici)
	 */
	MA_BUG_ON(0 != (SLOT_AREA_TOP & (THREAD_SLOT_SIZE-1)));

	LOG_OUT();
}

__ma_initfunc_prio(marcel_slot_init, MA_INIT_SLOT, MA_INIT_SLOT_PRIO,
		   "Initialise memory slot system");

static volatile unsigned long threads_created_in_cache;

inline static void* slot_cache_get(struct cache_head **head, int *main_slot)
{
	struct cache_head *cache=*head;
	register void *ptr;

	if (!cache) {
		return NULL;
	}
	if (cache->nb_cached) {
		ptr=cache->stacks[--cache->nb_cached];
	} else {
		if (main_slot) {
			*main_slot=1;
		}
		ptr=cache;
		*head=cache->prev;
	}
	return ptr;

}

inline static int slot_cache_put(void* ptr, struct cache_head *cache)
{
	if (!cache || cache->nb_cached >= cache->nb_max_cached) {
		return 0;
	}
	cache->stacks[cache->nb_cached++]=ptr;
	return 1;
}

inline static void slot_cache_init(struct cache_head *cache, 
				   struct cache_head *old)
{
	static int max_cached=
		(THREAD_SLOT_SIZE - sizeof(struct cache_head))/sizeof(void*);
	cache->prev=old;
	cache->nb_max_cached=max_cached;
	cache->nb_cached=0;
}

void *marcel_slot_alloc(void)
{
	register void *ptr;
	int main_slot;
	int nb_try_left=1000;

	LOG_IN();

retry:
	main_slot=0;
	marcel_lock_acquire(&alloc_lock);

	if(NULL != (ptr=slot_cache_get(&stack_cache_mapped, NULL))) {
		threads_created_in_cache++;
	} else {
		if(NULL == (ptr=slot_cache_get(&stack_cache_unmapped, &main_slot))) {
			next_slot -= 
#if defined(PM2VALGRIND) || defined(PM2STACKSGUARD)
				2*
#endif
				THREAD_SLOT_SIZE;
#ifdef SLOT_AREA_BOTTOM
			if ((unsigned long)next_slot < SLOT_AREA_BOTTOM) {
				marcel_lock_release(&alloc_lock);
				if (!ma_in_atomic() && nb_try_left--) {
					/* On tente de faire avancer les autres
					 * threads */
					mdebugl(PM2DEBUG_DISPLEVEL,
						"Trying to for slot\n");
					marcel_yield();
					goto retry;
				}
				RAISE(STORAGE_ERROR);
			}
#endif
			ptr=next_slot;
		}
		if (main_slot) {
			threads_created_in_cache++;
		} else {
			ptr = mmap(next_slot,
				   THREAD_SLOT_SIZE,
				   PROT_READ | PROT_WRITE | PROT_EXEC,
				   MMAP_MASK,
				   FILE_TO_MAP, 0);
			/* TODO: m�moriser l'id et effectuer des VALGRIND_STACK_DEREGISTER sur munmap() */
			VALGRIND_STACK_REGISTER(next_slot, next_slot + THREAD_SLOT_SIZE);

			if(ptr == MAP_FAILED) {
				marcel_lock_release(&alloc_lock);
				if (!ma_in_atomic() && nb_try_left--) {
					/* On tente de faire avancer les autres
					 * threads */
					mdebugl(PM2DEBUG_DISPLEVEL,
						"Trying to wait for mmap. Current: mmap(%p, %lx, ...)\n", next_slot, THREAD_SLOT_SIZE);
					marcel_yield();
					goto retry;
				}
				perror("mmap");
				fprintf(stderr,"args %p, %lx, %u, %d", 
						next_slot, THREAD_SLOT_SIZE,
						MMAP_MASK, FILE_TO_MAP);
				RAISE(CONSTRAINT_ERROR);
			}
		}
	}
	stack_in_use++;
	marcel_lock_release(&alloc_lock);

	mdebug("Allocating slot %p\n", ptr);

	LOG_OUT();

	return ptr;
}

void marcel_slot_free(void *addr)
{
	LOG_IN();

#ifndef PM2_VALGRIND
	marcel_lock_acquire(&alloc_lock);

	mdebug("Desallocating slot %p\n", addr);

	if(!stack_cache_mapped) {
		slot_cache_init(addr, NULL);
		stack_cache_mapped=addr;
	} else if(!slot_cache_put(addr, stack_cache_mapped)) {
		if(slot_cache_put(addr, stack_cache_unmapped)) {
			if(munmap(addr, THREAD_SLOT_SIZE) == -1)
				RAISE(CONSTRAINT_ERROR);
		} else {
			slot_cache_init(addr, stack_cache_unmapped);
			stack_cache_unmapped=addr;
		}
	}
	stack_in_use--;
	marcel_lock_release(&alloc_lock);
#endif
	LOG_OUT();
}

void marcel_slot_exit(void)
{
	register void *ptr;
        int main_slot=0;
	while((ptr=slot_cache_get(&stack_cache_mapped,NULL))!=NULL) {
		if(munmap(ptr, THREAD_SLOT_SIZE) == -1)
			RAISE(CONSTRAINT_ERROR);
	}
	while((ptr=slot_cache_get(&stack_cache_mapped,&main_slot))!=NULL) {
		if (main_slot) {
			if(munmap(ptr, THREAD_SLOT_SIZE) == -1)
				RAISE(CONSTRAINT_ERROR);
			main_slot=0;
		}
	}
	mdebug("threads created in cache: %ld\n", threads_created_in_cache);
}
