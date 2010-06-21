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


#ifndef __ASM_I386_LINUX_SYSTEM_H__
#define __ASM_I386_LINUX_SYSTEM_H__


#include <stdlib.h>
#include "tbx_compiler.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal macros **/
#define ma_nop() __asm__ __volatile__ ("nop")
#define ma_xchg(ptr,v) ((__typeof__(*(ptr)))__ma_xchg((unsigned long)(v),(ptr),sizeof(*(ptr))))
#define ma_tas(ptr) (xchg((ptr),1))
    struct __ma_xchg_dummy {
	unsigned long a[100];
};
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

/*
 * Note: no "lock" prefix even on SMP: xchg always implies lock anyway
 * Note 2: xchg has side effect, so that attribute volatile is necessary,
 *	  but generally the primitive is invalid, *ptr is output argument. --ANK
 */
#define ma_xchg(ptr,v) ((__typeof__(*(ptr)))__ma_xchg((unsigned long)(v),(ptr),sizeof(*(ptr))))

/*
 * Atomic compare and exchange.  Compare OLD with MEM, if identical,
 * store NEW in MEM.  Return the initial value in MEM.  Success is
 * indicated by comparing RETURN with OLD.
 */
#define ma_cmpxchg(ptr,o,n)\
	((__typeof__(*(ptr)))__ma_cmpxchg((ptr),(unsigned long)(o),\
					(unsigned long)(n),sizeof(*(ptr))))

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
#define ma_alternative(oldinstr, newinstr, feature) 	\
	asm volatile (oldinstr ::: "memory")


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
#define ma_read_barrier_depends()	do { } while(0)

#define ma_mb() ma_alternative("lock; addl $0,0(%%esp)", "mfence", X86_FEATURE_XMM2)
/*
 * Some non-Intel clones support out of order store. wmb() ceases to be a
 * nop for these.
 */
#define ma_rmb() ma_alternative("lock; addl $0,0(%%esp)", "lfence", X86_FEATURE_XMM2)
#define ma_wmb() ma_alternative("lock; addl $0,0(%%esp)", "sfence", X86_FEATURE_XMM)


#ifdef MA__LWPS
#define ma_smp_mb()	ma_mb()
#define ma_smp_rmb()	ma_rmb()
#define ma_smp_wmb()	ma_wmb()
#define ma_smp_read_barrier_depends()	ma_read_barrier_depends()
#define ma_set_mb(var, value) do { (void) ma_xchg(&var, value); } while (0)
#else
#define ma_smp_mb()	ma_barrier()
#define ma_smp_rmb()	ma_barrier()
#define ma_smp_wmb()	ma_barrier()
#define ma_smp_read_barrier_depends()	do { } while(0)
#define ma_set_mb(var, value) do { var = value; ma_barrier(); } while (0)
#endif

#define ma_set_wmb(var, value) do { var = value; ma_wmb(); } while (0)

#define ma_cpu_relax() asm volatile("rep; nop" ::: "memory")


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_I386_LINUX_SYSTEM_H__ **/
