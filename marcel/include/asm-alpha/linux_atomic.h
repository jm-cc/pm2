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


#ifndef __ASM_ALPHA_LINUX_ATOMIC_H__
#define __ASM_ALPHA_LINUX_ATOMIC_H__


/*
 * Atomic operations that C can't guarantee us.  Useful for
 * resource counting etc...
 *
 * But use these as seldom as possible since they are much slower
 * than regular operations.
 */


#ifdef OSF_SYS
#include "asm-generic/linux_atomic.h"
#else
#include "tbx_compiler.h"
#include "asm-alpha/linux_compiler.h"
#endif


/** Public data types **/
/*
 * Counter is volatile to make sure gcc doesn't try to be clever
 * and move things around on us. We need to use _exactly_ the address
 * the user gave us, not some alias that contains the same information.
 */
#ifndef OSF_SYS
typedef struct { volatile int counter; } ma_atomic_t;
typedef struct { volatile long counter; } ma_atomic64_t;
#endif


#ifdef __MARCEL_KERNEL__


#ifndef OSF_SYS


/** Internal macros **/
/*
 * Counter is volatile to make sure gcc doesn't try to be clever
 * and move things around on us. We need to use _exactly_ the address
 * the user gave us, not some alias that contains the same information.
 */
#define MA_ATOMIC_INIT(i)	( (ma_atomic_t) { (i) } )
#define MA_ATOMIC64_INIT(i)	( (ma_atomic64_t) { (i) } )

#define ma_atomic_read(v)	((v)->counter + 0)
#define ma_atomic64_read(v)	((v)->counter + 0)

#define ma_atomic_set(v,i)	((v)->counter = (i))
#define ma_atomic64_set(v,i)	((v)->counter = (i))

#define ma_atomic_init(v,i)	ma_atomic_set((v),(i))
#define ma_atomic64_init(v,i)	ma_atomic64_set((v),(i))

/*
 * To get proper branch prediction for the main line, we must branch
 * forward to code at the end of this object's .text section, then
 * branch back to restart the operation.
 */
#define ma_atomic_add_negative(a, v)	(ma_atomic_add_return((a), (v)) < 0)

#define ma_atomic_dec_return(v) ma_atomic_sub_return(1,(v))
#define ma_atomic64_dec_return(v) ma_atomic64_sub_return(1,(v))

#define ma_atomic_inc_return(v) ma_atomic_add_return(1,(v))
#define ma_atomic64_inc_return(v) ma_atomic64_add_return(1,(v))

#define ma_atomic_sub_and_test(i,v) (ma_atomic_sub_return((i), (v)) == 0)
#define ma_atomic64_sub_and_test(i,v) (ma_atomic64_sub_return((i), (v)) == 0)

#define ma_atomic_dec_and_test(v) (ma_atomic_sub_return(1, (v)) == 0)
#define ma_atomic64_dec_and_test(v) (ma_atomic64_sub_return(1, (v)) == 0)

#define ma_atomic_inc(v) ma_atomic_add(1,(v))
#define ma_atomic64_inc(v) ma_atomic64_add(1,(v))

#define ma_atomic_dec(v) ma_atomic_sub(1,(v))
#define ma_atomic64_dec(v) ma_atomic64_sub(1,(v))

#define ma_atomic_xchg(o,r,v) ma_cmpxchg(&(v)->counter,o,r)
#define ma_atomic64_xchg(o,r,v) ma_cmpxchg(&(v)->counter,o,r)

#define ma_smp_mb__before_atomic_dec()	ma_smp_mb()
#define ma_smp_mb__after_atomic_dec()	ma_smp_mb()
#define ma_smp_mb__before_atomic_inc()	ma_smp_mb()
#define ma_smp_mb__after_atomic_inc()	ma_smp_mb()


/** Internal functions **/
/*
 * Counter is volatile to make sure gcc doesn't try to be clever
 * and move things around on us. We need to use _exactly_ the address
 * the user gave us, not some alias that contains the same information.
 */
static __tbx_inline__ void ma_atomic_add(int i, ma_atomic_t * v);
static __tbx_inline__ void ma_atomic64_add(long i, ma_atomic64_t * v);
static __tbx_inline__ void ma_atomic_sub(int i, ma_atomic_t * v);
static __tbx_inline__ void ma_atomic64_sub(long i, ma_atomic64_t * v);
static __tbx_inline__ long ma_atomic_add_return(int i, ma_atomic_t * v);
static __tbx_inline__ long ma_atomic64_add_return(long i, ma_atomic64_t * v);
static __tbx_inline__ long ma_atomic_sub_return(int i, ma_atomic_t * v);
static __tbx_inline__ long ma_atomic64_sub_return(long i, ma_atomic64_t * v);


#endif /** ! OSF_SYS **/


#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_ALPHA_LINUX_ATOMIC_H__ **/
