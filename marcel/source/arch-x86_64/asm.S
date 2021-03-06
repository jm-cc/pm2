
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

#include <sys/marcel_flags.h>
#include "tbx_compiler.h"
		  
	.text
	.align 8

#ifdef DARWIN_SYS
#define ma_setjmp _ma_setjmp
#define __sigsetjmp ___sigsetjmp
#define ma_savesigs _ma_savesigs
#endif
	
	.globl ma_setjmp
#ifndef DARWIN_SYS
#ifndef TBX_DISABLE_VISIBILITY
	.internal ma_setjmp
#endif
	.type  ma_setjmp,@function
#endif
ma_setjmp:
        movq    %rbx, 0(%rdi)
        movq    %rbp, 8(%rdi)
        movq    %r12, 16(%rdi)
        movq    %r13, 24(%rdi)
        movq    %r14, 32(%rdi)
        movq    %r15, 40(%rdi)
        popq    %rdx
        movq    %rsp, 48(%rdi)
        movq    %rdx, 56(%rdi)
        xorq    %rax, %rax
        jmp     *%rdx

#if defined(LINUX_SYS) || defined(GNU_SYS)
.size ma_setjmp,.-ma_setjmp
#endif

#ifdef MA__LIBPTHREAD
        .globl setjmp
#ifndef DARWIN_SYS
	.type  setjmp,@function
#endif
setjmp:
        movl    $1,%esi
#ifdef __PIC__
        jmp     __sigsetjmp@PLT
#else
        jmp     __sigsetjmp
#endif

        .globl _setjmp
#ifndef DARWIN_SYS
	.type  _setjmp,@function
#endif
_setjmp:
        xorl    %esi,%esi
        // fallthrough to __sigsetjmp

        .globl __sigsetjmp
#ifndef DARWIN_SYS
	.type  __sigsetjmp,@function
#endif
__sigsetjmp:
        movq    %rbx, 0(%rdi)
        movq    %rbp, 8(%rdi)
        movq    %r12, 16(%rdi)
        movq    %r13, 24(%rdi)
        movq    %r14, 32(%rdi)
        movq    %r15, 40(%rdi)
        lea     0x8(%rsp),%rdx
        movq    %rdx, 48(%rdi)
        movq    (%rsp), %rdx
        movq    %rdx, 56(%rdi)

#ifdef MARCEL_SIGNALS_ENABLED
#  ifdef __PIC__
        jmp     ma_savesigs@PLT
#  else
        jmp     ma_savesigs
#  endif
#else
	xorl	%eax, %eax
	ret
#endif

#if defined(LINUX_SYS) || defined(GNU_SYS)
.size __sigsetjmp,.-__sigsetjmp
.size _setjmp,.-_setjmp
.size setjmp,.-setjmp
#endif
#endif
