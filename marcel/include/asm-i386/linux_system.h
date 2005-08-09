
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
 * include/asm-i386/atomic.h
 */

#section marcel_macros
#define ma_nop() __asm__ __volatile__ ("nop")

#define ma_xchg(ptr,v) ((__typeof__(*(ptr)))__ma_xchg((unsigned long)(v),(ptr),sizeof(*(ptr))))

#define ma_tas(ptr) (xchg((ptr),1))

struct __ma_xchg_dummy { unsigned long a[100]; };
#define __ma_xg(x) ((struct __ma_xchg_dummy *)(x))


/*
 * The semantics of XCHGCMP8B are a bit strange, this is why
 * there is a loop and the loading of %%eax and %%edx has to
 * be inside. This inlines well in most cases, the cached
 * cost is around ~38 cycles. (in the future we might want
 * to do an SIMD/3DNOW!/MMX/FPU 64-bit store here, but that
 * might have an implicit FPU-save as a cost, so it's not
 * clear which path to go.)
 *
 * cmpxchg8b must be used with the lock prefix here to allow
 * the instruction to be executed atomically, see page 3-102
 * of the instruction set reference 24319102.pdf. We need
 * the reader side to see the coherent 64bit value.
 */
#if 0
static __tbx_inline__ void __ma_set_64bit (unsigned long long * ptr,
		unsigned int low, unsigned int high)
{
	__asm__ __volatile__ (
		"\n1:\t"
		"movl (%0), %%eax\n\t"
		"movl 4(%0), %%edx\n\t"
		"lock cmpxchg8b (%0)\n\t"
		"jnz 1b"
		: /* no outputs */
		:	"D"(ptr),
			"b"(low),
			"c"(high)
		:	"ax","dx","memory");
}

static __tbx_inline__ void __ma_set_64bit_constant (unsigned long long *ptr,
						 unsigned long long value)
{
	__ma_set_64bit(ptr,(unsigned int)(value), (unsigned int)((value)>>32ULL));
}
#define ma_ll_low(x)	*(((unsigned int*)(void*)&(x))+0)
#define ma_ll_high(x)	*(((unsigned int*)(void*)&(x))+1)

static __tbx_inline__ void __ma_set_64bit_var (unsigned long long *ptr,
			 unsigned long long value)
{
	__ma_set_64bit(ptr,ma_ll_low(value), ma_ll_high(value));
}

#define ma_set_64bit(ptr,value) \
(__builtin_constant_p(value) ? \
 __ma_set_64bit_constant(ptr, value) : \
 __ma_set_64bit_var(ptr, value) )

#define _ma_set_64bit(ptr,value) \
(__builtin_constant_p(value) ? \
 __ma_set_64bit(ptr, (unsigned int)(value), (unsigned int)((value)>>32ULL) ) : \
 __ma_set_64bit(ptr, ma_ll_low(value), ma_ll_high(value)) )
#endif

/*
 * Note: no "lock" prefix even on SMP: xchg always implies lock anyway
 * Note 2: xchg has side effect, so that attribute volatile is necessary,
 *	  but generally the primitive is invalid, *ptr is output argument. --ANK
 */
static __tbx_inline__ unsigned long __ma_xchg(unsigned long x, volatile void * ptr, int size)
{
	switch (size) {
		case 1:
			__asm__ __volatile__("xchgb %b0,%1"
				:"=q" (x)
				:"m" (*__ma_xg(ptr)), "0" (x)
				:"memory");
			break;
		case 2:
			__asm__ __volatile__("xchgw %w0,%1"
				:"=r" (x)
				:"m" (*__ma_xg(ptr)), "0" (x)
				:"memory");
			break;
		case 4:
			__asm__ __volatile__("xchgl %0,%1"
				:"=r" (x)
				:"m" (*__ma_xg(ptr)), "0" (x)
				:"memory");
			break;
		default:
			MA_BUG();
	}
	return x;
}

/*
 * Atomic compare and exchange.  Compare OLD with MEM, if identical,
 * store NEW in MEM.  Return the initial value in MEM.  Success is
 * indicated by comparing RETURN with OLD.
 */

static __tbx_inline__ unsigned long TBX_NOINST __ma_cmpxchg(volatile void *ptr, unsigned long old,
				      unsigned long new, int size)
{
	unsigned long prev;
	switch (size) {
	case 1:
		__asm__ __volatile__(MA_LOCK_PREFIX "cmpxchgb %b1,%2"
				     : "=a"(prev)
				     : "q"(new), "m"(*__ma_xg(ptr)), "0"(old)
				     : "memory");
		return prev;
	case 2:
		__asm__ __volatile__(MA_LOCK_PREFIX "cmpxchgw %w1,%2"
				     : "=a"(prev)
				     : "q"(new), "m"(*__ma_xg(ptr)), "0"(old)
				     : "memory");
		return prev;
	case 4:
		__asm__ __volatile__(MA_LOCK_PREFIX "cmpxchgl %1,%2"
				     : "=a"(prev)
				     : "q"(new), "m"(*__ma_xg(ptr)), "0"(old)
				     : "memory");
		return prev;
	default:
		MA_BUG();
	}
	return old;
}

#define ma_cmpxchg(ptr,o,n)\
	((__typeof__(*(ptr)))__ma_cmpxchg((ptr),(unsigned long)(o),\
					(unsigned long)(n),sizeof(*(ptr))))
    
#if 0
struct ma_alt_instr { 
	__u8 *instr; 		/* original instruction */
	__u8 *replacement;
	__u8  cpuid;		/* cpuid bit set for replacement */
	__u8  instrlen;		/* length of original instruction */
	__u8  replacementlen; 	/* length of new instruction, <= instrlen */ 
	__u8  pad;
}; 
#endif

/* 
 * Alternative instructions for different CPU types or capabilities.
 * 
 * This allows to use optimized instructions even on generic binary
 * kernels.
 * 
 * length of oldinstr must be longer or equal the length of newinstr
 * It can be padded with nops as needed.
 * 
 * For non barrier like inlines please define new variants
 * without volatile and memory clobber.
 */
#if 0
#define ma_alternative(oldinstr, newinstr, feature) 	\
	asm volatile ("661:\n\t" oldinstr "\n662:\n" 		     \
		      ".section .altinstructions,\"a\"\n"     	     \
		      "  .align 4\n"				       \
		      "  .long 661b\n"            /* label */          \
		      "  .long 663f\n"		  /* new instruction */ 	\
		      "  .byte %c0\n"             /* feature bit */    \
		      "  .byte 662b-661b\n"       /* sourcelen */      \
		      "  .byte 664f-663f\n"       /* replacementlen */ \
		      ".previous\n"						\
		      ".section .altinstr_replacement,\"ax\"\n"			\
		      "663:\n\t" newinstr "\n664:\n"   /* replacement */    \
		      ".previous" :: "i" (feature) : "memory")  
#endif
#define ma_alternative(oldinstr, newinstr, feature) 	\
	asm volatile ("661:\n\t" oldinstr "\n662:\n" 		     \
		      ::: "memory")  


/*
 * Alternative inline assembly with input.
 * 
 * Pecularities:
 * No memory clobber here. 
 * Argument numbers start with 1.
 * Best is to use constraints that are fixed size (like (%1) ... "r")
 * If you use variable sized constraints like "m" or "g" in the 
 * replacement maake sure to pad to the worst case length.
 */
#define ma_alternative_input(oldinstr, newinstr, feature, input)			\
	asm volatile ("661:\n\t" oldinstr "\n662:\n"				\
		      ".section .altinstructions,\"a\"\n"			\
		      "  .align 4\n"						\
		      "  .long 661b\n"            /* label */			\
		      "  .long 663f\n"		  /* new instruction */ 	\
		      "  .byte %c0\n"             /* feature bit */		\
		      "  .byte 662b-661b\n"       /* sourcelen */		\
		      "  .byte 664f-663f\n"       /* replacementlen */ 		\
		      ".previous\n"						\
		      ".section .altinstr_replacement,\"ax\"\n"			\
		      "663:\n\t" newinstr "\n664:\n"   /* replacement */ 	\
		      ".previous" :: "i" (feature), input)  

/*
 * Force strict CPU ordering.
 * And yes, this is required on UP too when we're talking
 * to devices.
 *
 * For now, "wmb()" doesn't actually do anything, as all
 * Intel CPU's follow what Intel calls a *Processor Order*,
 * in which all writes are seen in the program order even
 * outside the CPU.
 *
 * I expect future Intel CPU's to have a weaker ordering,
 * but I'd also expect them to finally get their act together
 * and add some real memory barriers if so.
 *
 * Some non intel clones support out of order store. wmb() ceases to be a
 * nop for these.
 */
 

/* 
 * Actually only lfence would be needed for mb() because all stores done 
 * by the kernel should be already ordered. But keep a full barrier for now. 
 */

#define ma_mb() ma_alternative("lock; addl $0,0(%%esp)", "mfence", X86_FEATURE_XMM2)
#define ma_rmb() ma_alternative("lock; addl $0,0(%%esp)", "lfence", X86_FEATURE_XMM2)

/**
 * read_barrier_depends - Flush all pending reads that subsequents reads
 * depend on.
 *
 * No data-dependent reads from memory-like regions are ever reordered
 * over this barrier.  All reads preceding this primitive are guaranteed
 * to access memory (but not necessarily other CPUs' caches) before any
 * reads following this primitive that depend on the data return by
 * any of the preceding reads.  This primitive is much lighter weight than
 * rmb() on most CPUs, and is never heavier weight than is
 * rmb().
 *
 * These ordering constraints are respected by both the local CPU
 * and the compiler.
 *
 * Ordering is not guaranteed by anything other than these primitives,
 * not even by data dependencies.  See the documentation for
 * memory_barrier() for examples and URLs to more information.
 *
 * For example, the following code would force ordering (the initial
 * value of "a" is zero, "b" is one, and "p" is "&a"):
 *
 * <programlisting>
 *	CPU 0				CPU 1
 *
 *	b = 2;
 *	memory_barrier();
 *	p = &b;				q = p;
 *					read_barrier_depends();
 *					d = *q;
 * </programlisting>
 *
 * because the read of "*q" depends on the read of "p" and these
 * two reads are separated by a read_barrier_depends().  However,
 * the following code, with the same initial values for "a" and "b":
 *
 * <programlisting>
 *	CPU 0				CPU 1
 *
 *	a = 2;
 *	memory_barrier();
 *	b = 3;				y = b;
 *					read_barrier_depends();
 *					x = a;
 * </programlisting>
 *
 * does not enforce ordering, since there is no data dependency between
 * the read of "a" and the read of "b".  Therefore, on some CPUs, such
 * as Alpha, "y" could be set to 3 and "x" to 0.  Use rmb()
 * in cases like thiswhere there are no data dependencies.
 **/

#define ma_read_barrier_depends()	do { } while(0)

#ifdef CONFIG_X86_OOSTORE
/* Actually there are no OOO store capable CPUs for now that do SSE, 
   but make it already an possibility. */
#define ma_wmb() ma_alternative("lock; addl $0,0(%%esp)", "sfence", X86_FEATURE_XMM)
#else
#define ma_wmb()	__asm__ __volatile__ ("": : :"memory")
#endif

#ifdef MA__LWPS
#define ma_smp_mb()	ma_mb()
#define ma_smp_rmb()	ma_rmb()
#define ma_smp_wmb()	ma_wmb()
#define ma_smp_read_barrier_depends()	ma_read_barrier_depends()
#define ma_set_mb(var, value) do { ma_xchg(&var, value); } while (0)
#else
#define ma_smp_mb()	ma_barrier()
#define ma_smp_rmb()	ma_barrier()
#define ma_smp_wmb()	ma_barrier()
#define ma_smp_read_barrier_depends()	do { } while(0)
#define ma_set_mb(var, value) do { var = value; ma_barrier(); } while (0)
#endif

#define ma_set_wmb(var, value) do { var = value; ma_wmb(); } while (0)

/* interrupt control.. */
//#define local_save_flags(x)	do { typecheck(unsigned long,x); __asm__ __volatile__("pushfl ; popl %0":"=g" (x): /* no input */); } while (0)
//#define local_irq_restore(x) 	do { typecheck(unsigned long,x); __asm__ __volatile__("pushl %0 ; popfl": /* no output */ :"g" (x):"memory", "cc"); } while (0)
//#define local_irq_disable() 	__asm__ __volatile__("cli": : :"memory")
//#define local_irq_enable()	__asm__ __volatile__("sti": : :"memory")
/* used in the idle loop; sti takes one instruction cycle to complete */
//#define safe_halt()		__asm__ __volatile__("sti; hlt": : :"memory")

/*
#define irqs_disabled()			\
({					\
	unsigned long flags;		\
	local_save_flags(flags);	\
	!(flags & (1<<9));		\
})
*/

/* For spinlocks etc */
//#define local_irq_save(x)	__asm__ __volatile__("pushfl ; popl %0 ; cli":"=g" (x): /* no input */ :"memory")


