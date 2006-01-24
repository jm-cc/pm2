
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

#define TOP_STACK_FREE_AREA     
#define SP_FIELD(buf)           
#define BSP_FIELD(buf)          
#define PC_FIELD(buf)           

#define call_ST_FLUSH_WINDOWS()  

#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
#define get_sp() \
({ \
  register unsigned long sp asm("sp"); \
  sp; \
})
#else
#depend "asm-generic/marcel_archdep.h[marcel_macros]"
#endif

#define set_sp(val)

#section marcel_variables
