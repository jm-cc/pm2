
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
 * f(marcel_current_vp_level(), NULL);
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
#include <stdio.h>
#include <string.h>
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
/** \brief Number of NUMA nodes */
extern unsigned marcel_nbnodes;
#ifdef MA__NUMA
/** \brief Maximum allowed arity in the level tree */
extern unsigned marcel_topo_max_arity;
/** \brief Is merge needed */
extern unsigned marcel_topo_merge;
/** \brief Direct access to node levels */
extern struct marcel_topo_level *marcel_topo_node_level;
/** \brief Direct access to core levels */
extern struct marcel_topo_level *marcel_topo_core_level;
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

#ifdef MA__NUMA
/** \brief VP number to node number conversion array */
extern struct marcel_topo_level *ma_vp_die_level[MA_NR_VPS];
extern struct marcel_topo_level *ma_vp_node_level[MA_NR_VPS];
#else
#define ma_vp_die_level marcel_machine_level
#define ma_vp_node_level marcel_machine_level
#endif

#section functions
/** \brief Free all topology levels but the last one */
extern void ma_topo_exit(void);
#ifndef MA__LWPS
#  define ma_topo_exit() (void)0
#endif

#section types
/** \brief Type of topology level
 *
 * To be stored in the type field of marcel_topo_level structures, this is
 * _not_ an index in marcel_topo_levels[].
 */
enum marcel_topo_level_e {
	MARCEL_LEVEL_MACHINE,	/**< \brief Whole machine */
#ifndef MA__LWPS
#  define MARCEL_LEVEL_LAST MARCEL_LEVEL_MACHINE
#  define MARCEL_LEVEL_VP MARCEL_LEVEL_MACHINE
#  define MARCEL_LEVEL_NODE MARCEL_LEVEL_MACHINE
#else
#  ifdef MA__NUMA
	MARCEL_LEVEL_FAKE,	/**< \brief Fake level for meeting the marcel_topo_max_arity constraint */
	MARCEL_LEVEL_MISC,	/**< \brief Misc kinds of {OS,machine}-dependant levels */
	MARCEL_LEVEL_NODE,	/**< \brief NUMA node */
	MARCEL_LEVEL_DIE,	/**< \brief Physical chip */
	MARCEL_LEVEL_L3,	/**< \brief L3 cache */
	MARCEL_LEVEL_L2,	/**< \brief L2 cache */
	MARCEL_LEVEL_CORE,	/**< \brief Core */
	MARCEL_LEVEL_L1,	/**< \brief L1 cache */
	MARCEL_LEVEL_PROC,	/**< \brief SMT Processor in a core */
#  endif
	MARCEL_LEVEL_VP,	/**< \brief Virtual Processor (\b not SMT) */
#  define MARCEL_LEVEL_LAST MARCEL_LEVEL_VP
#endif
};

#ifdef MA__NUMA
/** \brief Type of memory attached to the topology level */
enum marcel_topo_level_memory_type_e {
	MARCEL_TOPO_LEVEL_MEMORY_L1 = 0,
	MARCEL_TOPO_LEVEL_MEMORY_L2 = 1,
	MARCEL_TOPO_LEVEL_MEMORY_L3 = 2,
	MARCEL_TOPO_LEVEL_MEMORY_NODE = 3,
	MARCEL_TOPO_LEVEL_MEMORY_MACHINE = 4,
	MARCEL_TOPO_LEVEL_MEMORY_TYPE_MAX
};
#endif

/****************************************************************/
/*               Virtual Processors                             */
/****************************************************************/

#section types
#include <stdint.h>

#ifndef MARCEL_NBMAXCPUS
#error MARCEL_NBMAXCPUS undefined!
#endif
#if !MARCEL_NBMAXCPUS
#error MARCEL_NBMAXCPUS is zero!
#endif

#ifdef MA__NUMA
#ifndef MARCEL_NBMAXNODES
#error MARCEL_NBMAXNODES undefined!
#endif
#if !MARCEL_NBMAXNODES
#error MARCEL_NBMAXNODES is zero!
#endif
#endif

#if !defined(MA__LWPS) || MARCEL_NBMAXCPUS <= 32
/** \brief Virtual processor set: defines the set of "allowed" LWP for a given thread */
typedef uint32_t marcel_vpset_t; /* FIXME: uint_fast32_t if available? */
/** \brief Set with no or all LWP selected. */
#    define MARCEL_VPSET_ZERO		((marcel_vpset_t) 0U)
#    define MARCEL_VPSET_FULL		((marcel_vpset_t) ~0U)
/** \brief Set with only \e vp in selected. */
#    define MARCEL_VPSET_VP(vp)		((marcel_vpset_t)(1U<<(vp)))
/** \brief Format string snippet suitable for the vpset datatype */
#    define MARCEL_PRIxVPSET			"08x"
#    define MARCEL_VPSET_PRINTF_VALUE(x)	((unsigned)(x))

#elif MARCEL_NBMAXCPUS <= 64
typedef uint64_t marcel_vpset_t;
#    define MARCEL_VPSET_ZERO		((marcel_vpset_t) 0ULL)
#    define MARCEL_VPSET_FULL		((marcel_vpset_t) ~0ULL)
#    define MARCEL_VPSET_VP(vp)		((marcel_vpset_t)(1ULL<<(vp)))
#    define MARCEL_PRIxVPSET			"016llx"
#    define MARCEL_VPSET_PRINTF_VALUE(x)	((unsigned long long)(x))

#else /* large vpset using an array of unsigned long subsets */
#    define MA_HAVE_VPSUBSET		1 /* marker for VPSUBSET being enabled */
/* size and count of subsets within a set */
#    define MA_VPSUBSET_SIZE		(8*sizeof(long))
#    define MA_VPSUBSET_COUNT		((MARCEL_NBMAXCPUS+MA_VPSUBSET_SIZE-1)/MA_VPSUBSET_SIZE)
typedef struct { unsigned long s[MA_VPSUBSET_COUNT]; } marcel_vpset_t;
/* extract a subset from a set using an index or a vp */
#    define MA_VPSUBSET_SUBSET(set,x)	((set).s[x])
#    define MA_VPSUBSET_INDEX(vp)	((vp)/(MA_VPSUBSET_SIZE))
#    define MA_VPSUBSET_VPSUBSET(set,vp)	MA_VPSUBSET_SUBSET(set,MA_VPSUBSET_INDEX(vp))
/* predefined subset values */
#    define MA_VPSUBSET_VAL(vp)		(1UL<<((vp)%(MA_VPSUBSET_SIZE)))
#    define MA_VPSUBSET_ZERO		0UL
#    define MA_VPSUBSET_FULL		~0UL
/* actual whole-vpset values */
#    define MARCEL_VPSET_ZERO		(marcel_vpset_t){ .s[0 ... MA_VPSUBSET_COUNT-1] = MA_VPSUBSET_ZERO }
#    define MARCEL_VPSET_FULL		(marcel_vpset_t){ .s[0 ... MA_VPSUBSET_COUNT-1] = MA_VPSUBSET_FULL }
#    define MARCEL_VPSET_VP(vp)		({ marcel_vpset_t __set = MARCEL_VPSET_ZERO; MA_VPSUBSET_VPSUBSET(__set,vp) = MA_VPSUBSET_VAL(vp); __set; })
/* displaying vpsets */
#if MA_BITS_PER_LONG == 32
#    define MA_PRIxVPSUBSET		"%08lx"
#else
#    define MA_PRIxVPSUBSET		"%016lx"
#endif
#    define MA_VPSUBSET_STRING_LENGTH	(MA_BITS_PER_LONG/4)
#    define MARCEL_VPSET_STRING_LENGTH	(MA_VPSUBSET_COUNT*(MA_VPSUBSET_STRING_LENGTH+1))
#    define MARCEL_PRIxVPSET		"s"
#    define MARCEL_VPSET_PRINTF_VALUE(x)	({			\
	char *__buf = alloca(MARCEL_VPSET_STRING_LENGTH+1);		\
	char *__tmp = __buf;						\
	int __i;							\
	for(__i=MA_VPSUBSET_COUNT-1; __i>=0; __i--)			\
	  __tmp += sprintf(__tmp, MA_PRIxVPSUBSET ",", (x).s[__i]);	\
	*(__tmp-1) = '\0';						\
	__buf;								\
     })
#endif /* large vpset using an array of unsigned long subsets */

#ifndef MA_HAVE_VPSUBSET
#   define MA_VPSUBSET_SIZE 		0 /* useless */
#   define MA_VPSUBSET_COUNT		1
#   define MA_VPSUBSET_SUBSET(set,x)	(set)
#   define MA_VPSUBSET_VPSUBSET(set,vp)	(set)
#   define MA_VPSUBSET_VAL(vp)		MARCEL_VPSET_VP(vp)
#   define MA_VPSUBSET_ZERO		MARCEL_VPSET_ZERO
#   define MA_VPSUBSET_FULL		MARCEL_VPSET_FULL
#endif

typedef marcel_vpset_t pmarcel_cpu_set_t;

#define PMARCEL_CPU_SETSIZE MARCEL_NBMAXCPUS

#section functions

/*  Primitives & macros for building "sets" of virtual processors. */

/** \brief Initialize VP set */
void marcel_vpset_init(marcel_vpset_t * set);
#define marcel_vpset_init(m)        marcel_vpset_zero(m)

#section functions
/** \brief Empty VP set */
static __tbx_inline__ void marcel_vpset_zero(marcel_vpset_t * set);
#section inline
static __tbx_inline__ void marcel_vpset_zero(marcel_vpset_t * set)
{
	int i;
	for(i=0; i<MA_VPSUBSET_COUNT; i++)
		MA_VPSUBSET_SUBSET(*set,i) = MA_VPSUBSET_ZERO;
}

#define PMARCEL_CPU_ZERO(cpusetp) marcel_vpset_zero(cpusetp)
#define PMARCEL_CPU_ZERO_S(setsize, cpusetp) ({ MA_BUG_ON(setsize > PMARCEL_CPU_ZEROSIZE); PMARCEL_CPU_ZERO(cpusetp); })

#section functions
/** \brief Fill VP set */
static __tbx_inline__ void marcel_vpset_fill(marcel_vpset_t * set);
#section inline
static __tbx_inline__ void marcel_vpset_fill(marcel_vpset_t * set)
{
	int i;
	for(i=0; i<MA_VPSUBSET_COUNT; i++)
		MA_VPSUBSET_SUBSET(*set,i) = MA_VPSUBSET_FULL;
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
#ifndef MA_HAVE_VPSUBSET
	*set = MARCEL_VPSET_VP(vp);
#else
	marcel_vpset_zero(set);
	MA_VPSUBSET_VPSUBSET(*set,vp) |= MA_VPSUBSET_VAL(vp);
#endif
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
#ifndef MA_HAVE_VPSUBSET
	*set = ~MARCEL_VPSET_VP(vp);
#else
	marcel_vpset_fill(set);
	MA_VPSUBSET_VPSUBSET(*set,vp) &= ~MA_VPSUBSET_VAL(vp);
#endif
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
#ifndef MA_HAVE_VPSUBSET
	*set |= MARCEL_VPSET_VP(vp);
#else
	MA_VPSUBSET_VPSUBSET(*set,vp) |= MA_VPSUBSET_VAL(vp);
#endif
#else
	marcel_vpset_fill(set);
#endif
}
#define PMARCEL_CPU_SET(cpu, cpusetp) marcel_vpset_set(cpusetp, cpu)
#define PMARCEL_CPU_SET_S(cpu, setsize, cpusetp) ({ MA_BUG_ON((setsize) != PMARCEL_CPU_SETSIZE); PMARCEL_CPU_SET(cpu, cpusetp); })

#section functions
/** \brief Remove VP \e vp from VP set \e set */
static __tbx_inline__ void marcel_vpset_clr(marcel_vpset_t * set,
    unsigned vp);
#section inline
static __tbx_inline__ void marcel_vpset_clr(marcel_vpset_t * set,
    unsigned vp)
{
#ifdef MA__LWPS
#ifndef MA_HAVE_VPSUBSET
	*set &= ~(MARCEL_VPSET_VP(vp));
#else
	MA_VPSUBSET_VPSUBSET(*set,vp) &= ~MA_VPSUBSET_VAL(vp);
#endif
#else
	marcel_vpset_zero(set);
#endif
}

#define PMARCEL_CPU_CLR(cpu, cpusetp) marcel_vpset_clr(cpusetp, cpu)
#define PMARCEL_CPU_CLR_S(cpu, setsize, cpusetp) ({ MA_BUG_ON((setsize) != PMARCEL_CPU_SETSIZE); PMARCEL_CPU_CLR(cpu, cpusetp); })

#section functions
/** \brief Test whether VP \e vp is part of set \e set */
static __tbx_inline__ int marcel_vpset_isset(const marcel_vpset_t * set,
    unsigned vp);
#section inline
static __tbx_inline__ int marcel_vpset_isset(const marcel_vpset_t * set,
    unsigned vp)
{
#ifdef MA__LWPS
#ifndef MA_HAVE_VPSUBSET
	return 1 & (*set >> vp);
#else
	return (MA_VPSUBSET_VPSUBSET(*set,vp) & MA_VPSUBSET_VAL(vp)) != 0;
#endif
#else
	return *set;
#endif
}

#define PMARCEL_CPU_ISSET(cpu, cpusetp) marcel_vpset_isset(cpusetp, cpu)
#define PMARCEL_CPU_ISSET_S(cpu, setsize, cpusetp) ({ MA_BUG_ON((setsize) != PMARCEL_CPU_SETSIZE); PMARCEL_CPU_ISSET(cpu, cpusetp); })

#section functions
/** \brief Test whether set \e set is zero or full */
static __tbx_inline__ int marcel_vpset_iszero(const marcel_vpset_t *set);
static __tbx_inline__ int marcel_vpset_isfull(const marcel_vpset_t *set);
#section inline
static __tbx_inline__ int marcel_vpset_iszero(const marcel_vpset_t *set)
{
	int i;
	for(i=0; i<MA_VPSUBSET_COUNT; i++)
		if (MA_VPSUBSET_SUBSET(*set,i) != MA_VPSUBSET_ZERO)
			return 0;
	return 1;
}
static __tbx_inline__ int marcel_vpset_isfull(const marcel_vpset_t *set)
{
	int i;
	for(i=0; i<MA_VPSUBSET_COUNT; i++)
		if (MA_VPSUBSET_SUBSET(*set,i) != MA_VPSUBSET_FULL)
			return 0;
	return 1;
}

#section functions
/** \brief Test whether set \e set1 is equal to set \e set2 */
static __tbx_inline__ int marcel_vpset_isequal (const marcel_vpset_t *set1,
						const marcel_vpset_t *set2);
#section inline
static __tbx_inline__ int marcel_vpset_isequal (const marcel_vpset_t *set1,
						const marcel_vpset_t *set2)
{
	int i;
	for(i=0; i<MA_VPSUBSET_COUNT; i++)
		if (MA_VPSUBSET_SUBSET(*set1,i) != MA_VPSUBSET_SUBSET(*set2,i))
			return 0;
	return 1;
}

#section functions
/** \brief Test whether set \e sub_set is part of set \e super_set */
static __tbx_inline__ int marcel_vpset_isincluded (const marcel_vpset_t *super_set,
						   const marcel_vpset_t *sub_set);
#section inline
static __tbx_inline__ int marcel_vpset_isincluded (const marcel_vpset_t *super_set,
						   const marcel_vpset_t *sub_set)
{
#ifdef MA__LWPS
	int i;
	for(i=0; i<MA_VPSUBSET_COUNT; i++)
		if (MA_VPSUBSET_SUBSET(*super_set,i) != (MA_VPSUBSET_SUBSET(*super_set,i) | MA_VPSUBSET_SUBSET(*sub_set,i)))
			return 0;
	return 1;
#else
	return *super_set;
#endif
}

#section functions
/** \brief Or set \e modifier_set into set \e set */
static __tbx_inline__ void marcel_vpset_orset (marcel_vpset_t *set,
					       const marcel_vpset_t *modifier_set);
#section inline
static __tbx_inline__ void marcel_vpset_orset (marcel_vpset_t *set,
					       const marcel_vpset_t *modifier_set)
{
	int i;
	for(i=0; i<MA_VPSUBSET_COUNT; i++)
		MA_VPSUBSET_SUBSET(*set,i) |= MA_VPSUBSET_SUBSET(*modifier_set,i);
}

#section functions
/** \brief And set \e modifier_set into set \e set */
static __tbx_inline__ void marcel_vpset_andset (marcel_vpset_t *set,
						const marcel_vpset_t *modifier_set);
#section inline
static __tbx_inline__ void marcel_vpset_andset (marcel_vpset_t *set,
						const marcel_vpset_t *modifier_set)
{
	int i;
	for(i=0; i<MA_VPSUBSET_COUNT; i++)
		MA_VPSUBSET_SUBSET(*set,i) &= MA_VPSUBSET_SUBSET(*modifier_set,i);
}

#section functions
/** \brief Clear set \e modifier_set out of set \e set */
static __tbx_inline__ void marcel_vpset_clearset (marcel_vpset_t *set,
						  const marcel_vpset_t *modifier_set);
#section inline
static __tbx_inline__ void marcel_vpset_clearset (marcel_vpset_t *set,
						  const marcel_vpset_t *modifier_set)
{
	int i;
	for(i=0; i<MA_VPSUBSET_COUNT; i++)
		MA_VPSUBSET_SUBSET(*set,i) &= ~MA_VPSUBSET_SUBSET(*modifier_set,i);
}

#section marcel_functions
/** \brief Compute the first VP in VP mask */
static __tbx_inline__ int marcel_vpset_first(const marcel_vpset_t * vpset);
#section marcel_inline
#depend "asm/linux_bitops.h[marcel_inline]"
static __tbx_inline__ int marcel_vpset_first(const marcel_vpset_t * vpset)
{
#if (!defined MA_HAVE_VPSUBSET) && MA_BITS_PER_LONG < MARCEL_NBMAXCPUS
	/* FIXME: return ma_ffs64(*vpset)-1 */
	int i;
	i = ma_ffs(((unsigned*)vpset)[0]);
	if (i>0)
		return i-1;
	i = ma_ffs(((unsigned*)vpset)[1]);
	if (i>0)
		return i+32-1;
	return -1;
#else
	int i;
	for(i=0; i<MA_VPSUBSET_COUNT; i++) {
		int ffs = ma_ffs(MA_VPSUBSET_SUBSET(*vpset,i));
		if (ffs>0)
			return ffs - 1 + MA_VPSUBSET_SIZE*i;
	}

	return -1;
#endif
}

#section marcel_functions
/** \brief Compute the first VP in VP mask */
static __tbx_inline__ int marcel_vpset_last(const marcel_vpset_t * vpset);
#section marcel_inline
#depend "asm/linux_bitops.h[marcel_inline]"
static __tbx_inline__ int marcel_vpset_last(const marcel_vpset_t * vpset)
{
#if (!defined MA_HAVE_VPSUBSET) && MA_BITS_PER_LONG < MARCEL_NBMAXCPUS
	/* FIXME: return ma_ffs64(*vpset)-1 */
	int i;
	i = ma_fls(((unsigned*)vpset)[0]);
	if (i>0)
		return i-1;
	i = ma_fls(((unsigned*)vpset)[1]);
	if (i>0)
		return i+32-1;
	return -1;
#else
	int i;
	for(i=MA_VPSUBSET_COUNT-1; i>=0; i--) {
		int fls = ma_fls(MA_VPSUBSET_SUBSET(*vpset,i));
		if (fls>0)
			return fls - 1 + MA_VPSUBSET_SIZE*i;
	}

	return -1;
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
#if (!defined MA_HAVE_VPSUBSET) && MA_BITS_PER_LONG < MARCEL_NBMAXCPUS
	return ma_hweight64(*vpset);
#else
	int weight=0;
	int i;
	for(i=0; i<MA_VPSUBSET_COUNT; i++)
		weight += ma_hweight_long(MA_VPSUBSET_SUBSET(*vpset,i));
	return weight;
#endif
#else
	return *vpset;
#endif
}

#define PMARCEL_CPU_COUNT(cpusetp) marcel_vpset_weight(cpusetp)
#define PMARCEL_CPU_COUNT_S(setsize, cpusetp) ({ MA_BUG_ON((setsize) != PMARCEL_CPU_SETSIZE); PMARCEL_CPU_COUNT(cpusetp); })

#section marcel_macros
/** \brief Loop macro iterating on a vpset and yielding on each vp that
 *  is member of the set.
 *  Uses variables \e set (the vp set) and \e vp (the loop variable) */
#define marcel_vpset_foreach_begin(vp, vpset) \
        for (vp = 0; vp < marcel_nbvps() + MARCEL_NBMAXVPSUP; vp++) \
                if (marcel_vpset_isset(vpset, vp)) {
#define marcel_vpset_foreach_end() \
                }

#section marcel_functions
/** \brief Get the current VP number.
 *
 * Note that if preemption is enabled, this may change just after the function
 * call. Also note that this may be -1 when the current LWP is not currently
 * bound to a VP (Marcel termination or blocking system calls) */
unsigned marcel_current_vp(void);

/* Internal version, for inlining */
static __tbx_inline__ unsigned __marcel_current_vp(void);
#section marcel_inline
#depend "sys/marcel_lwp.h[variables]"
static __tbx_inline__ unsigned __marcel_current_vp(void)
{
	return ma_vpnum(MA_LWP_SELF);
}
#define marcel_current_vp __marcel_current_vp

#section marcel_functions
/** \brief Get the current VP level.
 *
 * Note that if preemption is enabled, this may change just after the function
 * call. Also note that this may be NULL when the current LWP is not currently
 * bound to a VP (Marcel termination or blocking system calls) */
static __tbx_inline__ struct marcel_topo_level * marcel_current_vp_level(void);

#section marcel_inline
static __tbx_inline__ struct marcel_topo_level * marcel_current_vp_level(void)
{
	unsigned vp = marcel_current_vp();
	if (vp == -1)
		return NULL;
	return &marcel_topo_vp_level[vp];
}

#section marcel_functions
/** \brief Get the die level of a given VP. */
struct marcel_topo_level * marcel_vp_die_level(unsigned vp);
#define marcel_vp_die_level(vp) ma_vp_die_level[vp]

/** \brief Get the current die level.
 *
 * Note that if preemption is enabled, this may change just after the function
 * call. Also note that this may be NULL when the current LWP is not currently
 * bound to a VP (Marcel termination or blocking system calls) */
struct marcel_topo_level * marcel_current_die_level(void);
#define marcel_current_die_level() marcel_vp_die_level(marcel_current_vp())

/** \brief Get the NUMA node level of a given VP. */
struct marcel_topo_level * marcel_vp_node_level(unsigned vp);
#define marcel_vp_node_level(vp) ma_vp_node_level[vp]

/** \brief Get the current NUMA node level.
 *
 * Note that if preemption is enabled, this may change just after the function
 * call. Also note that this may be NULL when the current LWP is not currently
 * bound to a VP (Marcel termination or blocking system calls) */
struct marcel_topo_level * marcel_current_node_level(void);
#define marcel_current_node_level() marcel_vp_node_level(marcel_current_vp())

/** \brief Get the current die number.
 *
 * Note that if preemption is enabled, this may change just after the function
 * call. Also note that this may be -1 when the current LWP is not currently
 * bound to a VP (Marcel termination or blocking system calls) */
unsigned marcel_current_die(void);
#define marcel_current_die() marcel_current_die_level()->number

/** \brief Get the current NUMA node number.
 *
 * Note that if preemption is enabled, this may change just after the function
 * call. Also note that this may be -1 when the current LWP is not currently
 * bound to a VP (Marcel termination or blocking system calls) */
unsigned marcel_current_node(void);
#define marcel_current_node() marcel_current_node_level()->number

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
#depend "marcel_sem.h[structures]"

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
	struct tbx_fast_list_head all_threads;
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
	struct tbx_fast_list_head memory_areas;
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
	.all_threads = TBX_FAST_LIST_HEAD_INIT((var)->all_threads), \
	MARCEL_TOPO_VPDATA_POSTEXIT_INITIALIZER \
}

#define MARCEL_TOPO_NODEDATA_INITIALIZER(var) { \
   .allocated = 0, \
   .memory_areas = TBX_FAST_LIST_HEAD_INIT((var)->memory_areas), \
   .memory_areas_lock = MA_SPIN_LOCK_UNLOCKED,\
}

#section marcel_structures

/** Structure of a topology level */
struct marcel_topo_level {
	enum marcel_topo_level_e type;	/**< \brief Type of level */
	unsigned long merged_type;	/**< \brief Pretty-printing type generated from merged levels, bitmask */
	unsigned level;			/**< \brief Vertical index in marcel_topo_levels[] */
	unsigned number;		/**< \brief Horizontal index in marcel_topo_levels[l.level] */
	unsigned index;			/**< \brief Index in fathers' children[] array */
	signed os_node;			/**< \brief OS-provided node number */
	signed os_die;			/**< \brief OS-provided die number */
	signed os_l3;			/**< \brief OS-provided L3 number */
	signed os_l2;			/**< \brief OS-provided L2 number */
	signed os_core;			/**< \brief OS-provided core number */
	signed os_l1;			/**< \brief OS-provided L1 number */
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
#ifdef PIOM_ENABLE_LTASKS
	void *piom_ltask_data;
#endif

#ifdef MA__LWPS
	/* for LWPs/VPs management */
	marcel_kthread_mutex_t kmutex;
	marcel_kthread_cond_t kneed;
	marcel_kthread_cond_t kneeddone;
	unsigned spare;
	int needed;
#endif

#ifdef MA__NUMA
	struct marcel_topo_nodedata nodedata; /* for NUMA node levels */
#endif
	struct marcel_topo_vpdata vpdata; /* for VP levels */

#ifdef MA__NUMA
	unsigned long memory_kB[MARCEL_TOPO_LEVEL_MEMORY_TYPE_MAX];
        unsigned long huge_page_free;
        unsigned long huge_page_size;
#endif

	/* allocated by ma_per_level_alloc() */
	char data[MA_PER_LEVEL_ROOM];

	/* Padding to make sure to skip to next page */
	char pad[4096];
};

#define ma_topo_set_empty_os_numbers(l) do { \
		struct marcel_topo_level *___l = (l); \
		___l->os_node = -1; \
		___l->os_die  = -1;  \
		___l->os_l3   = -1;  \
		___l->os_l2   = -1;  \
		___l->os_core = -1; \
		___l->os_l1   = -1;  \
		___l->os_cpu  = -1;  \
	} while(0)

#define ma_topo_set_os_numbers(l, _field, _val) do { \
		struct marcel_topo_level *__l = (l); \
		ma_topo_set_empty_os_numbers(l); \
		__l->os_##_field = _val; \
	} while(0)

#define ma_topo_setup_level(l, _type) do { \
		struct marcel_topo_level *__l = (l); \
		__l->type = _type; \
		__l->merged_type = 1<<_type; \
		marcel_vpset_zero(&__l->vpset); \
		marcel_vpset_zero(&__l->cpuset); \
		__l->arity = 0; \
		__l->children = NULL; \
		__l->father = NULL; \
	} while (0)

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

#section variables
/** \brief Number of horizontal levels */
extern TBX_EXTERN unsigned marcel_topo_nblevels;
/** \brief Machine level */
extern TBX_EXTERN struct marcel_topo_level marcel_machine_level[];
/** \brief Number of items on each horizontal level */
extern TBX_EXTERN unsigned marcel_topo_level_nbitems[2*MARCEL_LEVEL_LAST+1];
/** \brief Direct access to levels, marcel_topo_levels[l = 0 .. marcel_topo_nblevels-1][0..marcel_topo_level_nbitems[l]] */
extern TBX_EXTERN struct marcel_topo_level *marcel_topo_levels[2*MARCEL_LEVEL_LAST+1];

#section marcel_variables
#ifdef MA__NUMA

/** \brief A space-separated string describing a "synthetic" topology.  Each
	integer denotes the number of children attached to a each node of the
	corresponding topology level.  For example, "2 4 2" denotes a
	2-node machine with 4 dual-core CPUs.
	Only has an effect at startup-time.  */
extern TBX_EXTERN char * ma_synthetic_topology_description;

#endif /* MA__NUMA */

#section variables
#ifdef MA__NUMA

/** \brief A boolean indicating whether a fake topology is used. */
extern TBX_EXTERN tbx_bool_t marcel_use_fake_topology;

#else /* !MA__NUMA */

/* Support for fake topologies requires `MA__NUMA'.  */
# define marcel_use_fake_topology  0

#endif /* MA__NUMA */


#section functions
/** \brief indexes into ::marcel_topo_levels, but available from application */
marcel_topo_level_t *marcel_topo_level(unsigned level, unsigned index);

/** \brief return a stringified topology level type */
const char * marcel_topo_level_string(enum marcel_topo_level_e l);

/** \brief print a human-readable form of the given topology level */
void marcel_print_level(struct marcel_topo_level *l, FILE *output, int txt_mode, int verbose_mode,
			const char *separator, const char *indexprefix, const char* labelseparator, const char* levelterm);

/** \brief Returns the common father level to levels lvl1 and lvl2 */
marcel_topo_level_t *ma_topo_common_ancestor (marcel_topo_level_t *lvl1, marcel_topo_level_t *lvl2);

/** \brief Returns true if _level_ is inside the subtree beginning
    with _subtree_root_. */
int ma_topo_is_in_subtree (marcel_topo_level_t *subtree_root, marcel_topo_level_t *level);

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
	if ((level = ma_per_lwp(core_level,lwp)) && level->arity)
		ma_atomic_inc(&level->nbidle);
}

static __tbx_inline__  int ma_topology_lwp_idle_core(ma_lwp_t lwp) {
	struct marcel_topo_level *level;
	if ((level = ma_per_lwp(core_level,lwp)) && level->arity)
		return (ma_atomic_read(&level->nbidle) == level->arity);
	return 1;
}

static __tbx_inline__ void ma_topology_lwp_idle_end(ma_lwp_t lwp) {
	struct marcel_topo_level *level;
	if ((level = ma_per_lwp(core_level,lwp)) && level->arity)
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

#ifdef MA__NUMA
/** \brief Returns the depth of levels of type _type_. If no level of
    this type is present on the underlying architecture, the function
    returns the depth of the first "present" level we find uppon
    _type_. */
extern int ma_get_topo_type_depth (enum marcel_topo_level_e type);
#endif
//!! unused node
/* @} */
