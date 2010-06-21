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


#ifndef __SYS_MARCEL_ARCHSETJMP_H__
#define __SYS_MARCEL_ARCHSETJMP_H__


#include <setjmp.h>
#include "tbx_compiler.h"
#include "tbx_intdef.h"
#include "asm/marcel_archdep.h"


#if defined(__MARCEL_KERNEL__)
TBX_VISIBILITY_PUSH_INTERNAL


#ifdef MA_JMPBUF

/** Internal macros **/
#define jmp_buf ma_jmp_buf
#undef setjmp
#define setjmp ma_setjmp
#undef longjmp
#define longjmp ma_longjmp


/** Internal functions **/
extern int TBX_RETURNS_TWICE ma_setjmp(ma_jmp_buf buf);
static __tbx_inline__ void TBX_NORETURN TBX_UNUSED ma_longjmp(ma_jmp_buf buf, int val);

#endif


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif				/* __SYS_MARCEL_ARCHSETJMP_H__ */
