
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
        	  
        .text
        .align 4

#ifdef DARWIN_SYS
#define ma_setjmp _ma_setjmp
#define __sigsetjmp ___sigsetjmp
#define ma_savesigs _ma_savesigs
#endif	
        
#define SAVE \
        movl    4(%esp), %ecx  ; \
        movl    %ebx, 0(%ecx)  ; \
        movl    %esi, 4(%ecx)  ; \
        movl    %edi, 8(%ecx)  ; \
        movl    %ebp, 12(%ecx) ; \
        popl    %edx           ; \
        movl    %edx, 20(%ecx) ; \
        movl    %esp, 16(%ecx) ;

	
        .globl ma_setjmp
#ifndef DARWIN_SYS
#ifndef TBX_DISABLE_VISIBILITY
	.internal ma_setjmp
#endif
	.type  ma_setjmp, @function
#endif
ma_setjmp:
	SAVE
        xorl    %eax, %eax
        jmp     *%edx
#if defined(LINUX_SYS) || defined(GNU_SYS)
.size ma_setjmp,.-ma_setjmp
#endif

#ifdef MA__LIBPTHREAD
        .globl setjmp
#ifndef DARWIN_SYS
	.type  setjmp, @function
#endif
setjmp:
        SAVE
        pushl   %edx
        movl    4(%esp),%eax    // buffer
#ifdef MARCEL_SIGNALS_ENABLED
        pushl   $1
        pushl   %eax
	call	ma_savesigs
        addl    $8, %esp
#endif
        xorl    %eax, %eax
        ret

        .globl _setjmp
	.type  _setjmp, @function
_setjmp:
        SAVE
        pushl   %edx
        movl    4(%esp),%eax    // buffer
#ifdef MARCEL_SIGNALS_ENABLED
        pushl   $0
        pushl   %eax
	call	ma_savesigs
        addl    $8, %esp
#endif
        xorl    %eax, %eax
        ret

        .globl __sigsetjmp
#ifndef DARWIN_SYS
	.type  __sigsetjmp, @function
#endif
__sigsetjmp:
        SAVE
	pushl   %edx
        xorl    %eax, %eax
#ifdef MARCEL_SIGNALS_ENABLED
	jmp	ma_savesigs
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
