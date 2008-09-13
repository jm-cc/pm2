
/*
 * PM2 Parallel Multithreaded Machine
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
#ifdef MA__NUMA
#include <numaif.h>
#include <numa.h>
#endif

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
ma_allocator_t *heapinfo_allocator;
unsigned long ma_stats_memory_offset;

static void *next_slot;
static ma_spinlock_t next_slot_lock = MA_SPIN_LOCK_UNLOCKED;

#if defined(SOLARIS_SYS) || defined(IRIX_SYS) || defined(FREEBSD_SYS) || defined(DARWIN_SYS)
static int __zero_fd;
#endif

ma_allocator_t *marcel_mapped_slot_allocator, *marcel_unmapped_slot_allocator;
ma_allocator_t *marcel_thread_seed_allocator;

#if defined(MA__PROVIDE_TLS)
#if defined(X86_ARCH)
#  define libc_internal_function __attribute__((regparm (3), stdcall))
#else
#  define libc_internal_function
#endif
extern void *_dl_allocate_tls(void *) libc_internal_function;
extern void _dl_deallocate_tls(void *, int) libc_internal_function;
extern void _dl_get_tls_static_info (size_t *sizep, size_t *alignp) libc_internal_function;
extern void _rtld_global_ro;
ma_allocator_t *marcel_tls_slot_allocator;
#if defined(X86_ARCH)
unsigned short __main_thread_desc;
#elif defined(X86_64_ARCH) || defined(IA64_ARCH)
unsigned long __main_thread_tls_base;
#endif
#if defined(X86_ARCH) || defined(X86_64_ARCH)
static uintptr_t sysinfo;
static unsigned long stack_guard, pointer_guard;
#endif
#endif

/* Allocate a new slot but don't map it yet */
static void *unmapped_slot_alloc(void *foo)
{
	void *ptr;

	ma_spin_lock(&next_slot_lock);
	if ((unsigned long)next_slot <= SLOT_AREA_BOTTOM) {
		ma_spin_unlock(&next_slot_lock);
		return NULL;
	}
	ptr = next_slot -= 
#if defined(PM2VALGRIND) || defined(PM2STACKSGUARD) || defined(PM2DEBUG)
		2*
#endif
		THREAD_SLOT_SIZE;
	ma_spin_unlock(&next_slot_lock);
	return ptr;
}

/* Map a new slot */
static void *mapped_slot_alloc(void *foo)
{
	void *ptr;
	void *res;
	int nb_try_left=1000;


  retry:
	ptr = ma_obj_alloc(marcel_unmapped_slot_allocator);
	if (!ptr) {
		if (!ma_in_atomic() && nb_try_left--) {
			mdebugl(PM2DEBUG_DISPLEVEL, "Not enough room for new stack (stack size is %lx, stack allocation area is %lx-%lx), trying to wait for other threads to terminate.\n", THREAD_SLOT_SIZE, (unsigned long) SLOT_AREA_BOTTOM, (unsigned long) ISOADDR_AREA_TOP);
			marcel_yield();
			goto retry;
		} else {
			/* Erf. We are in atomic section or tried too long. */
			fprintf(stderr, "Not enough room for new stack (stack size is %lx, stack allocation area is %lx-%lx).\n", THREAD_SLOT_SIZE, (unsigned long) SLOT_AREA_BOTTOM, (unsigned long) ISOADDR_AREA_TOP);
			MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);
		}
	}
	/* TODO: mémoriser l'id et effectuer des VALGRIND_STACK_DEREGISTER sur munmap() */
	VALGRIND_STACK_REGISTER(ptr, ptr + THREAD_SLOT_SIZE);
	res = mmap(ptr,
				  THREAD_SLOT_SIZE,
				  PROT_READ | PROT_WRITE | PROT_EXEC,
				  MMAP_MASK,
				  FILE_TO_MAP, 0);

	if(res == MAP_FAILED) {
		if (!ma_in_atomic() && nb_try_left--) {
			mdebugl(PM2DEBUG_DISPLEVEL, "mmap(%p, %lx, ...) for stack failed! Trying to wait for other threads to terminate\n", next_slot, THREAD_SLOT_SIZE);
			marcel_yield();
			goto retry;
		}
		perror("mmap");
		fprintf(stderr,"args %p, %lx, %u, %d", 
				  ptr, THREAD_SLOT_SIZE,
				  MMAP_MASK, FILE_TO_MAP);
		MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);
	}

	MA_BUG_ON(res != ptr);

	return ptr;
}

/* Allocate a TLS area for the slot */
#ifdef MA__PROVIDE_TLS
static void *tls_slot_alloc(void *foo) {
	void *ptr = ma_obj_alloc(marcel_mapped_slot_allocator);
	marcel_t t = ma_slot_task(ptr);
#if defined(LINUX_SYS)
	lpt_tcb_t *tcb = marcel_tcb(t);
	_dl_allocate_tls(tcb);

#if defined(X86_ARCH) || defined(X86_64_ARCH)
	tcb->tcb = tcb;
	tcb->multiple_threads = 1;
	tcb->sysinfo = sysinfo;
	tcb->stack_guard = stack_guard;
	tcb->pointer_guard = pointer_guard;
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
		fprintf(stderr,"entry #%u address %lx failed: %s", desc.entry_number, (unsigned long) desc.base_addr, strerror(errno));
		MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);
	}
#endif
#endif
	return ptr;
}

/* Free TLS area of the slot */
static void tls_slot_free(void *slot, void *foo) {
	marcel_t t = ma_slot_task(slot);
	lpt_tcb_t *tcb = marcel_tcb(t);
	_dl_deallocate_tls(tcb, 0);
	ma_obj_free(marcel_mapped_slot_allocator, slot);
}
#endif /* MA__PROVIDE_TLS */

/* Unmap the slot */
static void mapped_slot_free(void *slot, void *foo)
{
	if (munmap(slot, THREAD_SLOT_SIZE) == -1)
		MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);
	ma_obj_free(marcel_unmapped_slot_allocator, slot);
}

/* No way to free mapped slots */

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
	/* SLOT_AREA_TOP doit Ãªtre un multiple de THREAD_SLOT_SIZE
	 * et THREAD_SLOT_SIZE doit Ãªtre une puissance de deux (non vÃ©rifiÃ©
	 * ici)
	 */
	MA_BUG_ON(0 != (SLOT_AREA_TOP & (THREAD_SLOT_SIZE-1)));

	marcel_unmapped_slot_allocator = ma_new_obj_allocator(1,
			unmapped_slot_alloc, NULL, NULL, NULL,
			POLICY_HIERARCHICAL, MARCEL_THREAD_CACHE_MAX);
	marcel_mapped_slot_allocator = ma_new_obj_allocator(0,
							    mapped_slot_alloc, NULL, mapped_slot_free, NULL,
							    POLICY_HIERARCHICAL_MEMORY, MARCEL_THREAD_CACHE_MAX);
	memory_area_allocator = ma_new_obj_allocator(0, ma_obj_allocator_malloc,
						     (void*) (sizeof(struct memory_area)), ma_obj_allocator_free, NULL,
						     POLICY_HIERARCHICAL_MEMORY, 0);

#ifdef MARCEL_STATS_ENABLED
	ma_stats_memory_offset = ma_stats_alloc(ma_stats_long_sum_reset, ma_stats_long_sum_synthesis, sizeof(long));
#endif /* MARCEL_STATS_ENABLED */

#ifdef MA__PROVIDE_TLS
	/* Check static TLS size */
	size_t static_tls_size, static_tls_align;
	_dl_get_tls_static_info(&static_tls_size, &static_tls_align);
	MA_BUG_ON(static_tls_align > MARCEL_ALIGN);
	if (static_tls_size + sizeof(lpt_tcb_t) > MA_TLS_AREA_SIZE) {
		fprintf(stderr,"Marcel has only %lu bytes for TLS while %ld are needed, please increase MA_TLS_AREA_SIZE. Aborting.\n", MA_TLS_AREA_SIZE, (unsigned long) (static_tls_size + sizeof(lpt_tcb_t)));
		abort();
	}
	/* Record the main thread's TLS register
	 * - extract infos from the main thread's control block (TCB)
	 *   --> see glibc-2.7/nptl/sysdeps/<ARCH>/tls.h for the NPTL's TCB layout
	 * - set the 'multiple_threads' field to 1; otherwise the NPTL's atomic ops are
	 *   optimized out as long as the program does not create any extra LWP.
	 * */
#ifdef X86_ARCH
	asm("movw %%gs, %w0" : "=q" (__main_thread_desc));
	asm("movl %0, %%gs:(0x0c)"::"r" (1)); /* multiple_threads */
	asm("movl %%gs:(0x10), %0":"=r" (sysinfo));
	asm("movl %%gs:(0x14), %0":"=r" (stack_guard));
	asm("movl %%gs:(0x18), %0":"=r" (pointer_guard));
#elif defined(X86_64_ARCH)
	syscall(SYS_arch_prctl, ARCH_GET_FS, &__main_thread_tls_base);
	asm("movl %0, %%fs:(0x18)"::"r" (1)); /* multiple_threads */
	asm("movq %%fs:(0x20), %0":"=r" (sysinfo));
	asm("movq %%fs:(0x28), %0":"=r" (stack_guard));
	asm("movq %%fs:(0x30), %0":"=r" (pointer_guard));
#elif defined(IA64_ARCH)
	register unsigned long base asm("r13");
	__main_thread_tls_base = base;
#else
#error TODO
#endif
	marcel_tls_slot_allocator = ma_new_obj_allocator(0,
			tls_slot_alloc, NULL, tls_slot_free, NULL,
			POLICY_HIERARCHICAL_MEMORY, MARCEL_THREAD_CACHE_MAX);
#endif
	/* TODO: on pourrait réduire la taille */
	marcel_thread_seed_allocator = ma_new_obj_allocator(0,
			ma_obj_allocator_malloc, (void *) sizeof(marcel_task_t),
			ma_obj_allocator_free, NULL,
			POLICY_HIERARCHICAL_MEMORY, MARCEL_THREAD_CACHE_MAX);

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

/******************* begin heap *******************/
#ifdef MA__NUMA

/* Marcel allocator */
void* marcel_malloc_customized(size_t size, enum pinfo_weight access, int local, int node, int level)
{
	/* Which entity ? */
	marcel_entity_t *entity = &MARCEL_SELF->as_entity;

	marcel_entity_t *upentity = entity;
	ma_holder_t *h;
	while (level--) {
		h = upentity->init_holder;
		if (h == NULL || h->type == MA_RUNQUEUE_HOLDER)
			break;
		upentity = &ma_bubble_holder(h)->as_entity;
	}

	/* New entity heap */
	if (!upentity->heap)
		upentity->heap = ma_hcreate_heap();
	
	/* Which node ? */
	unsigned long node_mask;
	mask_zero(&node_mask, sizeof(unsigned long));

	int local_node = ma_node_entity(upentity);

	if (local)
	{
		mask_set(&node_mask, local_node);
	}
	else
	{
		mask_set(&node_mask, node);
	}

	int policy = CYCLIC;
	//int policy = LESS_LOADED;
	//int policy = SMALL_ACCESSED;

	/* Smallest access */
	//enum pinfo_weight weight = ma_mem_access(access, size);
	enum pinfo_weight weight = access;
	void *data = ma_hmalloc(size, policy, weight, &node_mask, WORD_SIZE, upentity->heap);
	
	return data;
}

void* marcel_calloc_customized(size_t nmemb, size_t size, enum pinfo_weight access, int local, int node, int level)
{
	marcel_entity_t *entity = &MARCEL_SELF->as_entity;
	marcel_entity_t *upentity = entity;
	ma_holder_t *h;
	while (level--) {
		h = upentity->init_holder;
		if (h == NULL || h->type == MA_RUNQUEUE_HOLDER)
			break;
		upentity = &ma_bubble_holder(h)->as_entity;
	}

	if (!upentity->heap)
		upentity->heap = ma_hcreate_heap();
	
	unsigned long node_mask;
	mask_zero(&node_mask, sizeof(unsigned long));

	int local_node = ma_node_entity(upentity);

	if (local)
	{
		mask_set(&node_mask, local_node);
	}
	else
	{
		mask_set(&node_mask, node);
	}

	int policy = CYCLIC;
	//int policy = LESS_LOADED;
	//int policy = SMALL_ACCESSED;

	//enum pinfo_weight weight = ma_mem_access(access, size);
	enum pinfo_weight weight = access;
	void *data = ma_hcalloc(nmemb, size, policy, weight, &node_mask, WORD_SIZE, upentity->heap);
	
	return data;
}

void *marcel_realloc_customized(void *ptr, unsigned size)
{
	return ma_hrealloc(ptr,size);
}

/* Free memory */
void marcel_free_customized(void *data)
{
	ma_hfree(data);
}

/* Attach static memory */
void marcel_attach_memory(void *data, int size, enum pinfo_weight access, int local, int node, int level)
{
	marcel_entity_t *entity = &MARCEL_SELF->as_entity;
	marcel_entity_t *upentity = entity;
	ma_holder_t *h;
	while (level--) {
		h = upentity->init_holder;
		if (h == NULL || h->type == MA_RUNQUEUE_HOLDER)
			break;
		upentity = &ma_bubble_holder(h)->as_entity;
	}

	if (!upentity->heap)
		upentity->heap = ma_hcreate_heap();
	
	unsigned long node_mask;
	mask_zero(&node_mask, sizeof(unsigned long));
	
	int local_node = ma_node_entity(upentity);

	if (local)
	{
		mask_set(&node_mask, local_node);
	}
	else
	{
		mask_set(&node_mask, node);
	}

	int policy = CYCLIC;
	//int policy = LESS_LOADED;
	//int policy = SMALL_ACCESSED;

	//enum pinfo_weight weight = ma_mem_access(access, size);
	enum pinfo_weight weight = access;
	ma_hattach_memory(data, size, policy, weight, &node_mask, WORD_SIZE, upentity->heap);
}

/* Minimise access if small data */
enum pinfo_weight ma_mem_access(enum pinfo_weight access, int size)
{
	if (size < MA_CACHE_SIZE)
	{
		switch(access)
		{
			case HIGH_WEIGHT:
				return MEDIUM_WEIGHT;
				break;
			case MEDIUM_WEIGHT:
				return LOW_WEIGHT;
				break;
			default :
				return LOW_WEIGHT;
		}
	}
	return access;
}

/* Bubble thickness */
//tableau de noeuds
int ma_bubble_memory_affinity(marcel_bubble_t *bubble)//, ma_nodtab_t *attraction)
{
	ma_pinfo_t *pinfo;
	int total = 0;
	marcel_entity_t *entity = &bubble->as_entity;
	if (entity->heap)
	{
		pinfo = NULL;
		while (ma_hnext_pinfo(&pinfo, entity->heap))
		{
			switch (pinfo->weight) 
			{
				case (HIGH_WEIGHT) :
					total += HW;
					break;
				case (MEDIUM_WEIGHT) :
					total += MW;
					break;
				case (LOW_WEIGHT) :
					total += LW;
					break;
				default :
					total += 0;
			}
		}
	}
	return total;
}

/* Entity allocated memory, lock it recursively before */
int ma_entity_memory_volume(marcel_entity_t *entity, int recurse)
{
	ma_pinfo_t *pinfo;
	int total = 0;
	int begin = 1;
	if (entity->heap)
	{
		pinfo = NULL;
		while (ma_hnext_pinfo(&pinfo, entity->heap))
		{
			if (begin)
			{
				begin = 0;
			}
			total += pinfo->size;
		}
		marcel_entity_t *downentity;
		/* entities in bubble */
		if (entity->type == MA_BUBBLE_ENTITY)
		{
			list_for_each_entry(downentity, &ma_bubble_entity(entity)->heldentities, bubble_entity_list)
				{
					total += ma_entity_memory_volume(downentity, recurse + 1);
				}
		}
	}
	return total;
}

/* Memory attraction (weight and touched maps) */
void ma_attraction_inentity(marcel_entity_t *entity, ma_nodtab_t *allocated, int weight_coef, enum pinfo_weight access_min)
{
	ma_pinfo_t *pinfo;
	int poids;
	int i;

	for (i = 0 ; i < marcel_nbnodes ; ++i)
		allocated->array[i] = 0;
	if (entity->heap)
	{
		pinfo = NULL;
		
		if (entity->heap)
			while (ma_hnext_pinfo(&pinfo, entity->heap))
			{
				ma_hupdate_memory_nodes(pinfo, entity->heap);
				
				switch (pinfo->weight) 
				{
					case (HIGH_WEIGHT) :
						poids = HW;
						break;
					case (MEDIUM_WEIGHT) :
						/* pas considéré avec HIGH */
						if (access_min == LOW_WEIGHT
							 || access_min == MEDIUM_WEIGHT)
							poids = MW;
						else
							poids = 0;
						break;
						/* pas considéré avec MEDIUM et HIGH */
					case (LOW_WEIGHT) :
						if (access_min == LOW_WEIGHT)
							poids = LW;
						else
							poids = 0;
						break;
					default :
						poids = 0;
				}
				for (i = 0 ; i < marcel_nbnodes ; ++i)
				{
					if (weight_coef)
						allocated->array[i] += poids * pinfo->nb_touched[i];
					else 
						allocated->array[i] += pinfo->nb_touched[i];
				}
			}
	}
}

/* Recursif memory attraction, lock it recursively before */
int ma_most_attractive_node(marcel_entity_t *entity, ma_nodtab_t *allocated, int weight_coef, enum pinfo_weight access_min, int recurse)
{
	ma_nodtab_t local;
	marcel_entity_t *downentity;
	int i, imax;	

	if (!recurse)
	{
		/* init sum */
		for (i = 0 ; i < marcel_nbnodes ; ++i)
			allocated->array[i] = 0;
	}

	/* this entity sum */
	ma_attraction_inentity(entity, &local, weight_coef, access_min);
	
	for ( i = 0 ; i < marcel_nbnodes ; i++)
		allocated->array[i] += local.array[i];

	/* entities in bubble */
	if (entity->type == MA_BUBBLE_ENTITY)
	{
		list_for_each_entry(downentity, &ma_bubble_entity(entity)->heldentities, bubble_entity_list)
			{
				ma_most_attractive_node(downentity, allocated, weight_coef, access_min, recurse + 1);
			}
	}
	if (!recurse) 
	{  
		/* return the most allocated node */
		imax = 0;
		for (i = 0 ; i < marcel_nbnodes ; ++i)
		{
			if (allocated->array[i] > allocated->array[imax])
				imax = i;
		}
		//fprintf(stderr,"ma_most_allocated_node : entity %p on node %d\n", entity, imax);		 
		return imax;
	}
	return -1;
}

/* Compute attraction or volume with minimal access, lock it recursively before */
int ma_compute_total_attraction(marcel_entity_t *entity, int weight_coef, int access_min, int attraction, int *pnode)
{
	int total = 0;

	/* Nouveau calcul */
	ma_nodtab_t allocated;
	if (pnode)
		*pnode = ma_most_attractive_node(entity, &allocated, weight_coef,access_min,0);
	else
		ma_most_attractive_node(entity, &allocated, weight_coef,access_min,0);

	if (!attraction)
		return -1;

	/* sommer sur tous les noeuds */
	int i;
	for (i = 0 ; i < marcel_nbnodes ; i++ )
	{
		total += allocated.array[i];
	}
	return total;
}

/* Memory migration, lock it recursively before */
void ma_move_entity_alldata(marcel_entity_t *entity, int newnode)
{
	ma_pinfo_t *pinfo;
	ma_pinfo_t newpinfo;
	int migration = 0;
	int load;
	
	unsigned long newnodemask = 0;//[8] = {};
	mask_set(&newnodemask, newnode);
	if (entity->heap)
	{
			pinfo = NULL;
	
		while (ma_hnext_pinfo(&pinfo, entity->heap))
		{
			switch (pinfo->weight)
			{
				case HIGH_WEIGHT :
					migration = 1;
					break;

				case MEDIUM_WEIGHT :
					if (checkload)
						load = ma_entity_load(entity);
					else
						load = MA_DEFAULT_LOAD * ma_count_threads_in_entity(entity);
					
					if (load > MA_DEFAULT_LOAD)
						migration = 1;
					else
						migration = 0;
					break;
					
				case LOW_WEIGHT :
					migration = 0;
					break;
			}

			if (migration)
			{
				newpinfo.mempolicy = pinfo->mempolicy;
				newpinfo.weight = pinfo->weight;
				newpinfo.nodemask = &newnodemask;
				newpinfo.maxnode = WORD_SIZE;
				if (ma_hmove_memory(pinfo, newpinfo.mempolicy, newpinfo.weight,newpinfo.nodemask, newpinfo.maxnode, entity->heap))
				{
					pinfo = NULL;
				}
			}
		}
	}
	marcel_entity_t *downentity;
	if (entity->type == MA_BUBBLE_ENTITY)
	{
		list_for_each_entry(downentity, &ma_bubble_entity(entity)->heldentities, bubble_entity_list)
			{
				ma_move_entity_alldata(downentity, newnode);
			}
	}
}

/* See memory */
void marcel_see_allocated_memory(marcel_entity_t *entity)
{
	ma_nodtab_t allocated;
	int i;
	ma_most_attractive_node(entity, &allocated, 1, LOW_WEIGHT, 0);
	for (i = 0 ; i < marcel_nbnodes ; i++)
		marcel_fprintf(stderr,"ma_see_allocated_memory : thread %p : node %d -> %d Po\n", MARCEL_SELF, i, (int)allocated.array[i]);
}

/* find the node from the entity */
/* TODO => so pas de node level */
int ma_node_entity(marcel_entity_t *entity)
{
	ma_holder_t *holder = entity->sched_holder;
	/* if entity is scheduled in a bubble, find the first runqueue */
	while (holder->type == MA_BUBBLE_HOLDER)
	{
		holder = (ma_bubble_holder(holder)->as_entity).sched_holder;
		//marcel_fprintf(stderr,"holder %p\n",holder);			
	}	
	ma_runqueue_t *rq = ma_to_rq_holder(holder);
	struct marcel_topo_level *level
		= tbx_container_of(rq, struct marcel_topo_level, rq);
		
	if (nodelevel == -1)
	{
		//marcel_fprintf(stderr,"pas de nodelevel : level %p, level->level %d, level->number %d\n",level,level->level,level->number);
		return level->number;
	}
	while (level->level > nodelevel)
	{
		//marcel_fprintf(stderr,"level %p, level->level %d, level->number %d\n",level,level->level,level->number);
		if (level->father == NULL)
			return level->level;
		level = level->father;
	}
	return level->number;
}

#endif

/*******************end heap**************************/

/* TODO : une fois notre customized ok, gerer ces malloc */
/* marcel_malloc for the current entity on its level */
void* marcel_malloc(size_t size, const char *file, unsigned line)
{
	//if (!ma_is_numa_available())
	return ma_malloc_nonuma(size, __FILE__ , __LINE__);
	//else
	//return ma_malloc(size, __FILE__ , __LINE__);
}

void *marcel_realloc(void *ptr, unsigned size, const char * __restrict file, unsigned line)
{
	void *p;

	marcel_extlib_protect();
	p = __TBX_REALLOC(ptr, size, file, line);
	marcel_extlib_unprotect();
	if(p == NULL)
		MARCEL_EXCEPTION_RAISE(MARCEL_STORAGE_ERROR);

	return p;
}

void *marcel_calloc(unsigned nelem, unsigned elsize, const char *file, unsigned line)
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

void marcel_free(void *data)
{
	//if (!ma_is_numa_available())// penser au cas nnon linux (fonction externe, sysdep) si on sait pas, ma_free
	ma_free_nonuma(data, __FILE__ , __LINE__);
	//else
	//ma_free(data, __FILE__, __LINE__);
}

/* __marcel_malloc, __marcel_calloc, __marcel_free:
   internal use versions that ignore the TBX safe-malloc setting */
void *__marcel_malloc(unsigned size)
{
        void *p;

        if (size) {
		marcel_extlib_protect();
                p = malloc(size);
		marcel_extlib_unprotect();
                if(p == NULL)
                        MARCEL_EXCEPTION_RAISE(MARCEL_STORAGE_ERROR);
        } else {
                return NULL;
        }

        return p;
}

void *__marcel_realloc(void *ptr, unsigned size)
{
        void *p;

	marcel_extlib_protect();
        p = realloc(ptr, size);
	marcel_extlib_unprotect();
        if(p == NULL)
                MARCEL_EXCEPTION_RAISE(MARCEL_STORAGE_ERROR);

        return p;
}

void *__marcel_calloc(unsigned nelem, unsigned elsize)
{
        void *p;

        if (nelem && elsize) {
		marcel_extlib_protect();
                p = calloc(nelem, elsize);
		marcel_extlib_unprotect();
                if(p == NULL)
                        MARCEL_EXCEPTION_RAISE(MARCEL_STORAGE_ERROR);
        } else {
                return NULL;
        }

        return p;
}

void __marcel_free(void *ptr)
{
        if(ptr) {
		marcel_extlib_protect();
                free(ptr);
		marcel_extlib_unprotect();
        }
}


/* no NUMA malloc */
void* ma_malloc_nonuma(size_t size, const char *file, unsigned line)
{
	void *p;
	if (size) {
		marcel_extlib_protect();
		p = __TBX_MALLOC(size, file, line);
		marcel_extlib_unprotect();
		if(p == NULL)
			MARCEL_EXCEPTION_RAISE(MARCEL_STORAGE_ERROR);
	} 
	else {
		return NULL;
	}
	return p;
}

/* free non NUMA, version préexistante */
void ma_free_nonuma(void *data, const char * __restrict file, unsigned line)
{
	if(data) {
		marcel_extlib_protect();
		__TBX_FREE(data, file, line);
		marcel_extlib_unprotect();
	}	
}

void ma_memory_attach(marcel_entity_t *e, void *data, size_t size, int level)
{
#ifdef MA__NUMA
	struct memory_area *area;
	if (!e)
		e = &MARCEL_SELF->as_entity;
#ifdef MA__BUBBLES
	while (level--) {
		ma_holder_t *h;
		h = e->init_holder;
		MA_BUG_ON(h->type == MA_RUNQUEUE_HOLDER);
		e = &ma_bubble_holder(h)->as_entity;
	}
#endif
	area = ma_obj_alloc(memory_area_allocator);
	area->size = size;
	area->data = data;
	ma_spin_lock(&e->memory_areas_lock);
	list_add(&area->list, &e->memory_areas);
	ma_spin_unlock(&e->memory_areas_lock);
	ma_stats_add(long, e, ma_stats_memory_offset, size);
#endif
}

void ma_memory_detach(marcel_entity_t *e, void *data, int level)
{
#ifdef MA__NUMA
	struct memory_area *area;
	if (!e)
		e = &MARCEL_SELF->as_entity;
#ifdef MA__BUBBLES
	while (level--) {
		ma_holder_t *h;
		h = e->init_holder;
		MA_BUG_ON(h->type == MA_RUNQUEUE_HOLDER);
		e = &ma_bubble_holder(h)->as_entity;
	}
#endif
	ma_spin_lock(&e->memory_areas_lock);
	list_for_each_entry(area, &e->memory_areas, list)
		if (area->data == data) {
			data = NULL;
			list_del(&area->list);
			break;
		}
	MA_BUG_ON(data);
	ma_spin_unlock(&e->memory_areas_lock);
	ma_stats_sub(long, e, ma_stats_memory_offset, area->size);
	ma_obj_free(memory_area_allocator, area);
#endif
}
