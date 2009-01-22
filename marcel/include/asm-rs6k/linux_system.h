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
 * include/asm-ia64/system.h
 *
 * System defines. Note that this is included both from .c and .S
 * files, so it does only defines, not any C code.  This is based
 * on information published in the Processor Abstraction Layer
 * and the System Abstraction Layer manual.
 *
 * Copyright (C) 1998-2003 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 * Copyright (C) 1999 Asit Mallick <asit.k.mallick@intel.com>
 * Copyright (C) 1999 Don Dugger <don.dugger@intel.com>
 */

#section marcel_macros

/*
 * Macros to force memory ordering.  In these descriptions, "previous"
 * and "subsequent" refer to program order; "visible" means that all
 * architecturally visible effects of a memory access have occurred
 * (at a minimum, this means the memory has been read or written).
 *
 *   ma_wmb():	Guarantees that all preceding stores to memory-
 *		like regions are visible before any subsequent
 *		stores and that all following stores will be
 *		visible only after all previous stores.
 *   ma_rmb():	Like wmb(), but for reads.
 *   ma_mb():	wmb()/rmb() combo, i.e., all previous memory
 *		accesses are visible before all subsequent
 *		accesses and vice versa.  This is also known as
 *		a "fence."
 *
 * Note: "ma_mb()" and its variants cannot be used as a fence to order
 * accesses to memory mapped I/O registers.  For that, mf.a needs to
 * be used.  However, we don't want to always use mf.a because (a)
 * it's (presumably) much slower than mf and (b) mf.a is supported for
 * sequential memory pages only.
 */

#include "tbx_compiler.h"
#error "to check !"
#define ma_mb()		tbx_mb()
#define ma_rmb()	ma_mb()
#define ma_wmb()	ma_mb()
#define ma_read_barrier_depends()	do { } while(0)

#ifdef MA__LWPS
# define ma_smp_mb()	ma_mb()
# define ma_smp_rmb()	ma_rmb()
# define ma_smp_wmb()	ma_wmb()
# define ma_smp_read_barrier_depends()	ma_read_barrier_depends()
#else
# define ma_smp_mb()	ma_barrier()
# define ma_smp_rmb()	ma_barrier()
# define ma_smp_wmb()	ma_barrier()
# define ma_smp_read_barrier_depends()	do { } while(0)
#endif

/*
 * XXX check on these---I suspect what Linus really wants here is
 * acquire vs release semantics but we can't discuss this stuff with
 * Linus just yet.  Grrr...
 */
#define ma_set_mb(var, value)	do { (var) = (value); ma_mb(); } while (0)
#define ma_set_wmb(var, value)	do { (var) = (value); ma_wmb(); } while (0)

#define ma_cpu_relax() ma_barrier()
