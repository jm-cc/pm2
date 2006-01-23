
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
 * include/asm-generic/percpu.h
 */

#section marcel_macros
#define __MA_PERLWP_SECTION ".ma.per.lwp"

#define __MA_GENERIC_PER_LWP
#ifdef MA__LWPS

//extern int __ma_per_lwp_data asm(__MA_PERLWP_SECTION);
//__attribute__ ((visibility ("protected")));

/* Separate out the type, so (int[3], foo) works. */
/*#define MA_DEFINE_PER_LWP_SUBSEC(subsec, type, name) \
    TBX_SECTION(__MA_PERLWP_SECTION ".offset" \
                                     subsec ",\"\",@progbits \n#") \
       __typeof__(type) ma_per_lwp__##name; \
    TBX_SECTION(__MA_PERLWP_SECTION ".main" subsec) \
       __typeof__(type) ma_per_lwp_main__##name
*/
#define MA_DEFINE_PER_LWP_SUBSEC(subsec, type, name) \
    TBX_SECTION(__MA_PERLWP_SECTION ".main" subsec) \
       __typeof__(type) ma_per_lwp__##name

extern unsigned long __ma_main_lwp_start;
/* var is in discarded region: offset to particular copy we want */
#define ma_per_lwp(var, lwp) (*TBX_RELOC_HIDE(&ma_per_lwp__##var, \
   ((unsigned long)(ma_lwp_t)(lwp) - (unsigned long)&__main_lwp)))
//   ((ma_lwp_t)(lwp))->per_lwp_offset))
#define __ma_get_lwp_var(var) ma_per_lwp(var, LWP_SELF)

///* Défini dans le script du linker */
//extern unsigned long __ma_per_lwp_end;
//#define __ma_per_lwp_size ((size_t)&__ma_per_lwp_end-(size_t)&__ma_per_lwp_start)

#else /* ! MA__LWPS */

#define MA_DEFINE_PER_LWP_SUBSEC(subsec, type, name) \
       __typeof__(type) ma_per_lwp__##name

#define ma_per_lwp(var, lwp)		(*((void)lwp, &ma_per_lwp__##var))
#define __ma_get_lwp_var(var)		ma_per_lwp__##var

#endif	/* MA__LWPS */

/* Définis dans le script du linker */
extern unsigned long __ma_main_lwp_start;
extern unsigned long __ma_main_lwp_end;
#define __ma_per_lwp_size ((size_t)((unsigned long)&__ma_main_lwp_end \
                                    -(unsigned long)&__ma_main_lwp_start))

#define MA_DEFINE_PER_LWP(type, name, init) MA_DEFINE_PER_LWP_SUBSEC("", type, name) = init
#define MA_DECLARE_PER_LWP(type, name) extern __typeof__(type) ma_per_lwp__##name

