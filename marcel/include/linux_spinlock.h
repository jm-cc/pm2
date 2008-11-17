
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
#include "tbx_macros.h"
#depend "asm/marcel_compareexchange.h[macros]"
/*
 * similar to:
 * include/linux/spinlock.h - generic locking declarations
 */

#section marcel_macros
/*
 * Must define these before including other files, inline functions need them
 */
#define MA_LOCK_SECTION_NAME			\
	".text.lock." __ma_stringify(MODULE)

#define MA_LOCK_SECTION_START(extra)		\
	".subsection 1\n\t"			\
	extra					\
	".ifndef " MA_LOCK_SECTION_NAME "\n\t"	\
	MA_LOCK_SECTION_NAME ":\n\t"		\
	".endif\n\t"

#define MA_LOCK_SECTION_END			\
	".previous\n\t"

#if defined(MA__LWPS) || !defined(MA_HAVE_COMPAREEXCHANGE)
#depend "asm/linux_spinlock.h[marcel_macros]"
#define ma_spin_is_locked_nofail(x) ma_spin_is_locked(x)
#endif

#section types
/*
 * If MA__LWPS is set, pull in the _raw_* definitions
 */
#if defined(MA__LWPS) || !defined(MA_HAVE_COMPAREEXCHANGE)
#depend "asm/linux_spinlock.h[types]"
#else
#ifdef MARCEL_DEBUG_SPINLOCK
typedef struct {
        unsigned long magic;
        volatile unsigned long lock;
        volatile unsigned int babble;
        const char *module;
        char *owner;
        int oline;
	void *bt[TBX_BACKTRACE_DEPTH];
	size_t btsize;
} ma_spinlock_t;
#define MA_SPINLOCK_MAGIC  0x1D244B3C
#define MA_SPIN_LOCK_UNLOCKED { .magic=MA_SPINLOCK_MAGIC, .lock=0, .babble=10, .module=__FILE__ , .owner=NULL , .oline=0}
#else /* MARCEL_DEBUG_SPINLOCK */
/*
 * gcc versions before ~2.95 have a nasty bug with empty initializers.
 */
#if (__GNUC__ > 2)
  typedef struct { } ma_spinlock_t;
  #define MA_SPIN_LOCK_UNLOCKED { }
#else
  typedef struct { int gcc_is_buggy; } ma_spinlock_t;
  #define MA_SPIN_LOCK_UNLOCKED { 0 }
#endif
#endif /* MARCEL_DEBUG_SPINLOCK */
#endif /* MA__LWPS */
#section marcel_macros
#if !defined(MA__LWPS) && defined(MA_HAVE_COMPAREEXCHANGE)
#ifdef MARCEL_DEBUG_SPINLOCK

/* #define SPIN_ABORT() */
#define SPIN_ABORT() MARCEL_EXCEPTION_RAISE(MARCEL_PROGRAM_ERROR) 
	
#define ma_spin_lock_init(x) \
        do { \
                (x)->magic = MA_SPINLOCK_MAGIC; \
                (x)->lock = 0; \
                (x)->babble = 5; \
                (x)->module = __FILE__; \
                (x)->owner = NULL; \
                (x)->oline = 0; \
		(x)->btsize = 0; \
        } while (0)

#define MA_CHECK_LOCK(x) \
        do { \
                if ((x)->magic != MA_SPINLOCK_MAGIC) { \
                        pm2debug("%s:%d: spin_is_locked on uninitialized spinlock %p.\n", \
                                        __FILE__, __LINE__, (x)); \
			SPIN_ABORT(); \
                } \
        } while(0)

#define _ma_raw_spin_lock(x)               \
        do { \
                MA_CHECK_LOCK(x); \
                if ((x)->lock&&(x)->babble) { \
                        (x)->babble--; \
                        pm2debug("%s:%d: spin_lock(%s:%p) already locked by %s:%d\n", \
                                        __FILE__,__LINE__, (x)->module, \
                                        (x), (x)->owner, (x)->oline); \
			if ((x)->btsize) \
				__TBX_PRINT_SOME_TRACE((x)->bt, (x)->btsize); \
			SPIN_ABORT(); \
                } \
                (x)->lock = 1; \
                (x)->owner = __FILE__; \
                (x)->oline = __LINE__; \
		if (!SELF_GETMEM(spinlock_backtrace) && ma_init_done[MA_INIT_SCHEDULER]) { \
			SELF_GETMEM(spinlock_backtrace) = 1; \
			(x)->btsize = __TBX_RECORD_SOME_TRACE((x)->bt, TBX_BACKTRACE_DEPTH); \
		} \
          } while (0)

/* without debugging, spin_is_locked on UP always says
 * FALSE. --> printk if already locked. */
#define ma_spin_is_locked(x) \
        ({ \
                MA_CHECK_LOCK(x); \
                if ((x)->lock&&(x)->babble) { \
                        (x)->babble--; \
                        pm2debug("%s:%d: spin_is_locked(%s:%p) already locked by %s:%d\n", \
                                        __FILE__,__LINE__, (x)->module, \
                                        (x), (x)->owner, (x)->oline); \
			if ((x)->btsize) \
				__TBX_PRINT_SOME_TRACE((x)->bt, (x)->btsize); \
			SPIN_ABORT(); \
                } \
                0; \
        })

#define ma_spin_is_locked_nofail(x) \
        ({ \
                MA_CHECK_LOCK(x); \
                (x)->lock; \
	 })

/* without debugging, spin_trylock on UP always says
 * TRUE. --> printk if already locked. */
#define _ma_raw_spin_trylock(x) \
        ({ \
                MA_CHECK_LOCK(x); \
                if ((x)->lock&&(x)->babble) { \
                        (x)->babble--; \
                        pm2debug("%s:%d: spin_trylock(%s:%p) already locked by %s:%d\n", \
                                        __FILE__,__LINE__, (x)->module, \
                                        (x), (x)->owner, (x)->oline); \
			if ((x)->btsize) \
				__TBX_PRINT_SOME_TRACE((x)->bt, (x)->btsize); \
			SPIN_ABORT(); \
                } \
                (x)->lock = 1; \
                (x)->owner = __FILE__; \
                (x)->oline = __LINE__; \
		if (!SELF_GETMEM(spinlock_backtrace) && ma_init_done[MA_INIT_SCHEDULER]) { \
			SELF_GETMEM(spinlock_backtrace) = 1; \
			(x)->btsize = __TBX_RECORD_SOME_TRACE((x)->bt, TBX_BACKTRACE_DEPTH); \
		} \
                1; \
        })

#define ma_spin_unlock_wait(x)     \
        do { \
                MA_CHECK_LOCK(x); \
                if ((x)->lock&&(x)->babble) { \
                        (x)->babble--; \
                        pm2debug("%s:%d: spin_unlock_wait(%s:%p) owned by %s:%d\n", \
                                        __FILE__,__LINE__, (x)->module, (x), \
                                        (x)->owner, (x)->oline); \
			if ((x)->btsize) \
				__TBX_PRINT_SOME_TRACE((x)->bt, (x)->btsize); \
			SPIN_ABORT(); \
                }\
		if ((x)->btsize) { \
			(x)->btsize = 0; \
			SELF_GETMEM(spinlock_backtrace) = 0; \
		} \
        } while (0)

#define _ma_raw_spin_unlock(x) \
        do { \
                MA_CHECK_LOCK(x); \
                if (!(x)->lock&&(x)->babble) { \
                        (x)->babble--; \
                        pm2debug("%s:%d: spin_unlock(%s:%p) not locked\n", \
                                        __FILE__,__LINE__, (x)->module, (x));\
			SPIN_ABORT(); \
                } \
                (x)->lock = 0; \
        } while (0)
#else /* MARCEL_DEBUG_SPINLOCK */
/*
 * If MA__LWPS is unset, declare the _raw_* definitions as nops
 */
#define ma_spin_lock_init(lock)	do { (void)(lock); } while(0)
#define _ma_raw_spin_lock(lock)	do { (void)(lock); } while(0)
#define ma_spin_is_locked_nofail(lock) ((void)(lock), 0)
#define ma_spin_is_locked(lock)	ma_spin_is_locked_nofail(lock)
#define _ma_raw_spin_trylock(lock)	((void)(lock), 1)
#define ma_spin_unlock_wait(lock)	do { (void)(lock); } while(0)
#define _ma_raw_spin_unlock(lock)	do { (void)(lock); } while(0)
#endif /* MARCEL_DEBUG_SPINLOCK */
#endif /* MA__LWPS */

#ifndef MA__LWPS
/* RW spinlocks: No debug version */

#if (__GNUC__ > 2)
  typedef struct { } ma_rwlock_t;
  #define MA_RW_LOCK_UNLOCKED (ma_rwlock_t) { }
#else
  typedef struct { int gcc_is_buggy; } ma_rwlock_t;
  #define MA_RW_LOCK_UNLOCKED (ma_rwlock_t) { 0 }
#endif

#define ma_rwlock_init(lock)	do { (void)(lock); } while(0)
#define _ma_raw_read_lock(lock)	do { (void)(lock); } while(0)
#define _ma_raw_read_unlock(lock)	do { (void)(lock); } while(0)
#define _ma_raw_write_lock(lock)	do { (void)(lock); } while(0)
#define _ma_raw_write_unlock(lock)	do { (void)(lock); } while(0)
#define _ma_raw_write_trylock(lock) ({ (void)(lock); (1); })
#endif /* !MA__LWPS */

#section marcel_macros
#depend "linux_preempt.h[marcel_macros]"
/*
 * Define the various spin_lock and rw_lock methods.  Note we define these
 * regardless of whether MA__LWPS is set. The various
 * methods are defined as nops in the case they are not required.
 */
#define ma_spin_trylock(lock)	({ma_preempt_disable(); _ma_raw_spin_trylock(lock) ? \
				1 : ({ma_preempt_enable(); 0;});})

#define ma_write_trylock(lock)	({ma_preempt_disable();_ma_raw_write_trylock(lock) ? \
				1 : ({ma_preempt_enable(); 0;});})

/* Where's read_trylock? */
#section functions
#if defined(MA__LWPS) || !defined(MA_HAVE_COMPAREEXCHANGE)
#depend "asm/linux_spinlock.h[types]"
TBX_EXTERN void __ma_preempt_spin_lock(ma_spinlock_t *lock);
#endif

#section marcel_functions
#if defined(MA__LWPS)
#depend "asm/linux_rwlock.h[marcel_types]"
TBX_EXTERN void __ma_preempt_write_lock(ma_rwlock_t *lock);
#endif

#section marcel_macros
#depend "linux_interrupt.h[marcel_macros]"
#if defined(MA__LWPS) || !defined(MA_HAVE_COMPAREEXCHANGE)
#define ma_spin_lock(lock) \
do { \
	ma_preempt_disable(); \
	if (tbx_unlikely(!_ma_raw_spin_trylock(lock))) \
		__ma_preempt_spin_lock(lock); \
} while (0)
#else
#define ma_spin_lock(lock)	\
do { \
	ma_preempt_disable(); \
	_ma_raw_spin_lock(lock); \
} while(0)
#endif

#ifdef MA__LWPS
#define ma_write_lock(lock) \
do { \
	ma_preempt_disable(); \
	if (tbx_unlikely(!_ma_raw_write_trylock(lock))) \
		__ma_preempt_write_lock(lock); \
} while (0)
#else
#define ma_write_lock(lock) \
do { \
	ma_preempt_disable(); \
	_ma_raw_write_lock(lock); \
} while(0)
#endif

#define ma_read_lock(lock)	\
do { \
	ma_preempt_disable(); \
	_ma_raw_read_lock(lock); \
} while(0)

#define ma_spin_unlock(lock) \
do { \
	_ma_raw_spin_unlock(lock); \
	ma_preempt_enable(); \
} while (0)

#define ma_write_unlock(lock) \
do { \
	_ma_raw_write_unlock(lock); \
	ma_preempt_enable(); \
} while(0)

#define ma_read_unlock(lock) \
do { \
	_ma_raw_read_unlock(lock); \
	ma_preempt_enable(); \
} while(0)

/*
#define ma_spin_lock_irqsave(lock, flags) \
do { \
	ma_local_irq_save(flags); \
	ma_preempt_disable(); \
	_ma_raw_spin_lock(lock); \
} while (0)

#define spin_lock_irq(lock) \
do { \
	ma_local_irq_disable(); \
	ma_preempt_disable(); \
	_ma_raw_spin_lock(lock); \
} while (0)
*/
#define ma_spin_lock_softirq(lock) ma_spin_lock_bh(lock)

#define ma_spin_lock_bh(lock) \
do { \
	  ma_local_bh_disable(); \
	  ma_preempt_disable(); \
	  _ma_raw_spin_lock(lock); \
} while (0)

/*
#define ma_read_lock_irqsave(lock, flags) \
do { \
	ma_local_irq_save(flags); \
	ma_preempt_disable(); \
	_ma_raw_read_lock(lock); \
} while (0)

#define ma_read_lock_irq(lock) \
do { \
	ma_local_irq_disable(); \
	ma_preempt_disable(); \
	_ma_raw_read_lock(lock); \
} while (0)
*/
#define ma_read_lock_softirq(lock) ma_read_lock_bh(lock)

#define ma_read_lock_bh(lock) \
do { \
	ma_local_bh_disable(); \
	ma_preempt_disable(); \
	_ma_raw_read_lock(lock); \
} while (0)

/*
#define ma_write_lock_irqsave(lock, flags) \
do { \
	ma_local_irq_save(flags); \
	ma_preempt_disable(); \
	_ma_raw_write_lock(lock); \
} while (0)

#define ma_write_lock_irq(lock) \
do { \
	ma_local_irq_disable(); \
	ma_preempt_disable(); \
	_ma_raw_write_lock(lock); \
} while (0)
*/
#define ma_write_lock_softirq(lock) ma_write_lock_bh(lock)

#define ma_write_lock_bh(lock) \
do { \
	ma_local_bh_disable(); \
	ma_preempt_disable(); \
	_ma_raw_write_lock(lock); \
} while (0)

/*
#define ma_spin_unlock_irqrestore(lock, flags) \
do { \
	_ma_raw_spin_unlock(lock); \
	ma_local_irq_restore(flags); \
	ma_preempt_enable(); \
} while (0)

#define _ma_raw_spin_unlock_irqrestore(lock, flags) \
do { \
	_ma_raw_spin_unlock(lock); \
	ma_local_irq_restore(flags); \
} while (0)

#define ma_spin_unlock_irq(lock) \
do { \
	_ma_raw_spin_unlock(lock); \
	ma_local_irq_enable(); \
	ma_preempt_enable(); \
} while (0)
*/
#define ma_spin_unlock_softirq(lock) ma_spin_unlock_bh(lock)
#define ma_spin_unlock_softirq_no_resched(lock) ma_spin_unlock_bh_no_resched(lock)

#define ma_spin_unlock_bh_no_resched(lock) \
do { \
	_ma_raw_spin_unlock(lock); \
	ma_preempt_enable_no_resched(); \
	ma_local_bh_enable_no_resched(); \
} while (0)

#define ma_spin_unlock_bh(lock) \
do { \
	_ma_raw_spin_unlock(lock); \
	ma_preempt_enable_no_resched(); \
	ma_local_bh_enable(); \
} while (0)

/*
#define ma_read_unlock_irqrestore(lock, flags) \
do { \
	_ma_raw_read_unlock(lock); \
	ma_local_irq_restore(flags); \
	ma_preempt_enable(); \
} while (0)

#define ma_read_unlock_irq(lock) \
do { \
	_ma_raw_read_unlock(lock); \
	ma_local_irq_enable(); \
	ma_preempt_enable(); \
} while (0)
*/
#define ma_read_unlock_softirq(lock) ma_read_unlock_bh(lock)

#define ma_read_unlock_bh(lock) \
do { \
	_ma_raw_read_unlock(lock); \
	ma_preempt_enable_no_resched(); \
	ma_local_bh_enable(); \
} while (0)

/*
#define ma_write_unlock_irqrestore(lock, flags) \
do { \
	_ma_raw_write_unlock(lock); \
	ma_local_irq_restore(flags); \
	ma_preempt_enable(); \
} while (0)

#define ma_write_unlock_irq(lock) \
do { \
	_ma_raw_write_unlock(lock); \
	ma_local_irq_enable(); \
	ma_preempt_enable(); \
} while (0)
*/
#define ma_write_unlock_softirq(lock) ma_write_unlock_bh(lock)

#define ma_write_unlock_bh(lock) \
do { \
	_ma_raw_write_unlock(lock); \
	ma_preempt_enable_no_resched(); \
	ma_local_bh_enable(); \
} while (0)

#define ma_spin_trylock_softirq(lock) ma_spin_trylock_bh(lock)
#define ma_spin_trylock_bh(lock) \
	({ ma_local_bh_disable(); ma_preempt_disable(); \
	_ma_raw_spin_trylock(lock) ? 1 : \
	({ma_preempt_enable_no_resched(); ma_local_bh_enable(); 0;});})

#section marcel_functions
/* "lock on reference count zero" */
#include <asm/linux_atomic.h>
extern int ma_atomic_dec_and_lock(ma_atomic_t *atomic, ma_spinlock_t *lock);

#section marcel_inline
#if defined(MA__LWPS) || !defined(MA_HAVE_COMPAREEXCHANGE)
#depend "asm/linux_spinlock.h[marcel_inline]"
#endif
#depend "marcel_topology.h[marcel_inline]"
/*
 *  bit-based spin_lock()
 *
 * Don't use this unless you really need to: spin_lock() and spin_unlock()
 * are significantly faster.
 */
static __tbx_inline__ void ma_bit_spin_lock(int bitnum, unsigned long *addr)
{
	/*
	 * Assuming the lock is uncontended, this never enters
	 * the body of the outer loop. If it is contended, then
	 * within the inner loop a non-atomic test is used to
	 * busywait with less bus contention for a good time to
	 * attempt to acquire the lock bit.
	 */
	ma_preempt_disable();
#if defined(MA__LWPS) || defined(MA_DEBUG_SPINLOCK)
	while (ma_test_and_set_bit(bitnum, addr)) {
		while (ma_test_bit(bitnum, addr))
			cpu_relax();
	}
#endif
}

/*
 * Return true if it was acquired
 */
static __tbx_inline__ int ma_bit_spin_trylock(int bitnum, unsigned long *addr)
{
#if defined(MA__LWPS) || defined(MA_DEBUG_SPINLOCK)
	int ret;

	ma_preempt_disable();
	ret = !ma_test_and_set_bit(bitnum, addr);
	if (!ret)
		ma_preempt_enable();
	return ret;
#else
	ma_preempt_disable();
	return 1;
#endif
}

/*
 *  bit-based spin_unlock()
 */
static __tbx_inline__ void ma_bit_spin_unlock(int bitnum, unsigned long *addr)
{
#if defined(MA__LWPS) || defined(MA_DEBUG_SPINLOCK)
	MA_BUG_ON(!ma_test_bit(bitnum, addr));
	ma_smp_mb__before_clear_bit();
	ma_clear_bit(bitnum, addr);
#endif
	ma_preempt_enable();
}

/*
 * Return true if the lock is held.
 */
static __tbx_inline__ int ma_bit_spin_is_locked(int bitnum, unsigned long *addr)
{
#if defined(MA__LWPS) || defined(MA_DEBUG_SPINLOCK)
	return ma_test_bit(bitnum, addr);
#else
	return ma_preempt_count();
#endif
}
#section types
#section marcel_structures
