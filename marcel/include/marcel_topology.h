
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2004 "the PM2 team" (see AUTHORS file)
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

/** \file
 * \brief Topology management
 * \defgroup marcel_topology Topology management
 *
 * Topology is recorded in ::marcel_topo_levels, an array of arrays of struct
 * marcel_topo_level.
 *
 * It is usually browsed either as a tree, for instance:
 *
 * \code
 * void f(struct marcel_topo_level *l, struct marcel_topo_level *son) {
 * 	...
 *	// look down first
 * 	for (int i=0; i<l->arity; i++)
 * 		// avoid recursing into where we came from
 * 		if (l->children[i] != son)
 * 			f(l, l->children[i]);
 * 	...
 *	// then look up
 * 	f(l->father, l);
 * 	...
 * }
 *
 * f(&marcel_topo_vp_level[marcel_current_vp()], NULL);
 * \endcode
 *
 * or as an array of arrays, for instance:
 *
 * \code
 * for (i=0; i<marcel_topo_nblevels; i++) {
 * 	for (j=0; j<marcel_topo_level_nbitems[i]; j++) {
 * 		struct marcel_topo_level *l = &marcel_topo_levels[i][j];
 * 		...
 * 	}
 * }
 * \endcode
 *
 * or as an array of NULL-terminated arrays, for instance:
 *
 * \code
 * for (i=0; i<marcel_topo_nblevels; i++) {
 * 	for (l=&marcel_topo_levels[i][0]; l->vpset; l++) {
 * 		...
 * 	}
 * }
 * \endcode
 *
 * @{ */

#section common
#include "tbx_compiler.h"

#section variables
/** \brief Total number of physical processors */
extern unsigned marcel_nbprocessors;
/** \brief Processors "stride", i.e. the number of physical processors skipped
 * for each VP, stride = 2 can be a good idea for 2-SMT processors for instance
 */
extern unsigned marcel_cpu_stride;
/** \brief Number of the first processor used by Marcel. */
extern unsigned marcel_first_cpu;
/** \brief Number of VPs per physical processor */
extern unsigned marcel_vps_per_cpu;
#ifdef MA__NUMA
/** \brief Maximum allowed arity in the level tree */
extern unsigned marcel_topo_max_arity;
/** \brief Number of NUMA nodes */
extern unsigned marcel_nbnodes;
/** \brief Direct access to node levels */
extern struct marcel_topo_level *marcel_topo_node_level;
#endif
/** \brief Direct access to VP levels */
extern struct marcel_topo_level *marcel_topo_vp_level;
#ifndef MA__LWPS
#  define marcel_nbprocessors 1
#  define marcel_cpu_stride 1
#define marcel_first_cpu 0
#  define marcel_vps_per_cpu 1
#  define marcel_topo_vp_level marcel_machine_level
#endif
#ifndef MA__NUMA
#  define marcel_topo_node_level marcel_machine_level
#endif

#section marcel_variables
#depend "sys/marcel_lwp.h[marcel_macros]"
/** \brief VP number to node number conversion array */
extern int ma_vp_node[MA_NR_LWPS];

#section functions
/** \brief Fill ::marcel_nbprocessors with the number of available
 *  processors */
extern void ma_set_nbprocessors(void);
/** \brief Compute ::marcel_vps_per_cpu, and ::marcel_cpu_stride if not
 *  already set */
extern void ma_set_processors(void);
/** \brief Free all topology levels but the last one */
extern void ma_topo_exit(void);
#ifndef MA__LWPS
#  define ma_set_nbprocessors() (void)0
#  define ma_set_processors() (void)0
#  define ma_topo_exit() (void)0
#endif

#section types
/** \brief Type of topology level */
enum marcel_topo_level_e {
	MARCEL_LEVEL_MACHINE,	/**< \brief Whole machine */
#ifndef MA__LWPS
#  define MARCEL_LEVEL_LAST MARCEL_LEVEL_MACHINE
#  define MARCEL_LEVEL_VP MARCEL_LEVEL_MACHINE
#  define MARCEL_LEVEL_NODE MARCEL_LEVEL_MACHINE
#else
	MARCEL_LEVEL_FAKE,	/**< \brief Fake level for meeting the marcel_topo_max_arity constraint */
#  ifdef MA__NUMA
	MARCEL_LEVEL_NODE,	/**< \brief NUMA node */
	MARCEL_LEVEL_DIE,	/**< \brief Physical chip */
	MARCEL_LEVEL_L3,	/**< \brief L3 cache */
	MARCEL_LEVEL_L2,	/**< \brief L2 cache */
	MARCEL_LEVEL_CORE,	/**< \brief Core */
	MARCEL_LEVEL_PROC,	/**< \brief SMT Processor in a core */
#  endif
	MARCEL_LEVEL_VP,	/**< \brief Virtual Processor (\b not SMT) */
#  define MARCEL_LEVEL_LAST MARCEL_LEVEL_VP
#endif
};

/****************************************************************/
/*               Virtual Processors                             */
/****************************************************************/

#section types
#include <limits.h>
#if !defined(MA__LWPS) || (1<<(MARCEL_NBMAXCPUS-1) < UINT_MAX)
/** \brief Virtual processor set: defines the set of "allowed" LWP for a given thread */
typedef unsigned marcel_vpset_t;
/** \brief Typed unset bit constant. */
#    define MARCEL_VPSET_CONST_0	0U
/** \brief Typed set bit constant. */
#    define MARCEL_VPSET_CONST_1	1U
/** \brief Format string snippet suitable for the vpset datatype */
#    define MA_VPSET_x			"x"
#elif (1<<(MARCEL_NBMAXCPUS-1) < ULONG_MAX)
typedef unsigned long marcel_vpset_t;
#    define MARCEL_VPSET_CONST_0	0UL
#    define MARCEL_VPSET_CONST_1	1UL
#    define MA_VPSET_x			"lx"
#elif (1<<(MARCEL_NBMAXCPUS-1) < ULLONG_MAX)
typedef unsigned long long marcel_vpset_t;
#    define MARCEL_VPSET_CONST_0	0ULL
#    define MARCEL_VPSET_CONST_1	1ULL
#    define MA_VPSET_x			"llx"
#else
#    error MARCEL_NBMAXCPUS is too big, change it in marcel_config.h
#endif

/** \brief Set with no LWP selected. */
#define MARCEL_VPSET_ZERO           ((marcel_vpset_t)  MARCEL_VPSET_CONST_0)
#define MARCEL_VPSET_FULL	    (~MARCEL_VPSET_ZERO)
/** \brief Set with only \e vp in selected. */
#define MARCEL_VPSET_VP(vp)         ((marcel_vpset_t)(  MARCEL_VPSET_CONST_1 << (vp)) )

#section functions

/*  Primitives & macros for building "sets" of virtual processors. */

/** \brief Initialize VP set */
void marcel_vpset_init(marcel_vpset_t * set);
#define marcel_vpset_init(m)        marcel_vpset_empty(m)

#section functions
/** \brief Empty VP set */
static __tbx_inline__ void marcel_vpset_zero(marcel_vpset_t * set);
#section inline
static __tbx_inline__ void marcel_vpset_zero(marcel_vpset_t * set)
{
	*set = MARCEL_VPSET_ZERO;
}

#section functions
/** \brief Fill VP set */
static __tbx_inline__ void marcel_vpset_fill(marcel_vpset_t * set);
#section inline
static __tbx_inline__ void marcel_vpset_fill(marcel_vpset_t * set)
{
	*set = MARCEL_VPSET_FULL;
}

#section functions
/** \brief Clear VP set and set VP \e vp */
static __tbx_inline__ void marcel_vpset_vp(marcel_vpset_t * set,
    unsigned vp);
#section inline
static __tbx_inline__ void marcel_vpset_vp(marcel_vpset_t * set,
    unsigned vp)
{
#ifdef MA__LWPS
	*set = MARCEL_VPSET_VP(vp);
#else
	marcel_vpset_fill(set);
#endif
}

#section functions
/** \brief Clear VP set and set all but the VP \e vp */
static __tbx_inline__ void marcel_vpset_all_but_vp(marcel_vpset_t * set,
    unsigned vp);
#section inline
static __tbx_inline__ void marcel_vpset_all_but_vp(marcel_vpset_t * set,
    unsigned vp)
{
#ifdef MA__LWPS
	*set = ~MARCEL_VPSET_VP(vp);
#else
	marcel_vpset_zero(set);
#endif
}

#section functions
/** \brief Add VP \e vp in VP set \e set */
static __tbx_inline__ void marcel_vpset_set(marcel_vpset_t * set,
    unsigned vp);
#section inline
static __tbx_inline__ void marcel_vpset_set(marcel_vpset_t * set,
    unsigned vp)
{
#ifdef MA__LWPS
	*set |= MARCEL_VPSET_VP(vp);
#else
	marcel_vpset_fill(set);
#endif
}

#section functions
/** \brief Remove VP \e vp from VP set \e set */
static __tbx_inline__ void marcel_vpset_clr(marcel_vpset_t * set,
    unsigned vp);
#section inline
static __tbx_inline__ void marcel_vpset_clr(marcel_vpset_t * set,
    unsigned vp)
{
#ifdef MA__LWPS
	*set &= ~(MARCEL_VPSET_VP(vp));
#else
	marcel_vpset_zero(set);
#endif
}

#section functions
/** \brief Test whether VP \e vp is part of set \e set */
static __tbx_inline__ int marcel_vpset_isset(const marcel_vpset_t * set,
    unsigned vp);
#section inline
static __tbx_inline__ int marcel_vpset_isset(const marcel_vpset_t * set,
    unsigned vp)
{
#ifdef MA__LWPS
	return 1 & (*set >> vp);
#else
	return *set;
#endif
}

#section marcel_functions
/** \brief Compute the number of VPs in VP mask */
static __tbx_inline__ int marcel_vpset_weight(const marcel_vpset_t * vpset);
#section marcel_inline
#depend "asm/linux_bitops.h[marcel_inline]"
static __tbx_inline__ int marcel_vpset_weight(const marcel_vpset_t * vpset)
{
#ifdef MA__LWPS
	return ma_hweight_long(*vpset);
#else
	return *vpset;
#endif
}

#section marcel_macros
/** \brief Loop macro iterating on a vpset and yielding on each vp that
 *  is member of the set.
 *  Uses variables \e set (the vp set) and \e vp (the loop variable) */
#define marcel_vpset_foreach_begin(vp, vpset) \
        for (vp = 0; vp < marcel_nbvps() + MARCEL_NBMAXVPSUP; vp++) \
                if (marcel_vpset_isset(vpset, vp)) {
#define marcel_vpset_foreach_end() \
                }

#section functions
/** \brief Get the current VP number. Note that if preemption is enabled,
 *  this may change just after the function call. */
unsigned marcel_current_vp(void);
#section marcel_functions
/* Internal version, for inlining */
static __tbx_inline__ unsigned __marcel_current_vp(void);
#section marcel_inline
#depend "sys/marcel_lwp.h[variables]"
static __tbx_inline__ unsigned __marcel_current_vp(void)
{
	return ma_vpnum(MA_LWP_SELF);
}
#define marcel_current_vp __marcel_current_vp

#section marcel_structures

#ifdef PM2_DEV
/*  #warning should not rely on a specific scheduler */
#endif
#depend "scheduler-marcel/linux_runqueues.h[marcel_structures]"
#depend "scheduler-marcel/linux_runqueues.h[types]"
#depend "linux_softirq.h[marcel_structures]"
#depend "[types]"
#ifdef MARCEL_SMT_IDLE
#depend "asm/linux_atomic.h[marcel_types]"
#endif
#depend "linux_spinlock.h[types]"
#depend "sys/marcel_kthread.h[marcel_types]"

struct marcel_topo_vpdata {
	/* For VP levels */
	marcel_task_t *ksoftirqd;
	unsigned long softirq_pending;
	struct ma_tasklet_head tasklet_vec, tasklet_hi_vec;

#ifdef MA__LWPS
#  ifdef MARCEL_REMOTE_TASKLETS
	ma_spinlock_t  tasklet_lock;
#  endif
#endif
	/* For one_more_task, wait_all_tasks, etc. */
	/* marcel_end() was called */
	tbx_bool_t main_is_waiting;
	/* Number of tasks created by this VP that are still alive */
	unsigned nb_tasks;
	/* List of all threads created on this VP that are still alive */
	struct list_head all_threads;
	/* Sequence number of the last thread created on this VP, which also
	 * corresponds to the total number of threads that have been created on
	 * this VP (including dead threads). */
	int task_number;
	/* Lock for all_threads */
	ma_spinlock_t threadlist_lock;

	/* Postexit call */
#ifdef MARCEL_POSTEXIT_ENABLED
	marcel_postexit_func_t postexit_func;
	any_t postexit_arg;
	marcel_sem_t postexit_thread;
	marcel_sem_t postexit_space;
#endif /* MARCEL_POSTEXIT_ENABLED */

	int need_resched;
};

struct marcel_topo_nodedata {
	/* For NUMA levels */
	size_t allocated;
	struct list_head memory_areas;
	ma_spinlock_t memory_areas_lock;
};

#section marcel_macros
#ifdef MARCEL_POSTEXIT_ENABLED
#  define MARCEL_TOPO_VPDATA_POSTEXIT_INITIALIZER \
	.postexit_thread = MARCEL_SEM_INITIALIZER(0), \
	.postexit_space = MARCEL_SEM_INITIALIZER(1), 
#else /* MARCEL_POSTEXIT_ENABLED */
#  define MARCEL_TOPO_VPDATA_POSTEXIT_INITIALIZER
#endif /* MARCEL_POSTEXIT_ENABLED */

#define MARCEL_TOPO_VPDATA_INITIALIZER(var) { \
	.threadlist_lock = MA_SPIN_LOCK_UNLOCKED, \
	.task_number = 0, \
	.all_threads = LIST_HEAD_INIT((var)->all_threads), \
	MARCEL_TOPO_VPDATA_POSTEXIT_INITIALIZER \
}

#define MARCEL_TOPO_NODEDATA_INITIALIZER(var) { \
   .allocated = 0, \
   .memory_areas = LIST_HEAD_INIT((var)->memory_areas), \
   .memory_areas_lock = MA_SPIN_LOCK_UNLOCKED,\
}

#section marcel_structures

/** Structure of a topology level */
struct marcel_topo_level {
	enum marcel_topo_level_e type;	/**< \brief Type of level */
	unsigned level;			/**< \brief Vertical index in marcel_topo_levels */
	unsigned number;		/**< \brief Horizontal index in marcel_topo_levels[l.level] */
	unsigned index;			/**< \brief Index in fathers' children[] array */
	signed os_node;			/**< \brief OS-provided node number */
	signed os_die;			/**< \brief OS-provided die number */
	signed os_l3;			/**< \brief OS-provided L3 number */
	signed os_l2;			/**< \brief OS-provided L2 number */
	signed os_core;			/**< \brief OS-provided core number */
	signed os_cpu;			/**< \brief OS-provided CPU number */

	marcel_vpset_t vpset;		/**< \brief VPs covered by this level */
	marcel_vpset_t cpuset;		/**< \brief CPUs covered by this level */

	unsigned arity;			/**< \brief Number of children */
	struct marcel_topo_level **children;	/**< \brief Children, children[0 .. arity -1] */
	struct marcel_topo_level *father;	/**< \brief Father, NULL if root (machine level) */

#ifdef MARCEL_SMT_IDLE
	ma_atomic_t nbidle;		/**< \brief Number of currently idle SMT processors, for SMT Idleness */
#endif

	ma_runqueue_t rq;		/**< \brief data for the scheduler (runqueue for Marcel) */

#ifdef MA__SMP
	/* for LWPs/VPs management */
	marcel_kthread_mutex_t kmutex;
	marcel_kthread_cond_t kneed;
	marcel_kthread_cond_t kneeddone;
	unsigned spare;
	int needed;
#endif

	struct marcel_topo_nodedata nodedata; /* for NUMA node levels */
	struct marcel_topo_vpdata vpdata; /* for VP levels */

	/* allocated by ma_per_level_alloc() */
	char data[MA_PER_LEVEL_ROOM];
};

#define ma_topo_set_os_numbers(l, node, die, l3, l2, core, cpu) do { \
		struct marcel_topo_level *__l = (l); \
		__l->os_node = node; \
		__l->os_die  = die;  \
		__l->os_l3   = l3;  \
		__l->os_l2   = l2;  \
		__l->os_core = core; \
		__l->os_cpu  = cpu;  \
	} while(0)

#section types
/** \brief Type of a topology level */
typedef struct marcel_topo_level marcel_topo_level_t;

#section marcel_macros
#define ma_topo_vpdata_l(vp, field) ((vp)->vpdata.field)
#define ma_topo_vpdata(vpnum, field) ma_topo_vpdata_l(&marcel_topo_vp_level[vpnum], field)
#define ma_topo_vpdata_by_vpnum(vpnum) (&marcel_topo_vp_level[(vpnum)].vpdata)
#define ma_topo_vpdata_self() (ma_topo_vpdata_by_vpnum(ma_vpnum(MA_LWP_SELF)))
#define ma_topo_nodedata_l(node, field) ((node)->nodedata.field)
#define ma_topo_nodedata(nodenum, field) ma_topo_nodedata_l(&marcel_topo_node_level[nodenum], field)

#section marcel_variables
/** \brief Number of horizontal levels */
extern TBX_EXTERN unsigned marcel_topo_nblevels;
/** \brief Machine level */
extern TBX_EXTERN struct marcel_topo_level marcel_machine_level[];
/** \brief Number of items on each horizontal level */
extern TBX_EXTERN unsigned marcel_topo_level_nbitems[2*MARCEL_LEVEL_LAST+1];
/** \brief Direct access to levels, marcel_topo_levels[l = 0 .. marcel_topo_nblevels-1][0..marcel_topo_level_nbitems[l]] */
extern TBX_EXTERN struct marcel_topo_level *marcel_topo_levels[2*MARCEL_LEVEL_LAST+1];

#section functions
/** \brief indexes into ::marcel_topo_levels, but available from application */
marcel_topo_level_t *marcel_topo_level(unsigned level, unsigned index);

#section marcel_macros
/** \brief Iterate over VPs */
#define for_all_vp(vp) \
	for (vp = &marcel_topo_vp_level[0]; \
			vp < &marcel_topo_vp_level[marcel_nbvps()+MARCEL_NBMAXVPSUP]; \
			vp++)

/** \brief Iterate over VPs, starting from a given number (begin macro) */
#define for_all_vp_from_begin(vp, number) \
	vp = &marcel_topo_vp_level[number]; \
	do {

/** \brief Iterate over VPs, starting from a given number (end macro) */
#define for_all_vp_from_end(vp, number) \
		vp ++; \
		if (vp == &marcel_topo_vp_level[marcel_nbvps()]) \
			vp = &marcel_topo_vp_level[0]; \
	} while (vp != &marcel_topo_vp_level[number]);

/** \brief Iterate over VPs, except VP \e number */
#define for_vp_from(vp, number) \
	for (vp = &marcel_topo_vp_level[(number+1)%marcel_nbvps()]; \
			vp != &marcel_topo_vp_level[number]; \
			vp++, ({if (vp == &marcel_topo_vp_level[marcel_nbvps()]) vp = &marcel_topo_vp_level[0]; }))
#define ma_per_vp(vp, field) (marcel_topo_vp[vp].(field))

#section marcel_functions
#ifdef MARCEL_SMT_IDLE
/*  return whether one should sleep (because other siblings are working) */
static __tbx_inline__ void ma_topology_lwp_idle_start(ma_lwp_t lwp);
static __tbx_inline__  int ma_topology_lwp_idle_core(ma_lwp_t lwp);
static __tbx_inline__ void ma_topology_lwp_idle_end(ma_lwp_t lwp);
#else
#define ma_topology_lwp_idle_start(lwp) (void)0
#define ma_topology_lwp_idle_core(lwp) 1
#define ma_topology_lwp_idle_end(lwp) (void)0
#endif

#section marcel_inline
#ifdef MARCEL_SMT_IDLE
static __tbx_inline__ void ma_topology_lwp_idle_start(ma_lwp_t lwp) {
	struct marcel_topo_level *level;
	if ((level = ma_per_lwp(core_level,lwp)))
		ma_atomic_inc(&level->nbidle);
}

static __tbx_inline__  int ma_topology_lwp_idle_core(ma_lwp_t lwp) {
	struct marcel_topo_level *level;
	if ((level = ma_per_lwp(core_level,lwp)))
		return (ma_atomic_read(&level->nbidle) == level->arity);
	return 1;
}

static __tbx_inline__ void ma_topology_lwp_idle_end(ma_lwp_t lwp) {
	struct marcel_topo_level *level;
	if ((level = ma_per_lwp(core_level,lwp)))
		ma_atomic_dec(&level->nbidle);
}
#endif

#section marcel_macros
#ifdef MARCEL_BLOCKING_ENABLED
#  define ma_set_need_resched(v) \
	do { \
		__ma_get_lwp_var(need_resched) = (v); \
		if (!ma_spare_lwp()) \
			ma_topo_vpdata_l((__ma_get_lwp_var(vp_level)), need_resched) = (v); \
	} while(0)
#  define ma_get_need_resched() \
	(__ma_get_lwp_var(need_resched) \
	 || (!ma_spare_lwp() \
	     && ma_topo_vpdata_l((__ma_get_lwp_var(vp_level)), need_resched)))
#  define ma_set_need_resched_ext(vp, lwp, v) \
	do { \
		ma_per_lwp(need_resched, lwp) = (v); \
		if (vp != -1) \
			ma_topo_vpdata(vp, need_resched) = (v); \
	} while(0)
#  define ma_get_need_resched_ext(vp, lwp) \
	(ma_per_lwp(need_resched, lwp) \
	 || (vp != -1 \
	     && ma_topo_vpdata(vp, need_resched)))
#else
#  define ma_set_need_resched(v) \
	do { \
		ma_topo_vpdata_l((__ma_get_lwp_var(vp_level)), need_resched) = (v); \
	} while(0)
#  define ma_get_need_resched() \
	(ma_topo_vpdata_l((__ma_get_lwp_var(vp_level)), need_resched))
#  define ma_set_need_resched_ext(vp, lwp, v) \
	do { \
		ma_topo_vpdata(vp, need_resched) = (v); \
	} while(0)
#  define ma_get_need_resched_ext(vp, lwp) \
	 (ma_topo_vpdata(vp, need_resched))
#endif

#section functions
#depend "tbx_compiler.h"
#depend "marcel_sysdep.h[functions]"

/** \brief Allocate data on specific node */
TBX_FMALLOC extern void *marcel_malloc_node(size_t size, int node);
/** \brief Free data allocated by marcel_malloc_node() */
extern void marcel_free_node(void *ptr, size_t size, int node);
#define marcel_malloc_node(size, node)	ma_malloc_node(size, node, __FILE__, __LINE__)
#define marcel_free_node(ptr, size, node)	ma_free_node(ptr, size, __FILE__, __LINE__)
//!! unused node
/* @} */
