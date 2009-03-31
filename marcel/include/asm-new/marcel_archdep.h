
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

#ifdef MA__PROVIDE_TLS
typedef struct {
  /* LPT binary compatibility */

  /* Needs to be the same as glibc's tcbhead_t structure, see glibc/nptl/sysdeps/<yourarch>/tls.h */
  /* Fill the various fields from marcel/source/marcel_alloc.c */

  /* LPT binary compatibility end */
} lpt_tcb_t;

/*
 * Define this if it is defined in glibc/nptl/sysdeps/<yourarch>/tls.h>,
 * meaning that the multiple_threads flag is in the TCB described above and not
 * in the global variable whose address is returned by __libc_pthread_init().
 */
/* #define MA_TLS_MULTIPLE_THREADS_IN_TCB */

/*
 * Define this according to your TLS variant
 * See marcel/include/asm-ia64/marcel_archdep.h for a Variant I example, and
 * marcel/include/asm-i386/marcel_archdep.h for a Variant II example
 * See Drepper's paper about TLS for the details and which one your arch uses.
 */
#define marcel_tcb(new_task)

/*
 * How to set the arch-specific TLS reg for the new task
 */
#define marcel_ctx_set_tls_reg(new_task)

#else
#define marcel_ctx_set_tls_reg(new_task) (void)0
#endif
