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


#ifndef __MARCEL_H__
#define __MARCEL_H__


#ifdef __cplusplus
extern "C" {
#endif


#ifdef PUK
#include <Padico/padico_config.h>
#endif
#include "marcel_abi.h"
#include "sys/marcel_flags.h"
#include "marcel_config.h"
#include "marcel_compiler.h"
#include <pthread.h>
#if defined(MA__LIBPTHREAD) || defined(MA__PROVIDE_TLS)
#include "pthread_libc-symbols.h"
#endif

#include "asm/marcel_profile.h"
#include "asm/linux_system.h"
#include "asm/marcel_ctx.h"
#include "asm/marcel_arch_switchto.h"
#include "asm/marcel_testandset.h"
#include "asm/marcel_archdep.h"
#include "asm/marcel_compareexchange.h"
#include "asm/linux_hardirq.h"

#include "sys/marcel_privatedefs.h"
#include "sys/marcel_kthread.h"
#include "sys/marcel_work.h"
#include "sys/isomalloc_archdep.h"
#include "sys/marcel_lwp.h"
#include "sys/marcel_archsetjmp.h"

#include "linux_bitops.h"
#include "linux_interrupt.h"
#include "linux_jiffies.h"
#include "linux_linkage.h"
#include "linux_notifier.h"
#include "linux_perlwp.h"
#include "linux_preempt.h"
#include "linux_softirq.h"
#include "linux_spinlock.h"
#include "linux_thread_info.h"
#include "linux_timer.h"
#include "marcel_alias.h"
#include "marcel_alloc.h"
#include "marcel_allocator.h"
#include "marcel_attr.h"
#include "marcel_barrier.h"
#include "marcel_cond.h"
#include "marcel_container.h"
#include "marcel_debug.h"
#include "marcel_descr.h"
#include "marcel_errno.h"

#include "marcel_exception.h"
#include "marcel_fastlock.h"
#include "marcel_fortran.h"
#include "marcel_futex.h"
#include "marcel_glue_pthread.h"
#include "marcel_init.h"
#include "marcel_io.h"
#include "marcel_keys.h"
#include "marcel_locking.h"
#include "marcel_lpt_barrier.h"
#include "marcel_lpt_cond.h"
#include "marcel_lpt_mutex.h"
#include "marcel_misc.h"
#include "marcel_mutex.h"
#include "marcel_polling.h"
#include "marcel_rwlock.h"
#include "marcel_sched_generic.h"
#include "marcel_sem.h"
#include "marcel_signal.h"
#include "marcel_spin.h"
#include "marcel_stats.h"
#include "marcel_stdio.h"
#include "marcel_supervisor.h"
#include "marcel_switchto.h"
#include "marcel_sysdep.h"
#include "marcel_threads.h"
#include "marcel_timer.h"
#include "marcel_top.h"
#include "marcel_topology.h"
#include "marcel_utils.h"

#include "scheduler-marcel/linux_runqueues.h"
#include "scheduler-marcel/linux_sched.h"
#include "scheduler-marcel/marcel_bubble_browse.h"
#include "scheduler-marcel/marcel_bubble_distribution.h"
#include "scheduler-marcel/marcel_bubble_helper.h"
#include "scheduler-marcel/marcel_bubble_sched.h"
#include "scheduler-marcel/marcel_bubble_sched_interface.h"
#include "scheduler-marcel/marcel_holder.h"
#include "scheduler-marcel/marcel_sched.h"

#include "scheduler/profiles/marcel_bubble_cache.h"
#include "scheduler/profiles/marcel_bubble_mspread.h"
#include "scheduler/profiles/marcel_bubble_memaware.h"
#include "scheduler/profiles/marcel_bubble_spread.h"
#include "scheduler/profiles/marcel_bubble_explode.h"
#include "scheduler/profiles/marcel_bubble_msteal.h"
#include "scheduler/profiles/marcel_bubble_gang.h"
#include "scheduler/profiles/marcel_bubble_null.h"
#include "scheduler/profiles/marcel_bubble_steal.h"
#include "scheduler/profiles/marcel_bubble_memory.h"

#include "asm/inlinefunctions/allfunctions.h"
#include "sys/inlinefunctions/allfunctions.h"
#include "scheduler/inlinefunctions/allfunctions.h"
#include "inlinefunctions/allfunctions.h"

#include "tbx.h"
#include "pm2_common.h"
#include "pm2_profile.h"
#include "pm2_valgrind.h"


#ifdef __cplusplus
}
#endif


#endif /** __MARCEL_H__ **/
