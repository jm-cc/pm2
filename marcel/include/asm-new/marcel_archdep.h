
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

#section marcel_macros
#include "tbx_compiler.h"

#include "sys/marcel_flags.h"
#include "sys/marcel_win_sys.h"

#error "to write !"

/*
 * Amount of room to keep free on top of the stack when creating threads, for
 * e.g. local data, sparc-like register window, etc.
 * If unsure, try 128.
 */
#define TOP_STACK_FREE_AREA     

/*
 * How to access the stack pointer field in a jmp_buf.
 */
#define SP_FIELD(buf)           

/* Same for PC, BSP and FP, if these exist.  */
#define PC_FIELD(buf)           
#define BSP_FIELD(buf)          
#define FP_FIELD(buf)          

/*
 * How to flush a sparc-like register window, if any.
 */
#define call_ST_FLUSH_WINDOWS()  

#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
/*
 * How to get the current stack pointer register, in the gcc or intel case it's
 * usually just a matter of writing the register name below.
 */
#define get_sp() \
({ \
  register unsigned long sp asm(""); \
  sp; \
})
#else
/*
 * Else we have a generic ugly way to do it.
 */
#depend "asm-generic/marcel_archdep.h[marcel_macros]"
#endif

/*
 * How to set the stack pointer register
 */
#define set_sp(val)
