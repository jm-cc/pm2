
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
        
#if defined(FREEBSD_SYS) || defined(DARWIN_SYS)
#define ma_setjmp _ma_setjmp
#define __sigsetjmp ___sigsetjmp
#define ma_savesigs _ma_savesigs
#endif
        .globl ma_setjmp
        .internal ma_setjmp
	.type  ma_setjmp, @function
ma_setjmp:
        movl    4(%esp), %ecx
        movl    %ebx, 0(%ecx)
        movl    %esi, 4(%ecx)
        movl    %edi, 8(%ecx)
        movl    %ebp, 12(%ecx)
        popl	%edx
        movl    %edx, 20(%ecx)
        movl    %esp, 16(%ecx)
        xorl    %eax, %eax
        jmp     *%edx

#if defined(LINUX_SYS) || defined(GNU_SYS)
.size ma_setjmp,.-ma_setjmp
#endif

#ifdef MA__LIBPTHREAD

#define SAVE \
        movl    4(%esp), %ecx  ; \
        movl    %ebx, 0(%ecx)  ; \
        movl    %esi, 4(%ecx)  ; \
        movl    %edi, 8(%ecx)  ; \
        movl    %ebp, 12(%ecx) ; \
        popl    %edx           ; \
        movl    %edx, 20(%ecx) ; \
        movl    %esp, 16(%ecx) ; \
        pushl   %edx

        .globl setjmp
	.type  setjmp, @function
setjmp:
        SAVE
        movl    4(%esp),%eax    // buffer
        pushl   $1
        pushl   %eax
        call    ma_savesigs
        addl    $8, %esp
        xorl    %eax, %eax
        ret

        .globl _setjmp
	.type  _setjmp, @function
_setjmp:
        SAVE
        movl    4(%esp),%eax    // buffer
        pushl   $0
        pushl   %eax
        call    ma_savesigs
        addl    $8, %esp
        xorl    %eax, %eax
        ret

        .globl __sigsetjmp
__sigsetjmp:
        SAVE
        xorl    %eax, %eax
#ifdef __PIC__
        jmp     ma_savesigs@PLT
#else
        jmp     ma_savesigs
#endif

#if defined(LINUX_SYS) || defined(GNU_SYS)
.size __sigsetjmp,.-__sigsetjmp
.size _setjmp,.-_setjmp
.size setjmp,.-setjmp
#endif
#endif
        	  
#if !(defined(LINUX_SYS) || defined(GNU_SYS))
        .globl ma_i386_spinlock_contention
	.type  ma_i386_spinlock_contention, @function
ma_i386_spinlock_contention:
re:
        rep; nop
        cmpb $0,(%eax)
        jle re
        ret
#endif
