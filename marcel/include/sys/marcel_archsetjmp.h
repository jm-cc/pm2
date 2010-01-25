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
#include <tbx_compiler.h>
#include <stdint.h>


#if defined(ALPHA_ARCH) && defined(LINUX_SYS)

#ifdef setjmp
#undef setjmp
#endif

#define setjmp(env) __sigsetjmp ((env), 0)

#endif

#if defined(RS6K_ARCH)

_PRIVATE_ extern int _jmg(int r);
_PRIVATE_ extern TBX_NORETURN void LONGJMP(jmp_buf buf, int val);
#ifdef setjmp
#undef setjmp
#endif

#ifdef longjmp
#undef longjmp
#endif

#define setjmp(buf)		_jmg(setjmp(buf))
#define longjmp(buf, v)		LONGJMP(buf, v)

#endif

#if defined(X86_ARCH)
#define MA_JMPBUF

#define MARCEL_JB_BX   0
#define MARCEL_JB_SI   1
#define MARCEL_JB_DI   2
#define MARCEL_JB_BP   3
#define MARCEL_JB_SP   4
#define MARCEL_JB_PC   5

typedef intptr_t ma_jmp_buf[6];

extern int TBX_RETURNS_TWICE ma_setjmp(ma_jmp_buf buf);

static __tbx_inline__ void TBX_NORETURN TBX_UNUSED ma_longjmp(ma_jmp_buf buf, int val);

#elif defined(X86_64_ARCH)
#define MA_JMPBUF

#define MARCEL_JB_RBX   0
#define MARCEL_JB_RBP   1
#define MARCEL_JB_R12   2
#define MARCEL_JB_R13   3
#define MARCEL_JB_R14   4
#define MARCEL_JB_R15   5
#define MARCEL_JB_RSP   6
#define MARCEL_JB_PC    7

typedef intptr_t ma_jmp_buf[8];

extern int TBX_RETURNS_TWICE ma_setjmp(ma_jmp_buf buf);

static __tbx_inline__ void TBX_NORETURN TBX_UNUSED ma_longjmp(ma_jmp_buf buf, int val);
#endif

#ifdef MA_JMPBUF
#define jmp_buf ma_jmp_buf
#undef setjmp
#define setjmp ma_setjmp
#undef longjmp
#define longjmp ma_longjmp
#endif


#endif /* __SYS_MARCEL_ARCHSETJMP_H__ */
