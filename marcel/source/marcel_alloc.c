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
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#if defined(MA__PROVIDE_TLS) && (defined(X86_ARCH) || defined(X86_64_ARCH))
#include <sys/syscall.h>
#include <unistd.h>
#include <asm/ldt.h>
#ifdef X86_64_ARCH
#include <asm/prctl.h>
#endif
#endif

TBX_VISIBILITY_PUSH_INTERNAL 
unsigned long ma_stats_memory_offset;
unsigned long int ma_main_stacklimit;
ma_allocator_t *marcel_thread_seed_allocator;
ma_allocator_t *marcel_mapped_slot_allocator;
ma_allocator_t *marcel_lockcell_allocator;
TBX_VISIBILITY_POP 

static char *next_slot;
static ma_spinlock_t next_slot_lock = MA_SPIN_LOCK_UNLOCKED;
static ma_allocator_t *marcel_unmapped_slot_allocator;

#if defined(MA__PROVIDE_TLS)
extern void *_dl_allocate_tls(void *) ma_libc_internal_function;
extern void _dl_deallocate_tls(void *, int) ma_libc_internal_function;
extern void _dl_get_tls_static_info(size_t * sizep, size_t * alignp) ma_libc_internal_function;
extern void _rtld_global_ro;
static ma_allocator_t *marcel_tls_slot_allocator;
#if defined(X86_64_ARCH) || defined(IA64_ARCH)
unsigned long __main_thread_tls_base;
#endif
#if defined(X86_ARCH) || defined(X86_64_ARCH)
static ma_allocator_t *marcel_ldt_allocator;
static uintptr_t sysinfo;
static unsigned long stack_guard, pointer_guard;
static int gscope_flag, private_futex;
#endif
#endif

/* Allocate a new slot but don't map it yet */
static void *unmapped_slot_alloc(void *foo TBX_UNUSED)
{
	char *ptr;

	ma_spin_lock(&next_slot_lock);
	if ((unsigned long) next_slot <= SLOT_AREA_BOTTOM) {
		ma_spin_unlock(&next_slot_lock);
		return NULL;
	}
	ptr = next_slot -=
#if defined(PM2VALGRIND) || defined(PM2STACKSGUARD) || defined(PM2DEBUG)
	    /* Put a hole between slots to catch overflows */
	    2 *
#endif
	    THREAD_SLOT_SIZE;
	ma_spin_unlock(&next_slot_lock);
	return ptr;
}

/* Map a new slot */
static void *mapped_slot_alloc(void *foo TBX_UNUSED)
{
	char *ptr;
	void *res;
	int nb_try_left = 1000;


      retry:
	ptr = ma_obj_alloc(marcel_unmapped_slot_allocator, NULL);
	if (!ptr) {
		if (!ma_in_atomic() && nb_try_left--) {
			MARCEL_ALLOC_LOG("Not enough room for new stack (stack size is %luKB, stack "
					 "allocation area is 0x%lx-0x%lx), trying to wait for other threads to terminate.\n",
					 THREAD_SLOT_SIZE/1024, (unsigned long) SLOT_AREA_BOTTOM,
					 (unsigned long) SLOT_AREA_TOP);
			marcel_yield();
			goto retry;
		} else {
			/* Erf. We are in atomic section or tried too long. */
			MA_WARN_USER("Not enough room for new stack (stack size is %luKB, stack "
				     "allocation area is 0x%lx-0x%lx)\n",
				     THREAD_SLOT_SIZE/1024, (unsigned long) SLOT_AREA_BOTTOM,
				     (unsigned long) ISOADDR_AREA_TOP);
			MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);
		}
	}

	/* TODO: m�moriser l'id et effectuer des VALGRIND_STACK_DEREGISTER sur munmap() */
	VALGRIND_STACK_REGISTER(ptr, ptr + THREAD_SLOT_SIZE);

	res = marcel_mmap(ptr, THREAD_SLOT_SIZE,
			  PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_FIXED | MAP_ANON, -1, 0);

	if (res == MAP_FAILED) {
		if (!ma_in_atomic() && nb_try_left--) {
			MARCEL_ALLOC_LOG("mmap(%p, %lx, ...) for stack failed!" 
					 "Trying to wait for other threads to terminate\n",
					 next_slot, THREAD_SLOT_SIZE);
			marcel_yield();
			goto retry;
		}
		perror("mmap");
		MARCEL_ALLOC_LOG("args %p, %lx, %d, %d",
				 ptr, THREAD_SLOT_SIZE, MAP_PRIVATE | MAP_FIXED | MAP_ANON, -1);
		MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);
	}

	MA_BUG_ON(res != ptr);

	return ptr;
}

/* Allocate a TLS area for the slot */
#ifdef MA__PROVIDE_TLS
#if defined(X86_ARCH) || defined(X86_64_ARCH)
/* Allocate an LDT entry */
static void *ldt_alloc(void *foo TBX_UNUSED)
{
	static ma_spinlock_t next_ldt_lock = MA_SPIN_LOCK_UNLOCKED;
	static uintptr_t next_ldt = 0;
	uintptr_t ret;

	ma_spin_lock(&next_ldt_lock);
	if (next_ldt >= 8192) {
		MA_WARN_USER("no more LDT entries\n");
		MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);
	} else
		ret = next_ldt++;
	ma_spin_unlock(&next_ldt_lock);
	return (void *) (ret + 1);
}
#endif

void marcel_tls_attach(marcel_t t)
{
#if defined(LINUX_SYS)
	lpt_tcb_t *tcb = marcel_tcb(t);
	_dl_allocate_tls(tcb);

#if defined(X86_ARCH) || defined(X86_64_ARCH)
	unsigned int ldt_entry = ((uintptr_t) ma_obj_alloc(marcel_ldt_allocator, NULL)) - 1;

	t->tls_desc = (unsigned short)(ldt_entry * 8 | 0x4);

	tcb->tcb = tcb;
	tcb->self = tcb;
	tcb->multiple_threads = 1;
	tcb->sysinfo = sysinfo;
	tcb->stack_guard = stack_guard;
	tcb->pointer_guard = pointer_guard;
	tcb->gscope_flag = gscope_flag;
	tcb->private_futex = private_futex;

#ifdef X86_64_ARCH
	/* because else we can't use ldt */
	MA_ALWAYS_BUG_ON((unsigned long) tcb >= (1ul << 32));
#endif
	struct user_desc desc = {
		.entry_number = ldt_entry,
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
		MA_WARN_USER("entry #%u address %lx failed: %s\n", desc.entry_number,
			     (unsigned long) desc.base_addr, strerror(errno));
		MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);
	}
#endif
#endif
}

void marcel_tls_detach(marcel_t t)
{
#if defined(X86_ARCH) || defined(X86_64_ARCH)
	if (t->tls_desc)
		ma_obj_free(marcel_ldt_allocator,
			    (void *) (uintptr_t) (t->tls_desc / 8 + 1), NULL);
#endif
}

static void *tls_slot_alloc(void *foo TBX_UNUSED)
{
	void *ptr = ma_obj_alloc(marcel_mapped_slot_allocator, NULL);
	marcel_t t = ma_slot_task(ptr);
	marcel_tls_attach(t);
	return ptr;
}

/* Free TLS area of the slot */
static void tls_slot_free(void *slot, void *foo TBX_UNUSED)
{
	marcel_t t = ma_slot_task(slot);
	lpt_tcb_t *tcb = marcel_tcb(t);
	marcel_tls_detach(t);
	_dl_deallocate_tls(tcb, 0);
	ma_obj_free(marcel_mapped_slot_allocator, slot, NULL);
}
#endif				/* MA__PROVIDE_TLS */

/* Unmap the slot */
static void mapped_slot_free(void *slot, void *foo TBX_UNUSED)
{
	if (marcel_munmap(slot, THREAD_SLOT_SIZE) == -1)
		MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);
	ma_obj_free(marcel_unmapped_slot_allocator, slot, NULL);
}

void *marcel_slot_alloc(struct marcel_topo_level *level)
{
	return ma_obj_alloc(marcel_mapped_slot_allocator, level);
}

void marcel_slot_free(void *addr, struct marcel_topo_level *level) 
{
	ma_obj_free(marcel_mapped_slot_allocator, addr, level);
}


#ifdef MA__PROVIDE_TLS
void *marcel_tls_slot_alloc(struct marcel_topo_level *level)
{
	return ma_obj_alloc(marcel_tls_slot_allocator, level);
}

void marcel_tls_slot_free(void *addr, struct marcel_topo_level *level)
{
	ma_obj_free(marcel_tls_slot_allocator, addr, level);
}
#endif

static void marcel_slot_init(void)
{
	unsigned nb_topo_containers = 1;
	unsigned thread_cache_max;
	const unsigned nb_slots_max =
	    (unsigned) ((ISOADDR_AREA_TOP - SLOT_AREA_BOTTOM) / THREAD_SLOT_SIZE);

	MARCEL_ALLOC_LOG_IN();
	{
		unsigned l;
		for (l = 1; l < marcel_topo_nblevels; l++)
			nb_topo_containers += marcel_topo_level_nbitems[l];
	}
	thread_cache_max = nb_slots_max / nb_topo_containers / 4;
	if (thread_cache_max < 1)
		thread_cache_max = 1;

	next_slot = (char *) SLOT_AREA_TOP;
	/* SLOT_AREA_TOP doit être un multiple de THREAD_SLOT_SIZE
	 * et THREAD_SLOT_SIZE doit être une puissance de deux (non vérifié
	 * ici)
	 */
	MA_BUG_ON(0 != (SLOT_AREA_TOP & (THREAD_SLOT_SIZE - 1)));

	marcel_unmapped_slot_allocator =
		ma_new_obj_allocator(1, unmapped_slot_alloc, NULL, NULL, NULL,
				     POLICY_HIERARCHICAL, thread_cache_max);
	marcel_mapped_slot_allocator =
		ma_new_obj_allocator(0, mapped_slot_alloc, NULL, mapped_slot_free, NULL,
				     POLICY_HIERARCHICAL_MEMORY, thread_cache_max);
#ifdef MARCEL_STATS_ENABLED
	ma_stats_memory_offset =
	    ma_stats_alloc(ma_stats_long_sum_reset, ma_stats_long_sum_synthesis,
			   sizeof(long));
#endif				/* MARCEL_STATS_ENABLED */

#ifdef MA__PROVIDE_TLS
	/* Check static TLS size */
	size_t static_tls_size, static_tls_align;
	_dl_get_tls_static_info(&static_tls_size, &static_tls_align);
	MA_BUG_ON(static_tls_align > MARCEL_ALIGN);
	if (static_tls_size + sizeof(lpt_tcb_t) > MARCEL_TLS_AREA_SIZE) {
		MA_WARN_USER("Marcel has only %lu bytes for TLS while %lu are needed, please increase tls area size ; reconfigure Marcel with the following option: --with-tls_area_size=xxxx. Aborting.\n",
			     (unsigned long) MARCEL_TLS_AREA_SIZE,
			     (unsigned long) (static_tls_size + sizeof(lpt_tcb_t)));
		abort();
	}
	/* Record the main thread's TLS register
	 * - extract infos from the main thread's control block (TCB)
	 *   --> see glibc-2.7/nptl/sysdeps/<ARCH>/tls.h for the NPTL's TCB layout
	 * - set the 'multiple_threads' field to 1; otherwise the NPTL's atomic ops are
	 *   optimized out as long as the program does not create any extra LWP.
	 *
	 * Note: we do not initialize `self' because glibc has already initialized
	 * it and seems to depend on its value not changing. */
#ifdef X86_ARCH
	__main_thread->tls_desc = get_gs();
#endif
#ifdef X86_64_ARCH
	syscall(SYS_arch_prctl, ARCH_GET_FS, &__main_thread_tls_base);
	__main_thread->tls_desc = 0;
#endif
#if defined(X86_ARCH) || defined(X86_64_ARCH)
	MA_SET_TCB(multiple_threads, 1);
	sysinfo = MA_GET_TCB(sysinfo);
	stack_guard = MA_GET_TCB(stack_guard);
	pointer_guard = MA_GET_TCB(pointer_guard);
	gscope_flag = MA_GET_TCB(gscope_flag);
	private_futex = MA_GET_TCB(private_futex);
#endif
#ifdef IA64_ARCH
	register unsigned long base asm("r13");
	__main_thread_tls_base = base;
#endif
#if !defined(X86_ARCH) && !defined(X86_64_ARCH) && !defined(IA64_ARCH)
#error TODO
#endif
#if defined(X86_ARCH) || defined(X86_64_ARCH)
	/* LDT entries allocator */
	marcel_ldt_allocator =
		ma_new_obj_allocator(1, ldt_alloc, NULL, NULL, NULL, POLICY_HIERARCHICAL, 32);
#endif
	marcel_tls_slot_allocator = ma_new_obj_allocator(0, tls_slot_alloc, NULL, tls_slot_free, NULL,
							 POLICY_HIERARCHICAL_MEMORY, thread_cache_max);
#else
#ifdef IA64_ARCH
	/* save tls segment location in lwp data structure */
	register unsigned long base asm("r13");
	__main_lwp.ma_tls_tp = base;
#endif
#endif
	/* TODO: on pourrait r�duire la taille */
	marcel_thread_seed_allocator = ma_new_obj_allocator(0,
							    ma_obj_allocator_malloc,
							    (void *)sizeof(marcel_task_t),
							    ma_obj_allocator_free, NULL,
							    POLICY_HIERARCHICAL_MEMORY,
							    thread_cache_max);

	marcel_lockcell_allocator = ma_new_obj_allocator(0,
							 ma_obj_allocator_malloc,
							 (void *)sizeof(blockcell),
							 ma_obj_allocator_free, NULL,
							 POLICY_HIERARCHICAL_MEMORY,
							 thread_cache_max);

	MARCEL_ALLOC_LOG_OUT();
}

__ma_initfunc_prio(marcel_slot_init, MA_INIT_SLOT, MA_INIT_SLOT_PRIO,
		   "Initialise memory slot system");

void marcel_slot_exit(void)
{
#ifdef MA__PROVIDE_TLS
	ma_obj_allocator_fini(marcel_tls_slot_allocator);
#endif
	ma_obj_allocator_fini(marcel_mapped_slot_allocator);
	ma_obj_allocator_fini(marcel_unmapped_slot_allocator);
}


#ifdef MM_HEAP_ENABLED

/* Marcel allocator */
void *marcel_malloc_customized(size_t size, enum heap_pinfo_weight access, int local,
			       int node, int level)
{
	/* Which entity ? */
	marcel_entity_t *entity = &MARCEL_SELF->as_entity;

	marcel_entity_t *upentity = entity;
	ma_holder_t *h;
	while (level--) {
		h = upentity->natural_holder;
		if (h == NULL || h->type == MA_RUNQUEUE_HOLDER)
			break;
		upentity = &ma_bubble_holder(h)->as_entity;
	}

	/* New entity heap */
	if (!upentity->heap)
		upentity->heap = heap_hcreate_heap();

	/* Which node ? */
	unsigned long node_mask;
	heap_mask_zero(&node_mask, sizeof(unsigned long));

	int local_node = ma_node_entity(upentity);

	if (local) {
		heap_mask_set(&node_mask, local_node);
	} else {
		heap_mask_set(&node_mask, node);
	}

	int policy = CYCLIC;
	//int policy = LESS_LOADED;
	//int policy = SMALL_ACCESSED;

	/* Smallest access */
	//enum pinfo_weight weight = ma_mem_access(access, size);
	enum heap_pinfo_weight weight = access;
	void *data = heap_hmalloc(size, policy, weight, &node_mask, HEAP_WORD_SIZE,
				  upentity->heap);

	return data;
}

/* Free memory */
void marcel_free_customized(void *data)
{
	heap_hfree(data);
}

/* Minimise access if small data */
enum heap_pinfo_weight ma_mem_access(enum heap_pinfo_weight access, int size)
{
	if (size < MA_CACHE_SIZE) {
		switch (access) {
		case HIGH_WEIGHT:
			return MEDIUM_WEIGHT;
			break;
		case MEDIUM_WEIGHT:
			return LOW_WEIGHT;
			break;
		default:
			return LOW_WEIGHT;
		}
	}
	return access;
}

/* Bubble thickness */
//tableau de noeuds
int ma_bubble_memory_affinity(marcel_bubble_t * bubble)	//, ma_nodtab_t *attraction)
{
	heap_pinfo_t *pinfo;
	int total = 0;
	marcel_entity_t *entity = &bubble->as_entity;
	if (entity->heap) {
		pinfo = NULL;
		while (heap_hnext_pinfo(&pinfo, entity->heap)) {
			switch (pinfo->weight) {
			case (HIGH_WEIGHT):
				total += HW;
				break;
			case (MEDIUM_WEIGHT):
				total += MW;
				break;
			case (LOW_WEIGHT):
				total += LW;
				break;
			default:
				total += 0;
			}
		}
	}
	return total;
}

/* Entity allocated memory, lock it recursively before */
int ma_entity_memory_volume(marcel_entity_t * entity, int recurse)
{
	heap_pinfo_t *pinfo;
	int total = 0;
	int begin = 1;
	if (entity->heap) {
		pinfo = NULL;
		while (heap_hnext_pinfo(&pinfo, entity->heap)) {
			if (begin) {
				begin = 0;
			}
			total += pinfo->size;
		}
		marcel_entity_t *downentity;
		/* entities in bubble */
		if (entity->type == MA_BUBBLE_ENTITY) {
			tbx_fast_list_for_each_entry(downentity,
						     &ma_bubble_entity
						     (entity)->natural_entities,
						     natural_entities_item) {
				total += ma_entity_memory_volume(downentity, recurse + 1);
			}
		}
	}
	return total;
}

/* Memory attraction (weight and touched maps) */
void ma_attraction_inentity(marcel_entity_t * entity, ma_nodtab_t * allocated,
			    int weight_coef, enum heap_pinfo_weight access_min)
{
	heap_pinfo_t *pinfo;
	int poids;
	int i;

	for (i = 0; i < marcel_nbnodes; ++i)
		allocated->array[i] = 0;
	if (entity->heap) {
		pinfo = NULL;

		if (entity->heap)
			while (heap_hnext_pinfo(&pinfo, entity->heap)) {
				heap_hupdate_memory_nodes(pinfo, entity->heap);

				switch (pinfo->weight) {
				case (HIGH_WEIGHT):
					poids = HW;
					break;
				case (MEDIUM_WEIGHT):
					/* pas consid�r� avec HIGH */
					if (access_min == LOW_WEIGHT
					    || access_min == MEDIUM_WEIGHT)
						poids = MW;
					else
						poids = 0;
					break;
					/* pas consid�r� avec MEDIUM et HIGH */
				case (LOW_WEIGHT):
					if (access_min == LOW_WEIGHT)
						poids = LW;
					else
						poids = 0;
					break;
				default:
					poids = 0;
				}
				for (i = 0; i < marcel_nbnodes; ++i) {
					if (weight_coef)
						allocated->array[i] +=
						    poids * pinfo->nb_touched[i];
					else
						allocated->array[i] +=
						    pinfo->nb_touched[i];
				}
			}
	}
}

/* Recursif memory attraction, lock it recursively before */
int ma_most_attractive_node(marcel_entity_t * entity, ma_nodtab_t * allocated,
			    int weight_coef, enum heap_pinfo_weight access_min,
			    int recurse)
{
	ma_nodtab_t local;
	marcel_entity_t *downentity;
	int i, imax;

	if (!recurse) {
		/* init sum */
		for (i = 0; i < marcel_nbnodes; ++i)
			allocated->array[i] = 0;
	}

	/* this entity sum */
	ma_attraction_inentity(entity, &local, weight_coef, access_min);

	for (i = 0; i < marcel_nbnodes; i++)
		allocated->array[i] += local.array[i];

	/* entities in bubble */
	if (entity->type == MA_BUBBLE_ENTITY) {
		tbx_fast_list_for_each_entry(downentity,
					     &ma_bubble_entity(entity)->natural_entities,
					     natural_entities_item) {
			ma_most_attractive_node(downentity, allocated, weight_coef,
						access_min, recurse + 1);
		}
	}
	if (!recurse) {
		/* return the most allocated node */
		imax = 0;
		for (i = 0; i < marcel_nbnodes; ++i) {
			if (allocated->array[i] > allocated->array[imax])
				imax = i;
		}
		MARCEL_LOG_ALLOC("ma_most_allocated_node : entity %p on node %d\n", entity, imax);
		return imax;
	}
	return -1;
}

/* Compute attraction or volume with minimal access, lock it recursively before */
int ma_compute_total_attraction(marcel_entity_t * entity, int weight_coef, int access_min,
				int attraction, int *pnode)
{
	int total = 0;

	/* Nouveau calcul */
	ma_nodtab_t allocated;
	if (pnode)
		*pnode =
		    ma_most_attractive_node(entity, &allocated, weight_coef, access_min,
					    0);
	else
		ma_most_attractive_node(entity, &allocated, weight_coef, access_min, 0);

	if (!attraction)
		return -1;

	/* sommer sur tous les noeuds */
	int i;
	for (i = 0; i < marcel_nbnodes; i++) {
		total += allocated.array[i];
	}
	return total;
}

/* Memory migration, lock it recursively before */
void ma_move_entity_alldata(marcel_entity_t * entity, int newnode)
{
	heap_pinfo_t *pinfo;
	heap_pinfo_t newpinfo;
	int migration = 0;
	int load;

	unsigned long newnodemask = 0;	//[8] = {};
	heap_mask_set(&newnodemask, newnode);
	if (entity->heap) {
		pinfo = NULL;

		while (heap_hnext_pinfo(&pinfo, entity->heap)) {
			switch (pinfo->weight) {
			case HIGH_WEIGHT:
				migration = 1;
				break;

			case MEDIUM_WEIGHT:
				load = MA_DEFAULT_LOAD * ma_count_threads_in_entity(entity);
				if (load > MA_DEFAULT_LOAD)
					migration = 1;
				else
					migration = 0;
				break;

			case LOW_WEIGHT:
				migration = 0;
				break;
			}

			if (migration) {
				newpinfo.mempolicy = pinfo->mempolicy;
				newpinfo.weight = pinfo->weight;
				newpinfo.nodemask = &newnodemask;
				newpinfo.maxnode = HEAP_WORD_SIZE;
				if (heap_hmove_memory
				    (pinfo, newpinfo.mempolicy, newpinfo.weight,
				     newpinfo.nodemask, newpinfo.maxnode, entity->heap)) {
					pinfo = NULL;
				}
			}
		}
	}
	marcel_entity_t *downentity;
	if (entity->type == MA_BUBBLE_ENTITY) {
		tbx_fast_list_for_each_entry(downentity,
					     &ma_bubble_entity(entity)->natural_entities,
					     natural_entities_item) {
			ma_move_entity_alldata(downentity, newnode);
		}
	}
}

/* See memory */
void marcel_see_allocated_memory(marcel_entity_t * entity)
{
	ma_nodtab_t allocated;
	int i;
	ma_most_attractive_node(entity, &allocated, 1, LOW_WEIGHT, 0);
	for (i = 0; i < marcel_nbnodes; i++)
		marcel_fprint(stderr, "ma_see_allocated_memory : thread %p : node %d -> %d Po\n",
			      MARCEL_SELF, i, (int) allocated.array[i]);
}

/* find the node from the entity */
/* TODO => so pas de node level */
int ma_node_entity(marcel_entity_t * entity)
{
	ma_holder_t *holder;
	ma_runqueue_t *rq;
	struct marcel_topo_level *level;

	/* if entity is scheduled in a bubble, find the first runqueue */
	holder = entity->sched_holder;
	while (holder->type == MA_BUBBLE_HOLDER) {
		holder = (ma_bubble_holder(holder)->as_entity).sched_holder;
		MARCEL_LOG("holder %p\n",holder);
	}
	rq = ma_to_rq_holder(holder);
	level = tbx_container_of(rq, struct marcel_topo_level, rq);
	return level->number;
}

#endif				/* MM_HEAP_ENABLED */


/* returns the amount of mem between the base of the current thread stack and
   its stack pointer */
unsigned long marcel_usablestack(void)
{
	return ((unsigned long) get_sp() - (unsigned long) ma_self()->stack_base);
}
