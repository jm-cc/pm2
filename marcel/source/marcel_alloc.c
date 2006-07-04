
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
#ifndef __MINGW32__
#include <sys/mman.h>
#endif
#include <fcntl.h>

static void *next_slot;
static ma_spinlock_t next_slot_lock = MA_SPIN_LOCK_UNLOCKED;

#if defined(SOLARIS_SYS) || defined(IRIX_SYS) || defined(FREEBSD_SYS) || defined(DARWIN_SYS)
static int __zero_fd;
#endif

ma_allocator_t *marcel_mapped_slot_allocator, *marcel_unmapped_slot_allocator;

static void *unmapped_slot_alloc(void *foo)
{
	void *ptr;

	ma_spin_lock(&next_slot_lock);
#ifdef SLOT_AREA_BOTTOM
	if ((unsigned long)next_slot == SLOT_AREA_BOTTOM) {
		ma_spin_unlock(&next_slot_lock);
		return NULL;
	}
#endif
	ptr = next_slot -= 
#if defined(PM2VALGRIND) || defined(PM2STACKSGUARD)
		2*
#endif
		THREAD_SLOT_SIZE;
	ma_spin_unlock(&next_slot_lock);
	return ptr;
}

static void *mapped_slot_alloc(void *foo)
{
	void *ptr;
	void *res;
	int nb_try_left=1000;


retry:
	ptr = ma_obj_alloc(marcel_unmapped_slot_allocator);
	if (!ptr) {
		if (!ma_in_atomic() && nb_try_left--) {
			/* On tente de faire avancer les autres
			 * threads */
			mdebugl(PM2DEBUG_DISPLEVEL,
				"Trying to wait for mmap.\n");
			marcel_yield();
			goto retry;
		}
	}
	res = mmap(ptr,
		   THREAD_SLOT_SIZE,
		   PROT_READ | PROT_WRITE | PROT_EXEC,
		   MMAP_MASK,
		   FILE_TO_MAP, 0);
	/* TODO: m�moriser l'id et effectuer des VALGRIND_STACK_DEREGISTER sur munmap() */
	VALGRIND_STACK_REGISTER(next_slot, next_slot + THREAD_SLOT_SIZE);

	if(res == MAP_FAILED) {
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
		MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);
	}

	MA_BUG_ON(res != ptr);
	return ptr;
}

static void mapped_slot_free(void *slot, void *foo)
{
	if (munmap(slot, THREAD_SLOT_SIZE) == -1)
		MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);
	ma_obj_free(marcel_unmapped_slot_allocator, slot);
}

#undef marcel_slot_alloc
void *marcel_slot_alloc(void)
{
	return ma_obj_alloc(marcel_mapped_slot_allocator);
}

#undef marcel_slot_free
void marcel_slot_free(void *addr)
{
	ma_obj_free(marcel_mapped_slot_allocator, addr);
}

static void __marcel_init marcel_slot_init(void)
{
	LOG_IN();
#if defined(SOLARIS_SYS) || defined(IRIX_SYS) || defined(FREEBSD_SYS) || defined(DARWIN_SYS)
	__zero_fd = open("/dev/zero", O_RDWR);
#endif
	
	next_slot = (void *)SLOT_AREA_TOP;
	/* SLOT_AREA_TOP doit être un multiple de THREAD_SLOT_SIZE
	 * et THREAD_SLOT_SIZE doit être une puissance de deux (non vérifié
	 * ici)
	 */
	MA_BUG_ON(0 != (SLOT_AREA_TOP & (THREAD_SLOT_SIZE-1)));

	marcel_unmapped_slot_allocator = ma_new_obj_allocator(1,
			unmapped_slot_alloc, NULL, NULL, NULL,
			POLICY_HIERARCHICAL, 0);
	marcel_mapped_slot_allocator = ma_new_obj_allocator(0,
			mapped_slot_alloc, NULL, mapped_slot_free, NULL,
			POLICY_HIERARCHICAL, 0);
	LOG_OUT();
}

__ma_initfunc_prio(marcel_slot_init, MA_INIT_SLOT, MA_INIT_SLOT_PRIO, "Initialise memory slot system");

void marcel_slot_exit(void)
{
	ma_obj_allocator_fini(marcel_mapped_slot_allocator);
	ma_obj_allocator_fini(marcel_unmapped_slot_allocator);
}
