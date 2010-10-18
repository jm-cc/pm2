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


#ifndef __INLINEFUNCTIONS_SYS_MARCEL_LWP_H__
#define __INLINEFUNCTIONS_SYS_MARCEL_LWP_H__


#include "sys/marcel_lwp.h"
#include "tbx_compiler.h"
#include "tbx_debug.h"
#include "linux_spinlock.h"
#include "marcel_topology.h"


/** Public inline **/
static __tbx_inline__ unsigned marcel_nbvps(void)
{
#ifdef MA__LWPS
	return marcel_nb_vp;
#else
	return 1;
#endif
}


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal marcel_inline **/
static __tbx_inline__ void ma_lwp_list_lock_read(void)
{
#ifdef MA__LWPS
	ma_read_lock(&__ma_lwp_list_lock);
#endif
}

/** Internal marcel_inline **/
static __tbx_inline__ void ma_lwp_list_trylock_read(void)
{
#ifdef MA__LWPS
	ma_read_trylock(&__ma_lwp_list_lock);
#endif
}

static __tbx_inline__ void ma_lwp_list_unlock_read(void)
{
#ifdef MA__LWPS
	ma_read_unlock(&__ma_lwp_list_lock);
#endif
}

static __tbx_inline__ void ma_lwp_list_lock_write(void)
{
#ifdef MA__LWPS
	ma_write_lock(&__ma_lwp_list_lock);
#endif
}

static __tbx_inline__ void ma_lwp_list_unlock_write(void)
{
#ifdef MA__LWPS
	ma_write_unlock(&__ma_lwp_list_lock);
#endif
}

/*
 * Expected number of VPs, i.e. number of CPUs or argument of --marcel-nvp
 */
static __tbx_inline__ unsigned marcel_nballvps(void)
{
#ifdef MA__LWPS
	return ma_atomic_read(&ma__last_vp) + 1;
#else
	return 1;
#endif
}

__tbx_inline__ static marcel_lwp_t *marcel_lwp_next_lwp(marcel_lwp_t * lwp)
{
	return tbx_fast_list_entry(lwp->lwp_list.next, marcel_lwp_t, lwp_list);
}

__tbx_inline__ static marcel_lwp_t *marcel_lwp_prev_lwp(marcel_lwp_t * lwp)
{
	return tbx_fast_list_entry(lwp->lwp_list.prev, marcel_lwp_t, lwp_list);
}

static __tbx_inline__ unsigned ma_lwp_os_node(marcel_lwp_t * lwp)
{
	int vp = ma_vpnum(lwp);
	struct marcel_topo_level *l;
	if (vp == -1)
		vp = 0;
	l = marcel_vp_node_level(vp);
	if (!l)
		return 0;
	return l->os_node;
}

#ifndef MA__LWPS
static __tbx_inline__ int ma_register_lwp_notifier(struct ma_notifier_block *nb
						   TBX_UNUSED)
{
	return 0;
}

static __tbx_inline__ void ma_unregister_lwp_notifier(struct ma_notifier_block *nb
						      TBX_UNUSED)
{
}
#endif				/* MA__LWPS */


#ifdef MA__LWPS
static __tbx_inline__ int ma_lwp_online(ma_lwp_t lwp)
{
	return ma_per_lwp(online, lwp);
}
#else
static __tbx_inline__ int ma_lwp_online(ma_lwp_t lwp TBX_UNUSED)
{
	return 1;
}
#endif				/* MA__LWPS */


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __INLINEFUNCTIONS_SYS_MARCEL_LWP_H__ **/
