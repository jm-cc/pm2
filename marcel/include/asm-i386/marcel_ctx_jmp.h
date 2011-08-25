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


#ifndef __ASM_I386_MARCEL_CTX_JMP_H__
#define __ASM_I386_MARCEL_CTX_JMP_H__


#include <setjmp.h>
#include "tbx_compiler.h"
#include "tbx_types.h"
#include "asm/marcel_archdep.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal macros **/
#define jmp_buf ma_jmp_buf
#undef setjmp
#define setjmp ma_setjmp
#undef longjmp
#define longjmp ma_longjmp


/** Internal functions **/
extern int TBX_RETURNS_TWICE ma_setjmp(ma_jmp_buf buf);
static __tbx_inline__ void TBX_NORETURN TBX_UNUSED ma_longjmp(ma_jmp_buf buf, int val);


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#include "asm-generic/marcel_ctx_jmp.h"


#endif /** __ASM_I386_MARCEL_CTX_JMP_H__ **/
