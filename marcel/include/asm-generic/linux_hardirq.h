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


#ifndef __ASM_GENERIC_LINUX_HARDIRQ_H__
#define __ASM_GENERIC_LINUX_HARDIRQ_H__


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
/*
 * We put the hardirq and softirq counter into the preemption
 * counter. The bitmask has the following meaning:
 *
 * - bits 0-7 are the preemption count (max preemption depth: 256)
 * - bits 8-15 are the softirq count (max # of softirqs: 256)
 * - bits 16-23 are the hardirq count (max # of hardirqs: 256)
 *
 * - ( bit 26 is the PREEMPT_ACTIVE flag. )
 *
 * MA_PREEMPT_MASK: 0x000000ff
 * MA_SOFTIRQ_MASK: 0x0000ff00
 * MA_HARDIRQ_MASK: 0x00ff0000
 */
#define MA_PREEMPT_BITS	8
#define MA_SOFTIRQ_BITS	8
#define MA_HARDIRQ_BITS	8

#define MA_PREEMPT_SHIFT	0
#define MA_SOFTIRQ_SHIFT	(MA_PREEMPT_SHIFT + MA_PREEMPT_BITS)
#define MA_HARDIRQ_SHIFT	(MA_SOFTIRQ_SHIFT + MA_SOFTIRQ_BITS)

#define __MASK(x)	((1UL << (x))-1)

#define MA_PREEMPT_MASK	(__MASK(MA_PREEMPT_BITS) << MA_PREEMPT_SHIFT)
#define MA_HARDIRQ_MASK	(__MASK(MA_HARDIRQ_BITS) << MA_HARDIRQ_SHIFT)
#define MA_SOFTIRQ_MASK	(__MASK(MA_SOFTIRQ_BITS) << MA_SOFTIRQ_SHIFT)

#define ma_hardirq_count()	(ma_preempt_count() & MA_HARDIRQ_MASK)
#define ma_softirq_count()	(ma_preempt_count() & MA_SOFTIRQ_MASK)
#define ma_irq_count()	(ma_preempt_count() & (MA_HARDIRQ_MASK | MA_SOFTIRQ_MASK))

#define MA_PREEMPT_OFFSET	(1UL << MA_PREEMPT_SHIFT)
#define MA_SOFTIRQ_OFFSET	(1UL << MA_SOFTIRQ_SHIFT)
#define MA_HARDIRQ_OFFSET	(1UL << MA_HARDIRQ_SHIFT)

#define MA_PREEMPT_BUGMASK	((MA_PREEMPT_OFFSET|MA_SOFTIRQ_OFFSET|MA_HARDIRQ_OFFSET)<<7)

/*
 * The hardirq mask has to be large enough to have
 * space for potentially all IRQ sources in the system
 * nesting on a single CPU:
 */
#ifdef MA_NR_IRQS
#if (1 << MA_HARDIRQ_BITS) < MA_NR_IRQS
# error HARDIRQ_BITS is too low!
#endif
#endif

/*
 * Are we doing bottom half or hardware interrupt processing?
 * Are we in a softirq context? Interrupt context?
 */
#define ma_in_irq()		(ma_hardirq_count())
#define ma_in_softirq()		(ma_softirq_count())
#define ma_in_interrupt()	(ma_irq_count())


#define ma_hardirq_trylock()	(!ma_in_interrupt())
#define ma_hardirq_endlock()	do { } while (0)

#define ma_irq_enter()		(ma_preempt_count() += MA_HARDIRQ_OFFSET)
#define ma_nmi_enter()		(ma_irq_enter())
#define ma_nmi_exit()		(ma_preempt_count() -= MA_HARDIRQ_OFFSET)

#define ma_in_atomic()		(ma_preempt_count() & ~MA_PREEMPT_ACTIVE)
#define ma_last_preempt()	((ma_preempt_count() & ~MA_PREEMPT_ACTIVE) == MA_PREEMPT_OFFSET)
#define MA_IRQ_EXIT_OFFSET	(MA_HARDIRQ_OFFSET-1)
#define ma_irq_exit()							\
do {									\
		ma_preempt_count() -= MA_IRQ_EXIT_OFFSET;			\
		MA_BUG_ON(ma_preempt_count() & MA_PREEMPT_BUGMASK); \
		if (!ma_in_interrupt() && (ma_local_softirq_pending() || (ma_vpnum(MA_LWP_SELF) != -1 && ma_softirq_pending_vp(ma_vpnum(MA_LWP_SELF))))) \
			ma_do_softirq();					\
		ma_preempt_enable_no_resched();				\
} while (0)

#ifdef MA__LWPS
  extern void ma_synchronize_irq (unsigned int irq);
#else
# define ma_synchronize_irq(irq)	ma_barrier()
#endif /* MA__LWPS */


#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_GENERIC_LINUX_HARDIRQ_H__ **/
