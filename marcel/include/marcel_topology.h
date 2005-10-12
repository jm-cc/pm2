
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
#ifndef MA__LWPS
#define marcel_nbprocessors 1
#endif

#section marcel_variables
#depend "sys/marcel_lwp.h[marcel_macros]"
extern int ma_lwp_node[MA_NR_LWPS];

#section functions
extern void ma_set_nbprocessors(void);
extern void ma_topo_exit(void);
#ifndef MA__LWPS
#define ma_set_nbprocessors() (void)0
#define ma_topo_exit() (void)0
#endif

#section types
enum marcel_topo_level_t {
	MARCEL_LEVEL_MACHINE,
#ifndef MA__LWPS
#define MARCEL_LEVEL_LAST MARCEL_LEVEL_MACHINE
#else
#ifdef MA__NUMA
	MARCEL_LEVEL_NODE,
	MARCEL_LEVEL_DIE,
	MARCEL_LEVEL_CORE,
#endif
	MARCEL_LEVEL_PROC,
#define MARCEL_LEVEL_LAST MARCEL_LEVEL_PROC
#endif
};

#section types
#ifdef MA__LWPS
typedef unsigned long ma_cpu_set_t;
#else
typedef unsigned ma_cpu_set_t;
#endif

#section macros
#depend "linux_bitops.h[]"
#ifdef MA__LWPS
#define MA_CPU_EMPTY			(0UL)
#define MA_CPU_FULL			(~0UL)
#define MA_CPU_FILL(cpusetp)		(*(cpusetp)=MA_CPU_FULL)
#define MA_CPU_SET(cpu, cpusetp)	(*(cpusetp)|=1UL<<(cpu))
#define MA_CPU_CLR(cpu, cpusetp)	(*(cpusetp)&=~(1UL<<(cpu)))
#define MA_CPU_ISSET(cpu, cpusetp)	((*(cpusetp)&(1UL<<(cpu))) != 0)
#define MA_CPU_WEIGHT(cpusetp)		ma_hweight_long(*(cpusetp))
#else
#define MA_CPU_EMPTY			(0U)
#define MA_CPU_FULL			(~0U)
#define MA_CPU_FILL(cpusetp)		(*(cpusetp)=1)
#define MA_CPU_SET(cpu, cpusetp)	MA_CPU_FILL(cpusetp)
#define MA_CPU_CLR(cpu, cpusetp)	MA_CPU_ZERO(cpusetp)
#define MA_CPU_ISSET(cpu, cpusetp)	(*(cpusetp))
#define MA_CPU_WEIGHT(cpusetp)		(*(cpusetp))
#endif
#define MA_CPU_ZERO(cpusetp)		(*(cpusetp)=MA_CPU_EMPTY)

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

	ma_cpu_set_t cpuset;

	unsigned arity;
	struct marcel_topo_level **sons;
	struct marcel_topo_level *father;

#ifdef MARCEL_SMT_IDLE
	ma_atomic_t nbidle;
#endif

	ma_topo_level_schedinfo *sched;
};

#section variables
extern unsigned marcel_topo_nblevels;
extern struct marcel_topo_level marcel_machine_level[];
extern struct marcel_topo_level *marcel_topo_levels[MARCEL_LEVEL_LAST+1];

#section marcel_variables
#ifdef MARCEL_SMT_IDLE
MA_DECLARE_PER_LWP(struct marcel_topo_level *, core_level);
#endif

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

#section marcel_macros
#ifdef MA__LWPS
#define MARCEL_NBMAXCPUS	(sizeof(unsigned long)*8)
#ifdef MA__NUMA
#define MARCEL_NBMAXCORES	16
#define MARCEL_NBMAXDIES	8
#define MARCEL_NBMAXNODES	8
#endif
#endif

#section functions
#depend "tbx_compiler.h"

TBX_FMALLOC extern void *ma_malloc_node(unsigned size, int node, char *file,  unsigned line);
extern void ma_free_node(void *ptr, unsigned size, int node, char *file,  unsigned line);
TBX_FMALLOC extern void *marcel_malloc_node(unsigned size, int node);
extern void marcel_free_node(void *ptr, unsigned size, int node);
#ifndef MA__NUMA
#define ma_malloc_node(size, node, file, line) marcel_malloc(size, file, line)
#define ma_free_node(ptr, size, node, file, line) marcel_free(ptr, file, line)
#endif
#define marcel_malloc_node(size, node)	ma_malloc_node(size, node, __FILE__, __LINE__)
#define marcel_free_node(ptr, size, node)	ma_free_node(ptr, size, node, __FILE__, __LINE__)
#section marcel_types
