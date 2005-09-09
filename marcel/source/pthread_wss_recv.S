/* Wrapper arpund system calls to provide cancelation points.
   Copyright (C) 1996-1999,2000-2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1996.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include "pthread_wrapsyscall.h"
#ifdef MA__PTHREAD_FUNCTIONS

 .text
.globl __libc_recv
 .cfi_startproc
 .globl __libc_recv; .type __libc_recv,@function; .align 1<<4; __libc_recv:

 .section .gnu.linkonce.t.__i686.get_pc_thunk.cx,"ax",@progbits; .globl __i686.get_pc_thunk.cx; .hidden __i686.get_pc_thunk.cx; .type __i686.get_pc_thunk.cx,@function; __i686.get_pc_thunk.cx: movl (%esp), %ecx; ret; .size __i686.get_pc_thunk.cx, . - __i686.get_pc_thunk.cx; .previous; call __i686.get_pc_thunk.cx; addl $_GLOBAL_OFFSET_TABLE_, %ecx; cmpl $0, __pthread_multiple_threads@GOTOFF(%ecx)
 jne 1f



 movl %ebx, %edx
 .cfi_register 3, 2

 movl $102, %eax


 movl $10, %ebx
 lea 4(%esp), %ecx


 int $0x80


 movl %edx, %ebx
 .cfi_restore 3


 cmpl $-125, %eax
 jae 0f


.Lpseudo_end:
 ret




1: pushl %esi
 .cfi_adjust_cfa_offset 4


 call __pthread_enable_asynccancel;
 movl %eax, %esi
 .cfi_offset 6, -8


 movl %ebx, %edx
 .cfi_register 3, 2

 movl $102, %eax


 movl $10, %ebx
 lea 8(%esp), %ecx


 int $0x80


 movl %edx, %ebx
 .cfi_restore 3


 xchgl %esi, %eax
 call __pthread_disable_asynccancel


 movl %esi, %eax
 popl %esi
 .cfi_restore 6
 .cfi_adjust_cfa_offset -4


 cmpl $-125, %eax
 jae 0f


 ret

 .cfi_endproc
0:pushl %ebx; .section .gnu.linkonce.t.__i686.get_pc_thunk.bx,"ax",@progbits; .globl __i686.get_pc_thunk.bx; .hidden __i686.get_pc_thunk.bx; .type __i686.get_pc_thunk.bx,@function; __i686.get_pc_thunk.bx: movl (%esp), %ebx; ret; .size __i686.get_pc_thunk.bx, . - __i686.get_pc_thunk.bx; .previous; call __i686.get_pc_thunk.bx; addl $_GLOBAL_OFFSET_TABLE_, %ebx; xorl %edx, %edx; subl %eax, %edx; pushl %edx; ; call __errno_location@PLT; ; popl %ecx; popl %ebx; movl %ecx, (%eax); orl $-1, %eax; jmp .Lpseudo_end; .size __libc_recv,.-__libc_recv;


.weak recv ; recv = __libc_recv
.weak __recv ; __recv = __libc_recv

#endif


