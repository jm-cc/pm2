/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.

______________________________________________________________________________
$Log: act_spinlock.h,v $
Revision 1.2  2000/03/06 14:56:00  rnamyst
Modified to include "marcel_flags.h".

______________________________________________________________________________
*/

#ifndef __ACT_SPINLOCK_H
#define __ACT_SPINLOCK_H

#include "sys/marcel_flags.h"

#ifndef __SMP__

#define DEBUG_SPINLOCKS	0	/* 0 == no debugging, 1 == maintain lock state, 2 == full debug */

#if (DEBUG_SPINLOCKS < 1)

/*
 * Your basic spinlocks, allowing only a single CPU anywhere
 *
 * Gcc-2.7.x has a nasty bug with empty initializers.
 */
#if (__GNUC__ > 2) || (__GNUC__ == 2 && __GNUC_MINOR__ >= 8)
  typedef struct { } act_spinlock_t;
  #define ACT_SPIN_LOCK_UNLOCKED (act_spinlock_t) { }
#else
  typedef struct { int gcc_is_buggy; } act_spinlock_t;
  #define ACT_SPIN_LOCK_UNLOCKED (act_spinlock_t) { 0 }
#endif

#define act_spin_lock_init(lock)	do { } while(0)
#define act_spin_lock(lock)		(void)(lock) /*Not "unused variable".*/
#define act_spin_trylock(lock)	(1)
#define act_spin_unlock_wait(lock)	do { } while(0)
#define act_spin_unlock(lock)		do { } while(0)

#elif (DEBUG_SPINLOCKS < 2)

typedef struct {
	volatile unsigned int lock;
} act_spinlock_t;
#define ACT_SPIN_LOCK_UNLOCKED (act_spinlock_t) { 0 }

#define act_spin_lock_init(x)	do { (x)->lock = 0; } while (0)
#define act_spin_trylock(lock)	(!test_and_set_bit(0,(lock)))

#define act_spin_lock(x)	do { (x)->lock = 1; } while (0)
#define act_spin_unlock_wait(x)	do { } while (0)
#define act_spin_unlock(x)	do { (x)->lock = 0; } while (0)

#else /* (DEBUG_SPINLOCKS >= 2) */

#error "Not yet done"

typedef struct {
	volatile unsigned int lock;
	volatile unsigned int babble;
	const char *module;
} act_spinlock_t;
#define ACT_SPIN_LOCK_UNLOCKED (act_spinlock_t) { 0, 25, __BASE_FILE__ }

#include <linux/kernel.h>

#define act_spin_lock_init(x)	do { (x)->lock = 0; } while (0)
#define act_spin_trylock(lock)	(!test_and_set_bit(0,(lock)))

#define spin_lock(x)		do {unsigned long __spinflags; save_flags(__spinflags); cli(); if ((x)->lock&&(x)->babble) {printk("%s:%d: spin_lock(%s:%p) already locked\n", __BASE_FILE__,__LINE__, (x)->module, (x));(x)->babble--;} (x)->lock = 1; restore_flags(__spinflags);} while (0)
#define spin_unlock_wait(x)	do {unsigned long __spinflags; save_flags(__spinflags); cli(); if ((x)->lock&&(x)->babble) {printk("%s:%d: spin_unlock_wait(%s:%p) deadlock\n", __BASE_FILE__,__LINE__, (x)->module, (x));(x)->babble--;} restore_flags(__spinflags);} while (0)
#define spin_unlock(x)		do {unsigned long __spinflags; save_flags(__spinflags); cli(); if (!(x)->lock&&(x)->babble) {printk("%s:%d: spin_unlock(%s:%p) not locked\n", __BASE_FILE__,__LINE__, (x)->module, (x));(x)->babble--;} (x)->lock = 0; restore_flags(__spinflags);} while (0)
#define spin_lock_irq(x)	do {cli(); if ((x)->lock&&(x)->babble) {printk("%s:%d: spin_lock_irq(%s:%p) already locked\n", __BASE_FILE__,__LINE__, (x)->module, (x));(x)->babble--;} (x)->lock = 1;} while (0)
#define spin_unlock_irq(x)	do {cli(); if (!(x)->lock&&(x)->babble) {printk("%s:%d: spin_lock(%s:%p) not locked\n", __BASE_FILE__,__LINE__, (x)->module, (x));(x)->babble--;} (x)->lock = 0; sti();} while (0)

#define spin_lock_irqsave(x,flags)      do {save_flags(flags); cli(); if ((x)->lock&&(x)->babble) {printk("%s:%d: spin_lock_irqsave(%s:%p) already locked\n", __BASE_FILE__,__LINE__, (x)->module, (x));(x)->babble--;} (x)->lock = 1;} while (0)
#define spin_unlock_irqrestore(x,flags) do {cli(); if (!(x)->lock&&(x)->babble) {printk("%s:%d: spin_unlock_irqrestore(%s:%p) not locked\n", __BASE_FILE__,__LINE__, (x)->module, (x));(x)->babble--;} (x)->lock = 0; restore_flags(flags);} while (0)

#endif	/* DEBUG_SPINLOCKS */


#else	/* __SMP__ */

/*
 * Your basic spinlocks, allowing only a single CPU anywhere
 */

typedef struct {
	volatile unsigned int lock;
} act_spinlock_t;

#define ACT_SPIN_LOCK_UNLOCKED (act_spinlock_t) { 0 }

#define act_spin_lock_init(x)	do { (x)->lock = 0; } while(0)
/*
 * Simple spin lock operations.  There are two variants, one clears IRQ's
 * on the local processor, one does not.
 *
 * We make no fairness assumptions. They have a cost.
 */

#define act_spin_unlock_wait(x)	do { barrier(); } while(((volatile spinlock_t *)(x))->lock)

typedef struct { unsigned long a[100]; } __dummy_lock_t;
#define __dummy_lock(lock) (*(__dummy_lock_t *)(lock))

#define act_spin_lock_string \
	"\n1:\t" \
	"lock ; btsl $0,%0\n\t" \
	"jc 2f\n" \
	".section .text.lock,\"ax\"\n" \
	"2:\t" \
	"testb $1,%0\n\t" \
	"jne 2b\n\t" \
	"jmp 1b\n" \
	".previous"

#define act_spin_unlock_string \
	"lock ; btrl $0,%0"

#define act_spin_lock(lock) \
__asm__ __volatile__( \
	act_spin_lock_string \
	:"=m" (__dummy_lock(lock)))

#define act_spin_unlock(lock) \
__asm__ __volatile__( \
	act_spin_unlock_string \
	:"=m" (__dummy_lock(lock)))

#define act_spin_trylock(lock) (!test_and_set_bit(0,(lock)))

#endif /* __SMP__ */
#endif /* __ASM_SPINLOCK_H */
