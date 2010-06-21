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


#ifndef __ASM_I386_MARCEL_CTX_H__
#define __ASM_I386_MARCEL_CTX_H__


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal macros **/
/* marcel_create : passage père->fils */
#define marcel_ctx_set_new_stack(new_task, top, cur_top) \
  do { \
    /* on a besoin d'avoir un 0 à *ebp */ \
    unsigned long _local = ((unsigned long)(cur_top)) - get_sp();	\
    unsigned long *_bp = (unsigned long *)(((unsigned long)(top)) - MAL(2*sizeof(unsigned long))); \
    unsigned long _sp = ((unsigned long)_bp) - MAL(_local); \
    /* marqueur de fin de pile */ \
    *_bp = 0; \
    marcel_ctx_set_tls_reg(new_task); \
    MA_SET_INITIAL_SELF(new_task); \
    set_sp_bp(_sp, _bp); \
  } while (0)
/* marcel_deviate : passage temporaire sur une autre pile */

#define marcel_ctx_switch_stack(from_task, to_task, top, cur_top) \
	marcel_ctx_set_new_stack(to_task, top, cur_top)


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#include "asm-generic/marcel_ctx.h"
#endif /** __ASM_I386_MARCEL_CTX_H__ **/
