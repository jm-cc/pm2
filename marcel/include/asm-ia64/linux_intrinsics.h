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


#ifndef __ASM_IA64_LINUX_INTRINSICS_H__
#define __ASM_IA64_LINUX_INTRINSICS_H__


/*
 * Compiler-dependent intrinsics.
 *
 * Copyright (C) 2002-2003 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 */


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal macros **/
/* include compiler specific intrinsics */
#include "asm/linux_ia64regs.h"
#ifdef __INTEL_COMPILER
#include "asm/linux_intel_intrin.h"
#else
#include "asm/linux_gcc_intrin.h"
#endif

#define MA_IA64_FETCHADD(tmp,v,n,sz,sem)						\
({										\
	switch (sz) {								\
	      case 4:								\
	        tmp = ma_ia64_fetchadd4_##sem((volatile unsigned int *) v, n);		\
		break;								\
										\
	      case 8:								\
	        tmp = ma_ia64_fetchadd8_##sem((volatile unsigned long *) v, n);		\
		break;								\
										\
	      default:								\
		__ma_bad_size_for_ia64_fetch_and_add();				\
	}									\
})
#define ma_ia64_fetchadd(i,v,sem)								\
({											\
	uint64_t _tmp;									\
	volatile __typeof__(*(v)) *_v = (v);						\
	/* Can't use a switch () here: gcc isn't always smart enough for that... */	\
	if ((i) == -16)									\
		MA_IA64_FETCHADD(_tmp, _v, -16, sizeof(*(v)), sem);			\
	else if ((i) == -8)								\
		MA_IA64_FETCHADD(_tmp, _v, -8, sizeof(*(v)), sem);				\
	else if ((i) == -4)								\
		MA_IA64_FETCHADD(_tmp, _v, -4, sizeof(*(v)), sem);				\
	else if ((i) == -1)								\
		MA_IA64_FETCHADD(_tmp, _v, -1, sizeof(*(v)), sem);				\
	else if ((i) == 1)								\
		MA_IA64_FETCHADD(_tmp, _v, 1, sizeof(*(v)), sem);				\
	else if ((i) == 4)								\
		MA_IA64_FETCHADD(_tmp, _v, 4, sizeof(*(v)), sem);				\
	else if ((i) == 8)								\
		MA_IA64_FETCHADD(_tmp, _v, 8, sizeof(*(v)), sem);				\
	else if ((i) == 16)								\
		MA_IA64_FETCHADD(_tmp, _v, 16, sizeof(*(v)), sem);				\
	else										\
		_tmp = __ma_bad_increment_for_ia64_fetch_and_add();			\
	(__typeof__(*(v))) (_tmp);	/* return old value */				\
})
#define ma_ia64_fetch_and_add(i,v)	(ma_ia64_fetchadd(i, v, rel) + (i))	/* return new value */
#define __ma_xchg(x,ptr,size)						\
({									\
	unsigned long __xchg_result;					\
									\
	switch (size) {							\
	      case 1:							\
		      __xchg_result = ma_ia64_xchg1((volatile uint8_t *)ptr, x); \
		      break;						\
									\
	      case 2:							\
		      __xchg_result = ma_ia64_xchg2((volatile uint16_t *)ptr, x); \
		      break;						\
									\
	      case 4:							\
		      __xchg_result = ma_ia64_xchg4((volatile uint32_t *)ptr, x); \
		      break;						\
									\
	      case 8:							\
		      __xchg_result = ma_ia64_xchg8((volatile uint64_t *)ptr, x); \
		      break;						\
	      default:							\
		      ma_ia64_xchg_called_with_bad_pointer();		\
	}								\
	__xchg_result;							\
})
#define ma_xchg(ptr,x)							     \
  ((__typeof__(*(ptr))) __ma_xchg ((unsigned long) (x), (ptr), sizeof(*(ptr))))
/*
 * Atomic compare and exchange.  Compare OLD with MEM, if identical,
 * store NEW in MEM.  Return the initial value in MEM.  Success is
 * indicated by comparing RETURN with OLD.
 */
#define __MA_HAVE_ARCH_CMPXCHG 1
#define ma_ia64_cmpxchg(sem,ptr,old,repl,size)						\
({											\
	uint64_t _o_, _r_;									\
											\
	switch (size) {							\
	      case 1: _o_ = (uint8_t) (long) (old); break;		\
	      case 2: _o_ = (uint16_t) (long) (old); break;				\
	      case 4: _o_ = (uint32_t) (long) (old); break;				\
	      case 8: _o_ = (uint64_t) (long) (old); break;		\
	      default: break;								\
	}										\
	switch (size) {									\
	      case 1:									\
		      _r_ = ma_ia64_cmpxchg1_##sem((volatile uint8_t *)(void*) ptr, repl, _o_); \
		      break;						\
											\
	      case 2:									\
		      _r_ = ma_ia64_cmpxchg2_##sem((volatile uint16_t *)(void*)  ptr, repl, _o_);	\
		      break;						\
											\
	      case 4:									\
		      _r_ = ma_ia64_cmpxchg4_##sem((volatile uint32_t *)(void*)  ptr, repl, _o_); \
		      break;						\
											\
	      case 8:									\
		      _r_ = ma_ia64_cmpxchg8_##sem((volatile uint64_t *)(void*)  ptr, repl, _o_); \
		      break;						\
											\
	      default:									\
		      _r_ = ma_ia64_cmpxchg_called_with_bad_pointer();	\
		      break;						\
	}										\
	(__typeof__(old)) _r_;								\
})
#define ma_cmpxchg_acq(ptr,o,n)	ma_ia64_cmpxchg(acq, (ptr), (o), (n), sizeof(*(ptr)))
#define ma_cmpxchg_rel(ptr,o,n)	ma_ia64_cmpxchg(rel, (ptr), (o), (n), sizeof(*(ptr)))
/* for compatibility with other platforms: */
#define ma_cmpxchg(ptr,o,n)	ma_cmpxchg_acq(ptr,o,n)
#ifdef CONFIG_IA64_DEBUG_CMPXCHG
# define MA_CMPXCHG_BUGCHECK_DECL	int _cmpxchg_bugcheck_count = 128;
# define MA_CMPXCHG_BUGCHECK(v)							\
  do {										\
	if (_cmpxchg_bugcheck_count-- <= 0) {					\
		void *ip;							\
		extern int printk(const char *fmt, ...);			\
		ip = (void *) ia64_getreg(_IA64_REG_IP);			\
		printk("CMPXCHG_BUGCHECK: stuck at %p on word %p\n", ip, (v));	\
		break;								\
	}									\
  } while (0)
#else				/* !CONFIG_IA64_DEBUG_CMPXCHG */
# define MA_CMPXCHG_BUGCHECK_DECL
# define MA_CMPXCHG_BUGCHECK(v)
#endif				/* !CONFIG_IA64_DEBUG_CMPXCHG */


/** Internal functions **/
/*
 * Force an unresolved reference if someone tries to use
 * ia64_fetch_and_add() with a bad value.
 */
extern unsigned long __ma_bad_size_for_ia64_fetch_and_add(void);
extern unsigned long __ma_bad_increment_for_ia64_fetch_and_add(void);

/*
 * This function doesn't exist, so you'll get a linker error if
 * something tries to do an invalid xchg().
 */
extern void ma_ia64_xchg_called_with_bad_pointer(void);

/*
 * This function doesn't exist, so you'll get a linker error
 * if something tries to do an invalid cmpxchg().
 */
extern long ma_ia64_cmpxchg_called_with_bad_pointer(void);


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_IA64_LINUX_INTRINSICS_H__ **/
