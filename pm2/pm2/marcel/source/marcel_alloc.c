
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
#include "marcel_alloc.h"

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
	void* stacks[];
};

static struct cache_head *stack_cache_mapped = NULL;
static struct cache_head *stack_cache_unmapped = NULL;

static unsigned stack_in_use=0;

void marcel_slot_init(void)
{
#if defined(SOLARIS_SYS) || defined(IRIX_SYS) || defined(FREEBSD_SYS) || defined(DARWIN_SYS)
  __zero_fd = open("/dev/zero", O_RDWR);
#endif
  stack_cache_mapped = NULL;
  stack_cache_unmapped = NULL;

  next_slot = (void *)SLOT_AREA_TOP;
}

extern volatile unsigned long threads_created_in_cache;

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
	int main_slot=0;

	marcel_lock_acquire(&alloc_lock);

	if(NULL != (ptr=slot_cache_get(&stack_cache_mapped, NULL))) {
		threads_created_in_cache++;
	} else {
		if(NULL == (ptr=slot_cache_get(&stack_cache_unmapped, &main_slot))) {
			next_slot -= THREAD_SLOT_SIZE;
#ifdef SLOT_AREA_BOTTOM
			if ((unsigned long)next_slot < SLOT_AREA_BOTTOM) {
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

			if(ptr == MAP_FAILED) {
				perror("mmap");
				RAISE(CONSTRAINT_ERROR);
			}
		}
	}
	stack_in_use++;
	marcel_lock_release(&alloc_lock);

	return ptr;
}

void marcel_slot_free(void *addr)
{
	marcel_lock_acquire(&alloc_lock);

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
	
}
