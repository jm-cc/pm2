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

#section common
#include "tbx_compiler.h"
/*
 * Similar to:
 * include/asm-ia64/system.h
 *
 * System defines. Note that this is included both from .c and .S
 * files, so it does only defines, not any C code.  This is based
 * on information published in the Processor Abstraction Layer
 * and the System Abstraction Layer manual.
 *
 * Copyright (C) 1998-2003 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 * Copyright (C) 1999 Asit Mallick <asit.k.mallick@intel.com>
 * Copyright (C) 1999 Don Dugger <don.dugger@intel.com>
 */
// #include <linux/config.h>



#section marcel_macros

/*
 * Macros to force memory ordering.  In these descriptions, "previous"
 * and "subsequent" refer to program order; "visible" means that all
 * architecturally visible effects of a memory access have occurred
 * (at a minimum, this means the memory has been read or written).
 *
 *   ma_wmb():	Guarantees that all preceding stores to memory-
 *		like regions are visible before any subsequent
 *		stores and that all following stores will be
 *		visible only after all previous stores.
 *   ma_rmb():	Like wmb(), but for reads.
 *   ma_mb():	wmb()/rmb() combo, i.e., all previous memory
 *		accesses are visible before all subsequent
 *		accesses and vice versa.  This is also known as
 *		a "fence."
 *
 * Note: "ma_mb()" and its variants cannot be used as a fence to order
 * accesses to memory mapped I/O registers.  For that, mf.a needs to
 * be used.  However, we don't want to always use mf.a because (a)
 * it's (presumably) much slower than mf and (b) mf.a is supported for
 * sequential memory pages only.
 */
#define ma_mb()	ma_ia64_mf()
#define ma_rmb()	ma_mb()
#define ma_wmb()	ma_mb()
#define ma_read_barrier_depends()	do { } while(0)

#ifdef MA__LWPS
# define ma_smp_mb()	ma_mb()
# define ma_smp_rmb()	ma_rmb()
# define ma_smp_wmb()	ma_wmb()
# define ma_smp_read_barrier_depends()	ma_read_barrier_depends()
#else
# define ma_smp_mb()	ma_barrier()
# define ma_smp_rmb()	ma_barrier()
# define ma_smp_wmb()	ma_barrier()
# define ma_smp_read_barrier_depends()	do { } while(0)
#endif

/*
 * XXX check on these---I suspect what Linus really wants here is
 * acquire vs release semantics but we can't discuss this stuff with
 * Linus just yet.  Grrr...
 */
#define ma_set_mb(var, value)	do { (var) = (value); ma_mb(); } while (0)
#define ma_set_wmb(var, value)	do { (var) = (value); ma_wmb(); } while (0)

//#define safe_halt()         ia64_pal_halt_light()    /* PAL_HALT_LIGHT */

#section null
#if 0
/*
 * The group barrier in front of the rsm & ssm are necessary to ensure
 * that none of the previous instructions in the same group are
 * affected by the rsm/ssm.
 */
/* For spinlocks etc */

/*
 * - clearing psr.i is implicitly serialized (visible by next insn)
 * - setting psr.i requires data serialization
 * - we need a stop-bit before reading PSR because we sometimes
 *   write a floating-point register right before reading the PSR
 *   and that writes to PSR.mfl
 */
#define __local_irq_save(x)			\
do {						\
	ia64_stop();				\
	(x) = ia64_getreg(_IA64_REG_PSR);	\
	ia64_stop();				\
	ia64_rsm(IA64_PSR_I);			\
} while (0)

#define __local_irq_disable()			\
do {						\
	ia64_stop();				\
	ia64_rsm(IA64_PSR_I);			\
} while (0)

#define __local_irq_restore(x)	ia64_intrin_local_irq_restore((x) & IA64_PSR_I)

#ifdef CONFIG_IA64_DEBUG_IRQ

  extern unsigned long last_cli_ip;

# define __save_ip()		last_cli_ip = ia64_getreg(_IA64_REG_IP)

# define local_irq_save(x)					\
do {								\
	unsigned long psr;					\
								\
	__local_irq_save(psr);					\
	if (psr & IA64_PSR_I)					\
		__save_ip();					\
	(x) = psr;						\
} while (0)

# define local_irq_disable()	do { unsigned long x; local_irq_save(x); } while (0)

# define local_irq_restore(x)					\
do {								\
	unsigned long old_psr, psr = (x);			\
								\
	local_save_flags(old_psr);				\
	__local_irq_restore(psr);				\
	if ((old_psr & IA64_PSR_I) && !(psr & IA64_PSR_I))	\
		__save_ip();					\
} while (0)

#else /* !CONFIG_IA64_DEBUG_IRQ */
# define local_irq_save(x)	__local_irq_save(x)
# define local_irq_disable()	__local_irq_disable()
# define local_irq_restore(x)	__local_irq_restore(x)
#endif /* !CONFIG_IA64_DEBUG_IRQ */

#define local_irq_enable()	({ ia64_ssm(IA64_PSR_I); ia64_srlz_d(); })
#define local_save_flags(flags)	({ ia64_stop(); (flags) = ia64_getreg(_IA64_REG_PSR); })

#define irqs_disabled()				\
({						\
	unsigned long __ia64_id_flags;		\
	local_save_flags(__ia64_id_flags);	\
	(__ia64_id_flags & IA64_PSR_I) == 0;	\
})

//#ifdef __KERNEL__

#define prepare_to_switch()    do { } while(0)

#ifdef CONFIG_IA32_SUPPORT
# define IS_IA32_PROCESS(regs)	(ia64_psr(regs)->is != 0)
#else
# define IS_IA32_PROCESS(regs)		0
struct task_struct;
static __tbx_inline__ void ia32_save_state(struct task_struct *t __attribute__((unused))){}
static __tbx_inline__ void ia32_load_state(struct task_struct *t __attribute__((unused))){}
#endif

/*
 * Context switch from one thread to another.  If the two threads have
 * different address spaces, schedule() has already taken care of
 * switching to the new address space by calling switch_mm().
 *
 * Disabling access to the fph partition and the debug-register
 * context switch MUST be done before calling ia64_switch_to() since a
 * newly created thread returns directly to
 * ia64_ret_from_syscall_clear_r8.
 */
extern struct task_struct *ia64_switch_to (void *next_task);

struct task_struct;

extern void ia64_save_extra (struct task_struct *task);
extern void ia64_load_extra (struct task_struct *task);

#ifdef CONFIG_PERFMON
  DECLARE_PER_CPU(unsigned long, pfm_syst_info);
# define PERFMON_IS_SYSWIDE() (__get_cpu_var(pfm_syst_info) & 0x1)
#else
# define PERFMON_IS_SYSWIDE() (0)
#endif

#define IA64_HAS_EXTRA_STATE(t)							\
	((t)->thread.flags & (IA64_THREAD_DBG_VALID|IA64_THREAD_PM_VALID)	\
	 || IS_IA32_PROCESS(ia64_task_regs(t)) || PERFMON_IS_SYSWIDE())

#define __switch_to(prev,next,last) do {							 \
	if (IA64_HAS_EXTRA_STATE(prev))								 \
		ia64_save_extra(prev);								 \
	if (IA64_HAS_EXTRA_STATE(next))								 \
		ia64_load_extra(next);								 \
	ia64_psr(ia64_task_regs(next))->dfh = !ia64_is_local_fpu_owner(next);			 \
	(last) = ia64_switch_to((next));							 \
} while (0)

#ifdef CONFIG_SMP
/*
 * In the SMP case, we save the fph state when context-switching away from a thread that
 * modified fph.  This way, when the thread gets scheduled on another CPU, the CPU can
 * pick up the state from task->thread.fph, avoiding the complication of having to fetch
 * the latest fph state from another CPU.  In other words: eager save, lazy restore.
 */
# define switch_to(prev,next,last) do {						\
	if (ia64_psr(ia64_task_regs(prev))->mfh && ia64_is_local_fpu_owner(prev)) {				\
		ia64_psr(ia64_task_regs(prev))->mfh = 0;			\
		(prev)->thread.flags |= IA64_THREAD_FPH_VALID;			\
		__ia64_save_fpu((prev)->thread.fph);				\
	}									\
	__switch_to(prev, next, last);						\
} while (0)
#else
# define switch_to(prev,next,last)	__switch_to(prev, next, last)
#endif
#endif /* 0 */

#section marcel_macros
/*
 * On IA-64, we don't want to hold the runqueue's lock during the low-level context-switch,
 * because that could cause a deadlock.  Here is an example by Erich Focht:
 *
 * Example:
 * CPU#0:
 * schedule()
 *    -> spin_lock_irq(&rq->lock)
 *    -> context_switch()
 *       -> wrap_mmu_context()
 *          -> read_lock(&tasklist_lock)
 *
 * CPU#1:
 * sys_wait4() or release_task() or forget_original_parent()
 *    -> write_lock(&tasklist_lock)
 *    -> do_notify_parent()
 *       -> wake_up_parent()
 *          -> try_to_wake_up()
 *             -> spin_lock_irq(&parent_rq->lock)
 *
 * If the parent's rq happens to be on CPU#0, we'll wait for the rq->lock
 * of that CPU which will not be released, because there we wait for the
 * tasklist_lock to become available.
 */
#if 0
  /* On n'a pas ces problèmes avec marcel. Pas besoin de version spécifique ici
   * */
#define ma_prepare_arch_switch(rq, next)		\
do {						\
	ma_spin_lock(&(next)->switch_lock);	\
	ma_spin_unlock(&(rq)->lock);		\
} while (0)
#define ma_finish_arch_switch(rq, prev)	ma_spin_unlock_irq(&(prev)->switch_lock)
#define ma_task_running(rq, p) 		((rq)->curr == (p) || ma_spin_is_locked(&(p)->switch_lock))
#endif

#define ma_ia64_platform_is(x) (strcmp(x, platform_name) == 0)

/* From processor.h */
#define ma_cpu_relax()    ma_ia64_hint(ma_ia64_hint_pause)
