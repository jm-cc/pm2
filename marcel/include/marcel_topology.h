
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

#section common
#include "tbx_compiler.h"

#section variables
extern unsigned marcel_nbprocessors;
extern unsigned marcel_cpu_stride;
extern unsigned marcel_lwps_per_cpu;
#ifdef MA__NUMA
extern unsigned marcel_topo_max_arity;
#endif
#ifndef MA__LWPS
#define marcel_nbprocessors 1
#define marcel_cpu_stride 1
#define marcel_lwps_per_cpu 1
#endif
#define ma_cpu_of_lwp_num(num) (((num)/marcel_lwps_per_cpu)*marcel_cpu_stride)

#section marcel_variables
#depend "sys/marcel_lwp.h[marcel_macros]"
extern int ma_lwp_node[MA_NR_LWPS];

#section functions
extern void ma_set_nbprocessors(void);
extern void ma_set_processors(void);
extern void ma_topo_exit(void);
#ifndef MA__LWPS
#define ma_set_nbprocessors() (void)0
#define ma_set_processors() (void)0
#define ma_topo_exit() (void)0
#endif

#section types
enum marcel_topo_level_t {
	MARCEL_LEVEL_MACHINE,
#ifndef MA__LWPS
#define MARCEL_LEVEL_LAST MARCEL_LEVEL_MACHINE
#else
	MARCEL_LEVEL_FAKE,
#ifdef MA__NUMA
	MARCEL_LEVEL_NODE,
	MARCEL_LEVEL_DIE,
	MARCEL_LEVEL_CORE,
	MARCEL_LEVEL_PROC,
#endif
	MARCEL_LEVEL_LWP,
#define MARCEL_LEVEL_LAST MARCEL_LEVEL_LWP
#endif
};

/****************************************************************/
/*               Virtual Processors                             */
/****************************************************************/

#section types
/* VP mask: useful for selecting the set of "forbiden" LWP for a given thread */
#ifdef MA__LWPS
typedef unsigned long marcel_vpmask_t;
#else
typedef unsigned marcel_vpmask_t;
#endif

#section macros

// Primitives & macros de construction de "masques" de processeurs
// virtuels. 

// ATTENTION : le placement d un thread est autorise sur un 'vp' si le
// bit correspondant est a _ZERO_ dans le masque (similitude avec
// sigset_t pour la gestion des masques de signaux).

#ifdef MA__LWPS
#define MARCEL_VPMASK_EMPTY          ((marcel_vpmask_t)0UL)
#define MARCEL_VPMASK_FULL           ((marcel_vpmask_t)~0UL)
#define MARCEL_VPMASK_ONLY_VP(vp)    ((marcel_vpmask_t)(1UL << (vp)))
#define MARCEL_VPMASK_ALL_BUT_VP(vp) ((marcel_vpmask_t)(~(1UL << (vp))))
#else
#define MARCEL_VPMASK_EMPTY          ((marcel_vpmask_t)0U)
#define MARCEL_VPMASK_FULL           ((marcel_vpmask_t)~0U)
#define MARCEL_VPMASK_ONLY_VP(vp)    ((marcel_vpmask_t)(1U << (vp)))
#define MARCEL_VPMASK_ALL_BUT_VP(vp) ((marcel_vpmask_t)(~(1U << (vp))))
#endif

#define marcel_vpmask_init(m)        marcel_vpmask_empty(m)

#section functions
static __tbx_inline__ void marcel_vpmask_empty(marcel_vpmask_t *mask);
#section inline
static __tbx_inline__ void marcel_vpmask_empty(marcel_vpmask_t *mask)
{
  *mask = MARCEL_VPMASK_EMPTY;
}
#section functions
static __tbx_inline__ void marcel_vpmask_fill(marcel_vpmask_t *mask);
#section inline
static __tbx_inline__ void marcel_vpmask_fill(marcel_vpmask_t *mask)
{
  *mask = MARCEL_VPMASK_FULL;
}
#section functions
static __tbx_inline__ void marcel_vpmask_add_vp(marcel_vpmask_t *mask,
					    unsigned vp);
#section inline
static __tbx_inline__ void marcel_vpmask_add_vp(marcel_vpmask_t *mask,
					    unsigned vp)
{
#ifdef MA__LWPS
  *mask |= 1U << vp;
#else
  marcel_vpmask_fill(mask);
#endif
}

#section functions
static __tbx_inline__ void marcel_vpmask_only_vp(marcel_vpmask_t *mask,
					     unsigned vp);
#section inline
static __tbx_inline__ void marcel_vpmask_only_vp(marcel_vpmask_t *mask,
					     unsigned vp)
{
#ifdef MA__LWPS
  *mask = 1U << vp;
#else
  marcel_vpmask_fill(mask);
#endif
}

#section functions
static __tbx_inline__ void marcel_vpmask_del_vp(marcel_vpmask_t *mask,
					    unsigned vp);
#section inline
static __tbx_inline__ void marcel_vpmask_del_vp(marcel_vpmask_t *mask,
					    unsigned vp)
{
#ifdef MA__LWPS
  *mask &= ~(1U << vp);
#else
  marcel_vpmask_empty(mask);
#endif
}

#section functions
static __tbx_inline__ void marcel_vpmask_all_but_vp(marcel_vpmask_t *mask,
						unsigned vp);
#section inline
static __tbx_inline__ void marcel_vpmask_all_but_vp(marcel_vpmask_t *mask,
						unsigned vp)
{
#ifdef MA__LWPS
  *mask = ~(1U << vp);
#else
  marcel_vpmask_empty(mask);
#endif
}

#section functions
static __tbx_inline__ int marcel_vpmask_vp_ismember(marcel_vpmask_t *mask,
						unsigned vp);
#section inline
static __tbx_inline__ int marcel_vpmask_vp_ismember(marcel_vpmask_t *mask,
						unsigned vp)
{
#ifdef MA__LWPS
  return 1 & (*mask >> vp);
#else
  return *mask;
#endif
}

#section marcel_functions
static __tbx_inline__ int marcel_vpmask_weight(marcel_vpmask_t *mask);
#section marcel_inline
#depend "linux_bitops.h[marcel_inline]"
static __tbx_inline__ int marcel_vpmask_weight(marcel_vpmask_t *mask)
{
#ifdef MA__LWPS
  return ma_hweight_long(*mask);
#else
  return *mask;
#endif
}

#section functions
static __tbx_inline__ unsigned marcel_current_vp(void);
#section inline
#depend "sys/marcel_lwp.h[marcel_variables]"
static __tbx_inline__ unsigned marcel_current_vp(void)
{
  return LWP_NUMBER(LWP_SELF);
}

#section structures
#ifdef PM2_DEV
// #warning il ne faudrait pas dépendre d un scheduler particulier
#endif
#depend "scheduler-marcel/linux_runqueues.h[types]"
#depend "[types]"
#ifdef MARCEL_SMT_IDLE
#depend "asm/linux_atomic.h[marcel_types]"
#endif
struct marcel_topo_level {
	enum marcel_topo_level_t type;
	unsigned number; /* for whole machine */
	unsigned index; /* in father array */

	marcel_vpmask_t vpset;

	unsigned arity;
	struct marcel_topo_level **sons;
	struct marcel_topo_level *father;

#ifdef MARCEL_SMT_IDLE
	ma_atomic_t nbidle;
#endif

	ma_topo_level_schedinfo *sched;

	char data[MA_PER_LEVEL_ROOM];
};

#section variables
extern unsigned marcel_topo_nblevels;
extern struct marcel_topo_level marcel_machine_level[];
extern struct marcel_topo_level *marcel_topo_levels[2*MARCEL_LEVEL_LAST+1];

#section functions
#depend "[variables]"
static __tbx_inline__ unsigned marcel_topo_arity(unsigned level) {
	return marcel_topo_levels[level][0].arity;
}

#section marcel_functions
#ifdef MARCEL_SMT_IDLE
// return whether one should sleep (because other siblings are working)
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

#section functions
#depend "tbx_compiler.h"
#depend "marcel_sysdep.h[marcel_functions]"

TBX_FMALLOC extern void *marcel_malloc_node(size_t size, int node);
extern void marcel_free_node(void *ptr, size_t size, int node);
#define marcel_malloc_node(size, node)	ma_malloc_node(size, node, __FILE__, __LINE__)
#define marcel_free_node(ptr, size, node)	ma_free_node(ptr, size, node, __FILE__, __LINE__)

#section marcel_functions
extern unsigned ma_per_level_malloc(size_t size);
extern void *ma_per_level_data(struct marcel_topo_level *level, unsigned i);
#define ma_per_level_data(l, i) (&(l)->data[i])

// Pour l'instant...
#define ma_per_level_malloc(s) 0
