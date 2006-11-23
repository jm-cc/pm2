
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
#include "errno.h"
#include <unistd.h>
#ifndef __MINGW32__
#include <sys/mman.h>
#endif
#include <fcntl.h>
#include <errno.h>

#if defined(MA__PROVIDE_TLS) && defined(LINUX_SYS) && (defined(X86_ARCH) || defined(X86_64_ARCH))
#include <sys/syscall.h>
#include <unistd.h>
#include <asm/ldt.h>
#ifdef X86_64_ARCH
#include <asm/prctl.h>
#endif
#endif

struct memory_area {
	struct list_head list;
	void *data;
	size_t size;
};
ma_allocator_t *memory_area_allocator;
unsigned long ma_stats_memory_offset;

static void *next_slot;
static ma_spinlock_t next_slot_lock = MA_SPIN_LOCK_UNLOCKED;

#if defined(SOLARIS_SYS) || defined(IRIX_SYS) || defined(FREEBSD_SYS) || defined(DARWIN_SYS)
static int __zero_fd;
#endif

ma_allocator_t *marcel_mapped_slot_allocator, *marcel_unmapped_slot_allocator;
#if defined(MA__PROVIDE_TLS)
#if defined(X86_ARCH)
#  define libc_internal_function __attribute__((regparm (3), stdcall))
#else
#  define libc_internal_function
#endif
extern void *_dl_allocate_tls(void *) libc_internal_function;
extern void *_dl_allocate_tls_init(void *) libc_internal_function;
extern void _dl_deallocate_tls(void *, int) libc_internal_function;
extern void _dl_get_tls_static_info (size_t *sizep, size_t *alignp) libc_internal_function;
extern void _rtld_global_ro;
ma_allocator_t *marcel_tls_slot_allocator;
#if defined(X86_ARCH)
unsigned short __main_thread_desc;
#endif
#if defined(X86_64_ARCH)
unsigned long __main_thread_tls_base;
#endif
#endif

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
#if defined(PM2VALGRIND) || defined(PM2STACKSGUARD) || defined(PM2DEBUG)
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

#ifdef MA__PROVIDE_TLS
static void *tls_slot_alloc(void *foo) {
	void *ptr = ma_obj_alloc(marcel_mapped_slot_allocator);
	marcel_t t = ma_slot_task(ptr);
#if defined(LINUX_SYS)
	lpt_tcb_t *tcb = marcel_tcb(t);
	_dl_allocate_tls(tcb);

#if defined(X86_ARCH) || defined(X86_64_ARCH)
	tcb->tcb = tcb;
#endif

#if defined(X86_ARCH)
	// j'aimerais bien qu'on m'explique pourquoi la glibc stocke cela ici.....
	uintptr_t *sysinfo = &_rtld_global_ro + 
	/* gdb /usr/lib/debug/ld-linux.so.2
	 * > p (unsigned long) &_rtld_global_ro._dl_sysinfo - (unsigned long) &_rtld_global_ro; */
#if defined(X86_ARCH)
		384
#elif defined(IA64_ARCH)
		176
#endif
		;

	tcb->sysinfo = *sysinfo;
#endif

#if defined(X86_ARCH) || defined(X86_64_ARCH)
#ifdef X86_64_ARCH
	/* because else we can't use ldt */
	MA_BUG_ON((unsigned long) ptr >= (1ul<<32));
#endif
	struct user_desc desc = {
		.entry_number = (SLOT_AREA_TOP - (unsigned long) ptr) / THREAD_SLOT_SIZE - 1,
		.base_addr = (unsigned long) tcb,
		.limit = 0xfffffffful,
		.seg_32bit = 1,
		.contents = MODIFY_LDT_CONTENTS_DATA,
		.read_exec_only = 0,
		.limit_in_pages = 1,
		.seg_not_present = 0,
		.useable = 1,
	};
	int ret = syscall(SYS_modify_ldt, 0x11, &desc, sizeof(desc));
	if (ret != 0) {
		fprintf(stderr,"entry #%u address %lx failed: %s", desc.entry_number, desc.base_addr, strerror(errno));
		MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);
	}
#endif
#endif
	return ptr;
}

static void tls_slot_free(void *slot, void *foo) {
	marcel_t t = ma_slot_task(slot);
	lpt_tcb_t *tcb = marcel_tcb(t);
	_dl_deallocate_tls(tcb, 0);
	_dl_allocate_tls_init(tcb);
}
#endif /* MA__PROVIDE_TLS */

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

#ifdef MA__PROVIDE_TLS
#undef marcel_tls_slot_alloc
void *marcel_tls_slot_alloc(void)
{
	return ma_obj_alloc(marcel_tls_slot_allocator);
}

#endif

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
	memory_area_allocator = ma_new_obj_allocator(0, ma_obj_allocator_malloc,
	    (void*) (sizeof(struct memory_area)), ma_obj_allocator_free, NULL,
	    POLICY_HIERARCHICAL, 0);
	ma_stats_memory_offset = ma_stats_alloc(ma_stats_long_sum_reset, ma_stats_long_sum_synthesis, sizeof(long));

#ifdef MA__PROVIDE_TLS
	size_t static_tls_size, static_tls_align;
	_dl_get_tls_static_info(&static_tls_size, &static_tls_align);
	MA_BUG_ON(static_tls_align > MARCEL_ALIGN);
	if (static_tls_size + sizeof(lpt_tcb_t) > MA_TLS_AREA_SIZE) {
		fprintf(stderr,"Marcel has only %lu bytes for TLS while %d are needed, please increase MA_TLS_AREA_SIZE. Aborting.\n", MA_TLS_AREA_SIZE, static_tls_size + sizeof(lpt_tcb_t));
		abort();
	}
#ifdef X86_ARCH
	asm volatile ("movw %%gs, %w0" : "=q" (__main_thread_desc));
#elif defined(X86_64_ARCH)
	arch_prctl(ARCH_GET_FS, &__main_thread_tls_base);
#endif
	marcel_tls_slot_allocator = ma_new_obj_allocator(0,
			tls_slot_alloc, NULL, tls_slot_free, NULL,
			POLICY_HIERARCHICAL, 0);
#endif
	LOG_OUT();
}

__ma_initfunc_prio(marcel_slot_init, MA_INIT_SLOT, MA_INIT_SLOT_PRIO, "Initialise memory slot system");

void marcel_slot_exit(void)
{
#ifdef MA__PROVIDE_TLS
	ma_obj_allocator_fini(marcel_tls_slot_allocator);
#endif
	ma_obj_allocator_fini(marcel_mapped_slot_allocator);
	ma_obj_allocator_fini(marcel_unmapped_slot_allocator);
}

/* marcel_malloc, marcel_calloc, marcel_free:
   avoid locking penalty on trivial requests */
void *marcel_malloc(unsigned size, char *file, unsigned line)
{
        void *p;

        if (size) {
		marcel_extlib_protect();
                p = __TBX_MALLOC(size, file, line);
		marcel_extlib_unprotect();
                if(p == NULL)
                        MARCEL_EXCEPTION_RAISE(MARCEL_STORAGE_ERROR);
        } else {
                return NULL;
        }

        return p;
}

void *marcel_realloc(void *ptr, unsigned size, char * __restrict file, unsigned line)
{
        void *p;

	marcel_extlib_protect();
        p = __TBX_REALLOC(ptr, size, file, line);
	marcel_extlib_unprotect();
        if(p == NULL)
                MARCEL_EXCEPTION_RAISE(MARCEL_STORAGE_ERROR);

        return p;
}

void *marcel_calloc(unsigned nelem, unsigned elsize, char *file, unsigned line)
{
        void *p;

        if (nelem && elsize) {
		marcel_extlib_protect();
                p = __TBX_CALLOC(nelem, elsize, file, line);
		marcel_extlib_unprotect();
                if(p == NULL)
                        MARCEL_EXCEPTION_RAISE(MARCEL_STORAGE_ERROR);
        } else {
                return NULL;
        }

        return p;
}

void marcel_free(void *ptr, char * __restrict file, unsigned line)
{
        if(ptr) {
		marcel_extlib_protect();
                __TBX_FREE((char *)ptr, file, line);
		marcel_extlib_unprotect();
        }
}

void ma_memory_attach(marcel_entity_t *e, void *data, size_t size, int level)
{
	struct memory_area *area;
	ma_holder_t *h;
	if (!e)
		e = &MARCEL_SELF->sched.internal.entity;
	while (level--) {
		h = e->init_holder;
		MA_BUG_ON(h->type == MA_RUNQUEUE_HOLDER);
		e = &ma_bubble_holder(h)->sched;
	}
	area = ma_obj_alloc(memory_area_allocator);
	area->size = size;
	area->data = data;
	ma_spin_lock(&e->memory_areas_lock);
	list_add(&area->list, &e->memory_areas);
	ma_spin_unlock(&e->memory_areas_lock);
	*(long*)ma_stats_get(e, ma_stats_memory_offset) += size;
}

void ma_memory_detach(marcel_entity_t *e, void *data, int level)
{
	struct memory_area *area;
	ma_holder_t *h;
	if (!e)
		e = &MARCEL_SELF->sched.internal.entity;
	while (level--) {
		h = e->init_holder;
		MA_BUG_ON(h->type == MA_RUNQUEUE_HOLDER);
		e = &ma_bubble_holder(h)->sched;
	}
	ma_spin_lock(&e->memory_areas_lock);
	list_for_each_entry(area, &e->memory_areas, list)
		if (area->data == data) {
			data = NULL;
			list_del(&area->list);
			break;
		}
	MA_BUG_ON(data);
	ma_spin_unlock(&e->memory_areas_lock);
	*(long*)ma_stats_get(e, ma_stats_memory_offset) -= area->size;
	ma_obj_free(memory_area_allocator, area);
}
