//#ifndef _ASM_IA64_TYPES_H
//#define _ASM_IA64_TYPES_H

/*
 * This file is never included by application software unless
 * explicitly requested (e.g., via linux/types.h) in which case the
 * application is Linux specific so (user-) name space pollution is
 * not a major issue.  However, for interoperability, libraries still
 * need to be careful to avoid a name clashes.
 *
 * Copyright (C) 1998-2000 Hewlett-Packard Co
 * Copyright (C) 1998-2000 David Mosberger-Tang <davidm@hpl.hp.com>
 */


#section marcel_types

/*
 #ifdef __ASSEMBLY__
 # define __IA64_UL(x)		(x)
 # define __IA64_UL_CONST(x)	x
 #else
 # define __IA64_UL(x)		((unsigned long)(x))
 # define __IA64_UL_CONST(x)	x##UL
 #endif

 #ifndef __ASSEMBLY__

// typedef unsigned int umode_t;
*/

/*
 * __xx is ok: it doesn't pollute the POSIX namespace. Use these in the
 * header files exported to user space
 */

typedef __signed__ char __ma_s8;
typedef unsigned char __ma_u8;

typedef __signed__ short __ma_s16;
typedef unsigned short __ma_u16;

typedef __signed__ int __ma_s32;
typedef unsigned int __ma_u32;

typedef __signed__ long __ma_s64;
typedef unsigned long __ma_u64;

/*
 * These aren't exported outside the kernel to avoid name space clashes
 */
//# ifdef __KERNEL__

typedef __ma_s8 ma_s8;
typedef __ma_u8 ma_u8;

typedef __ma_s16 ma_s16;
typedef __ma_u16 ma_u16;

typedef __ma_s32 ma_s32;
typedef __ma_u32 ma_u32;

typedef __ma_s64 ma_s64;
typedef __ma_u64 ma_u64;

/* DMA addresses are 64-bits wide, in general.  */

typedef ma_u64 ma_dma_addr_t;

//# endif /* __KERNEL__ */
//#endif /* !__ASSEMBLY__ */

//#endif /* _ASM_IA64_TYPES_H */
#section macros
#define MA_BITS_PER_LONG 64

