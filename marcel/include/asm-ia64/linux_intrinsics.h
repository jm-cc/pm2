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
/*
 * Similar to:
 * include/asm-ia64/intrinsics.h
 *
 * Compiler-dependent intrinsics.
 *
 * Copyright (C) 2002-2003 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 */

/* include compiler specific intrinsics */
#depend "asm/linux_ia64regs.h[marcel_macros]"
#ifdef __INTEL_COMPILER
# depend "asm/linux_intel_intrin.h[marcel_functions]"
#else
# depend "asm/linux_gcc_intrin.h[marcel_macros]"
# depend "asm/linux_gcc_intrin.h[marcel_functions]"
#endif

#section marcel_functions
/*
 * Force an unresolved reference if someone tries to use
 * ia64_fetch_and_add() with a bad value.
 */
extern unsigned long __ma_bad_size_for_ia64_fetch_and_add (void);
extern unsigned long __ma_bad_increment_for_ia64_fetch_and_add (void);

#section marcel_macros
#define MA_IA64_FETCHADD(tmp,v,n,sz,sem)						\
({										\
	switch (sz) {								\
	      case 4:								\
	        tmp = ma_ia64_fetchadd4_##sem((unsigned int *) v, n);		\
		break;								\
										\
	      case 8:								\
	        tmp = ma_ia64_fetchadd8_##sem((unsigned long *) v, n);		\
		break;								\
										\
	      default:								\
		__ma_bad_size_for_ia64_fetch_and_add();				\
	}									\
})

#define ma_ia64_fetchadd(i,v,sem)								\
({											\
	__ma_u64 _tmp;									\
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

#define ma_ia64_fetch_and_add(i,v)	(ma_ia64_fetchadd(i, v, rel) + (i)) /* return new value */

#section marcel_functions
/*
 * This function doesn't exist, so you'll get a linker error if
 * something tries to do an invalid xchg().
 */
extern void ma_ia64_xchg_called_with_bad_pointer (void);

#section marcel_macros
#define __ma_xchg(x,ptr,size)						\
({									\
	unsigned long __xchg_result;					\
									\
	switch (size) {							\
	      case 1:							\
		__xchg_result = ma_ia64_xchg1((__ma_u8 *)ptr, x);	\
		break;							\
									\
	      case 2:							\
		__xchg_result = ma_ia64_xchg2((__ma_u16 *)ptr, x);	\
		break;							\
									\
	      case 4:							\
		__xchg_result = ma_ia64_xchg4((__ma_u32 *)ptr, x);	\
		break;							\
									\
	      case 8:							\
		__xchg_result = ma_ia64_xchg8((__ma_u64 *)ptr, x);	\
		break;							\
	      default:							\
		ma_ia64_xchg_called_with_bad_pointer();			\
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

#section marcel_functions
/*
 * This function doesn't exist, so you'll get a linker error
 * if something tries to do an invalid cmpxchg().
 */
extern long ma_ia64_cmpxchg_called_with_bad_pointer (void);

#section marcel_macros
#define ma_ia64_cmpxchg(sem,ptr,old,new,size)						\
({											\
	__ma_u64 _o_, _r_;									\
											\
	switch (size) {									\
	      case 1: _o_ = (__ma_u8 ) (long) (old); break;				\
	      case 2: _o_ = (__ma_u16) (long) (old); break;				\
	      case 4: _o_ = (__ma_u32) (long) (old); break;				\
	      case 8: _o_ = (__ma_u64) (long) (old); break;				\
	      default: break;								\
	}										\
	switch (size) {									\
	      case 1:									\
	      	_r_ = ma_ia64_cmpxchg1_##sem((__ma_u8 *) ptr, new, _o_);			\
		break;									\
											\
	      case 2:									\
	       _r_ = ma_ia64_cmpxchg2_##sem((__ma_u16 *) ptr, new, _o_);			\
		break;									\
											\
	      case 4:									\
	      	_r_ = ma_ia64_cmpxchg4_##sem((__ma_u32 *) ptr, new, _o_);			\
		break;									\
											\
	      case 8:									\
		_r_ = ma_ia64_cmpxchg8_##sem((__ma_u64 *) ptr, new, _o_);			\
		break;									\
											\
	      default:									\
		_r_ = ma_ia64_cmpxchg_called_with_bad_pointer();				\
		break;									\
	}										\
	(__typeof__(old)) _r_;								\
})

#define ma_cmpxchg_acq(ptr,o,n)	ma_ia64_cmpxchg(acq, (ptr), (o), (n), sizeof(*(ptr)))
#define ma_cmpxchg_rel(ptr,o,n)	ma_ia64_cmpxchg(rel, (ptr), (o), (n), sizeof(*(ptr)))

/* for compatibility with other platforms: */
#define ma_cmpxchg(ptr,o,n)	ma_cmpxchg_acq(ptr,o,n)

#if 0 && defined(CONFIG_IA64_DEBUG_CMPXCHG)
# define CMPXCHG_BUGCHECK_DECL	int _cmpxchg_bugcheck_count = 128;
# define CMPXCHG_BUGCHECK(v)							\
  do {										\
	if (_cmpxchg_bugcheck_count-- <= 0) {					\
		void *ip;							\
		extern int printk(const char *fmt, ...);			\
		ip = (void *) ia64_getreg(_IA64_REG_IP);			\
		printk("CMPXCHG_BUGCHECK: stuck at %p on word %p\n", ip, (v));	\
		break;								\
	}									\
  } while (0)
#else /* !CONFIG_IA64_DEBUG_CMPXCHG */
# define MA_CMPXCHG_BUGCHECK_DECL
# define MA_CMPXCHG_BUGCHECK(v)
#endif /* !CONFIG_IA64_DEBUG_CMPXCHG */

