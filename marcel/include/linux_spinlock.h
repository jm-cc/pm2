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


#ifndef __LINUX_SPINLOCK_H__
#define __LINUX_SPINLOCK_H__


/* Taking a spin lock usually also disable preemption (since else other
 * spinners would wait for our being rescheduled): ma_*_lock/unlock
 *
 * If a spin lock is to be taken from a bottom half or softirq as well, these
 * have to be disabled as well: ma_*_lock/unlock_softirq/bh
 *
 * When releasing a spin lock, we need to check whether we should have been
 * preempted in the meanwhile, and thus the unlock functions may call
 * schedule(). If we are to call ma_schedule() shortly (e.g. in a
 * INTERRUPTIBLE_SLEEP_ON_CONDITION_RELEASING loop), then we can use the
 * *_no_resched version to avoid calling ma_schedule() twice.
 */


#include "tbx_compiler.h"
#include "tbx_macros.h"
#include "sys/marcel_flags.h"
#include "asm/linux_atomic.h"
#include "asm/linux_spinlock.h"
#include "asm/marcel_compareexchange.h"
#include "marcel_utils.h"
#include "linux_preempt.h"
#ifdef MA__LWPS
#include "asm/linux_rwlock.h"
#endif


/** Public data types **/
/*
 * If MA__LWPS is set, pull in the _raw_* definitions
 */
#if !defined(MA__LWPS) && defined(MA_HAVE_COMPAREEXCHANGE)
#ifdef MARCEL_DEBUG_SPINLOCK
typedef struct {
	unsigned long magic;
	volatile unsigned long lock;
	volatile unsigned int babble;
	const char *module;
	marcel_t owner;
	char *ofile;
	int oline;
	void *bt[TBX_BACKTRACE_DEPTH];
	size_t btsize;
} ma_spinlock_t;
#define MA_SPINLOCK_MAGIC  0x1D244B3C
#define MA_SPIN_LOCK_UNLOCKED { .magic=MA_SPINLOCK_MAGIC, .lock=0, .babble=10, .module=__FILE__ , .owner=NULL , .ofile=NULL, .oline=0}
#else				/* MARCEL_DEBUG_SPINLOCK */
/*
 * gcc versions before ~2.95 have a nasty bug with empty initializers.
 */
#if (__GNUC__ > 2)
typedef struct {
} ma_spinlock_t;
#define MA_SPIN_LOCK_UNLOCKED { }
#else
typedef struct {
	int gcc_is_buggy;
} ma_spinlock_t;
#define MA_SPIN_LOCK_UNLOCKED { 0 }
#endif
#endif				/* MARCEL_DEBUG_SPINLOCK */
#endif				/* MA__LWPS */


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal macros **/
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
#define ma_spin_is_locked_nofail(x) ma_spin_is_locked(x)
/* XXX: we do not check the owner */
#define ma_spin_check_locked(x) ma_spin_is_locked(x)
#endif

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
		(x)->ofile = NULL; \
                (x)->oline = 0; \
		(x)->btsize = 0; \
        } while (0)
#define MA_CHECK_LOCK(x) \
        do { \
                if ((x)->magic != MA_SPINLOCK_MAGIC) { \
                        PM2_LOG("%s:%d: spin_is_locked on uninitialized spinlock %p.\n", \
                                        __FILE__, __LINE__, (x)); \
			SPIN_ABORT(); \
                } \
        } while(0)
#define _ma_raw_spin_lock(x)               \
        do { \
                MA_CHECK_LOCK(x); \
                if ((x)->lock&&(x)->babble) { \
                        (x)->babble--; \
                        PM2_LOG("%s:%d: spin_lock(%s:%p) already locked by %p:%s:%d\n", \
                                        __FILE__,__LINE__, (x)->module, \
					(x), (x)->owner, (x)->ofile, (x)->oline); \
			if ((x)->btsize) \
				__TBX_PRINT_SOME_TRACE((x)->bt, (x)->btsize); \
			SPIN_ABORT(); \
                } \
                (x)->lock = 1; \
		(x)->owner = MARCEL_SELF; \
		(x)->ofile = __FILE__; \
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
                        PM2_LOG("%s:%d: spin_is_locked(%s:%p) already locked by %p:%s:%d\n", \
                                        __FILE__,__LINE__, (x)->module, \
                                        (x), (x)->owner, (x)->ofile, (x)->oline); \
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
/* TODO: also check that we are the owner */
#define ma_spin_check_locked(x) \
        ({ \
                MA_CHECK_LOCK(x); \
                (x)->lock && (x)->owner == MARCEL_SELF; \
	 })
/* without debugging, spin_trylock on UP always says
 * TRUE. --> printk if already locked. */
#define _ma_raw_spin_trylock(x) \
        ({ \
                MA_CHECK_LOCK(x); \
                if ((x)->lock&&(x)->babble) { \
                        (x)->babble--; \
                        PM2_LOG("%s:%d: spin_trylock(%s:%p) already locked by %p:%s:%d\n", \
                                        __FILE__,__LINE__, (x)->module, \
                                        (x), (x)->owner, (x)->ofile, (x)->oline); \
			if ((x)->btsize) \
				__TBX_PRINT_SOME_TRACE((x)->bt, (x)->btsize); \
			SPIN_ABORT(); \
                } \
                (x)->lock = 1; \
		(x)->owner = MARCEL_SELF; \
                (x)->ofile = __FILE__; \
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
                        PM2_LOG("%s:%d: spin_unlock_wait(%s:%p) owned by %p:%s:%d\n", \
                                        __FILE__,__LINE__, (x)->module, (x), \
                                        (x)->owner, (x)->ofile, (x)->oline); \
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
                        PM2_LOG("%s:%d: spin_unlock(%s:%p) not locked\n", \
                                        __FILE__,__LINE__, (x)->module, (x));\
			SPIN_ABORT(); \
                } \
                (x)->lock = 0; \
        } while (0)
#else				/* MARCEL_DEBUG_SPINLOCK */
/*
 * If MA__LWPS is unset, declare the _raw_* definitions as nops
 */
#define ma_spin_lock_init(lock)	do { (void)(lock); } while(0)
#define _ma_raw_spin_lock(lock)	do { (void)(lock); } while(0)
#define ma_spin_is_locked_nofail(lock) ((void)(lock), 0)
#define ma_spin_is_locked(lock)	ma_spin_is_locked_nofail(lock)
#define ma_spin_check_locked(lock)	((void)(lock), 1)
#define _ma_raw_spin_trylock(lock)	((void)(lock), 1)
#define ma_spin_unlock_wait(lock)	do { (void)(lock); } while(0)
#define _ma_raw_spin_unlock(lock)	do { (void)(lock); } while(0)
#endif				/* MARCEL_DEBUG_SPINLOCK */
#endif				/* MA__LWPS */

#ifndef MA__LWPS
/* RW spinlocks: No debug version */
#if (__GNUC__ > 2)
    typedef struct {
} ma_rwlock_t;
#define MA_RW_LOCK_UNLOCKED { }
#else
    typedef struct {
	int gcc_is_buggy;
} ma_rwlock_t;
#define MA_RW_LOCK_UNLOCKED { 0 }
#endif

#define ma_rwlock_init(lock)	do { (void)(lock); } while(0)
#define _ma_raw_read_lock(lock)	do { (void)(lock); } while(0)
#define _ma_raw_read_unlock(lock)	do { (void)(lock); } while(0)
#define _ma_raw_write_lock(lock)	do { (void)(lock); } while(0)
#define _ma_raw_write_unlock(lock)	do { (void)(lock); } while(0)
#define _ma_raw_write_trylock(lock) ({ (void)(lock); (1); })
#endif				/* !MA__LWPS */

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

#define ma_spin_lock_softirq(lock) ma_spin_lock_bh(lock)

#define ma_spin_lock_bh(lock) \
do { \
	  ma_local_bh_disable(); \
	  ma_preempt_disable(); \
	  _ma_raw_spin_lock(lock); \
} while (0)

#define ma_read_lock_softirq(lock) ma_read_lock_bh(lock)

#define ma_read_lock_bh(lock) \
do { \
	ma_local_bh_disable(); \
	ma_preempt_disable(); \
	_ma_raw_read_lock(lock); \
} while (0)

#define ma_write_lock_softirq(lock) ma_write_lock_bh(lock)

#define ma_write_lock_bh(lock) \
do { \
	ma_local_bh_disable(); \
	ma_preempt_disable(); \
	_ma_raw_write_lock(lock); \
} while (0)

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

#define ma_read_unlock_softirq(lock) ma_read_unlock_bh(lock)

#define ma_read_unlock_bh(lock) \
do { \
	_ma_raw_read_unlock(lock); \
	ma_preempt_enable_no_resched(); \
	ma_local_bh_enable(); \
} while (0)

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


/** Internal functions **/
#if defined(MA__LWPS) || !defined(MA_HAVE_COMPAREEXCHANGE)
void __ma_preempt_spin_lock(ma_spinlock_t * lock);
#endif

#if defined(MA__LWPS)
void __ma_preempt_write_lock(ma_rwlock_t * lock);
#endif

/* "lock on reference count zero" */
extern int ma_atomic_dec_and_lock(ma_atomic_t * atomic, ma_spinlock_t * lock);


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __LINUX_SPINLOCK_H__ **/
