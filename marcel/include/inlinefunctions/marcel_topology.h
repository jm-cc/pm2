/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 the PM2 team (see AUTHORS file)
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


#ifndef __INLINEFUNCTIONS_MARCEL_TOPOLOGY_H__
#define __INLINEFUNCTIONS_MARCEL_TOPOLOGY_H__


#include "marcel_topology.h"


/** Public inline **/
static __tbx_inline__ void marcel_vpset_zero(marcel_vpset_t * set)
{
	int i;
	for(i=0; i<MA_VPSUBSET_COUNT; i++)
		MA_VPSUBSET_SUBSET(*set,i) = MA_VPSUBSET_ZERO;
}

#define PMARCEL_CPU_ZERO(cpusetp) marcel_vpset_zero(cpusetp)
#define PMARCEL_CPU_ZERO_S(setsize, cpusetp) ({ MA_BUG_ON(setsize > PMARCEL_CPU_ZEROSIZE); PMARCEL_CPU_ZERO(cpusetp); })

static __tbx_inline__ void marcel_vpset_fill(marcel_vpset_t * set)
{
	int i;
	for(i=0; i<MA_VPSUBSET_COUNT; i++)
		MA_VPSUBSET_SUBSET(*set,i) = MA_VPSUBSET_FULL;
}

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

static __tbx_inline__ int marcel_vpset_intersect (const marcel_vpset_t *set1,
						  const marcel_vpset_t *set2)
{
	int i;
	for(i=0; i<MA_VPSUBSET_COUNT; i++)
		if (MA_VPSUBSET_SUBSET(*set1,i) & MA_VPSUBSET_SUBSET(*set2,i))
			return 1;
	return 0;
}

static __tbx_inline__ int marcel_vpset_isequal (const marcel_vpset_t *set1,
						const marcel_vpset_t *set2)
{
	int i;
	for(i=0; i<MA_VPSUBSET_COUNT; i++)
		if (MA_VPSUBSET_SUBSET(*set1,i) != MA_VPSUBSET_SUBSET(*set2,i))
			return 0;
	return 1;
}

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

static __tbx_inline__ void marcel_vpset_orset (marcel_vpset_t *set,
					       const marcel_vpset_t *modifier_set)
{
	int i;
	for(i=0; i<MA_VPSUBSET_COUNT; i++)
		MA_VPSUBSET_SUBSET(*set,i) |= MA_VPSUBSET_SUBSET(*modifier_set,i);
}

static __tbx_inline__ void marcel_vpset_andset (marcel_vpset_t *set,
						const marcel_vpset_t *modifier_set)
{
	int i;
	for(i=0; i<MA_VPSUBSET_COUNT; i++)
		MA_VPSUBSET_SUBSET(*set,i) &= MA_VPSUBSET_SUBSET(*modifier_set,i);
}

static __tbx_inline__ void marcel_vpset_clearset (marcel_vpset_t *set,
						  const marcel_vpset_t *modifier_set)
{
	int i;
	for(i=0; i<MA_VPSUBSET_COUNT; i++)
		MA_VPSUBSET_SUBSET(*set,i) &= ~MA_VPSUBSET_SUBSET(*modifier_set,i);
}


#ifdef __MARCEL_KERNEL__


/** Internal inline functions **/
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

static __tbx_inline__ int __marcel_current_vp(void)
{
	return ma_vpnum(MA_LWP_SELF);
}
#define marcel_current_vp __marcel_current_vp

static __tbx_inline__ struct marcel_topo_level * marcel_current_vp_level(void)
{
	int vp = marcel_current_vp();
	if (vp == -1)
		return NULL;
	return &marcel_topo_vp_level[vp];
}

#ifdef MARCEL_SMT_IDLE
static __tbx_inline__ void ma_topology_lwp_idle_start(ma_lwp_t lwp) {
	struct marcel_topo_level *level;
	int vpnum = ma_vpnum(lwp);
	if (vpnum >= 0 && (level = ma_vp_core_level[vpnum]) && level->arity)
		ma_atomic_inc(&level->nbidle);
}

static __tbx_inline__  int ma_topology_lwp_idle_core(ma_lwp_t lwp) {
	struct marcel_topo_level *level;
	int vpnum = ma_vpnum(lwp);
	if (vpnum >= 0 && (level = ma_vp_core_level[vpnum]) && level->arity)
		return (ma_atomic_read(&level->nbidle) == level->arity);
	return 1;
}

static __tbx_inline__ void ma_topology_lwp_idle_end(ma_lwp_t lwp) {
	struct marcel_topo_level *level;
	int vpnum = ma_vpnum(lwp);
	if (vpnum >= 0 && (level = ma_vp_core_level[vpnum]) && level->arity)
		ma_atomic_dec(&level->nbidle);
}
#endif


#endif /** __MARCEL_KERNEL__ **/


#endif /** __INLINEFUNCTIONS_MARCEL_TOPOLOGY_H__ **/
