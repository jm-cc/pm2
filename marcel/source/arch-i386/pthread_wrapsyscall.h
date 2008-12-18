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

#include "sys/marcel_flags.h"
#if 0

/* 0 */

.text; .globl __libc_write; .type __libc_write,@function; .align 1<<4; __libc_write: .section .gnu.linkonce.t.__i686.get_pc_thunk.cx,"ax",@progbits; .globl __i686.get_pc_thunk.cx; .hidden __i686.get_pc_thunk.cx; .type __i686.get_pc_thunk.cx,@function; __i686.get_pc_thunk.cx: movl (%esp), %ecx; ret; .size __i686.get_pc_thunk.cx, . - __i686.get_pc_thunk.cx; .previous; call __i686.get_pc_thunk.cx; addl $_GLOBAL_OFFSET_TABLE_, %ecx; cmpl $0, __pthread_multiple_threads@GOTOFF(%ecx); jne .Lpseudo_cancel; movl $4, %eax; int $0x80 ; cmpl $-4095, %eax; jae 0f; ret; .Lpseudo_cancel: call __pthread_enable_asynccancel; movl %eax, %ecx; movl $4, %eax; int $0x80 ; pushl %eax; movl %ecx, %eax; call __pthread_disable_asynccancel; popl %eax; cmpl $-4095, %eax; jae 0f; .Lpseudo_end:
ret
0:pushl %ebx; .section .gnu.linkonce.t.__i686.get_pc_thunk.bx,"ax",@progbits; .globl __i686.get_pc_thunk.bx; .hidden __i686.get_pc_thunk.bx; .type __i686.get_pc_thunk.bx,@function; __i686.get_pc_thunk.bx: movl (%esp), %ebx; ret; .size __i686.get_pc_thunk.bx, . - __i686.get_pc_thunk.bx; .previous; call __i686.get_pc_thunk.bx; addl $_GLOBAL_OFFSET_TABLE_, %ebx; xorl %edx, %edx; subl %eax, %edx; pushl %edx; ; call __errno_location@PLT; ; popl %ecx; popl %ebx; movl %ecx, (%eax); orl $-1, %eax; jmp .Lpseudo_end; .size __libc_write,.-__libc_write;

.weak __write ; __write = __libc_write

.weak write ; write = __libc_write

/* 1 */

.text; .globl __libc_write; .type __libc_write,@function; .align 1<<4; __libc_write: .section .gnu.linkonce.t.__i686.get_pc_thunk.cx,"ax",@progbits; .globl __i686.get_pc_thunk.cx; .hidden __i686.get_pc_thunk.cx; .type __i686.get_pc_thunk.cx,@function; __i686.get_pc_thunk.cx: movl (%esp), %ecx; ret; .size __i686.get_pc_thunk.cx, . - __i686.get_pc_thunk.cx; .previous; call __i686.get_pc_thunk.cx; addl $_GLOBAL_OFFSET_TABLE_, %ecx; cmpl $0, __pthread_multiple_threads@GOTOFF(%ecx); jne .Lpseudo_cancel; movl %ebx, %edx; .LSAVEBX1: movl 4(%esp), %ebx; movl $4, %eax; int $0x80 ; movl %edx, %ebx; .LRESTBX1:; cmpl $-4095, %eax; jae 0f; ret; .Lpseudo_cancel: call __pthread_enable_asynccancel; movl %eax, %ecx; movl %ebx, %edx; movl 4(%esp), %ebx; movl $4, %eax; int $0x80 ; movl %edx, %ebx; pushl %eax; movl %ecx, %eax; call __pthread_disable_asynccancel; popl %eax; cmpl $-4095, %eax; jae 0f; .Lpseudo_end:
ret
0:pushl %ebx; .section .gnu.linkonce.t.__i686.get_pc_thunk.bx,"ax",@progbits; .globl __i686.get_pc_thunk.bx; .hidden __i686.get_pc_thunk.bx; .type __i686.get_pc_thunk.bx,@function; __i686.get_pc_thunk.bx: movl (%esp), %ebx; ret; .size __i686.get_pc_thunk.bx, . - __i686.get_pc_thunk.bx; .previous; call __i686.get_pc_thunk.bx; addl $_GLOBAL_OFFSET_TABLE_, %ebx; xorl %edx, %edx; subl %eax, %edx; pushl %edx; ; call __errno_location@PLT; ; popl %ecx; popl %ebx; movl %ecx, (%eax); orl $-1, %eax; jmp .Lpseudo_end; .size __libc_write,.-__libc_write;

.weak __write ; __write = __libc_write

.weak write ; write = __libc_write

/* 2 */

.text; .globl __libc_write; .type __libc_write,@function; .align 1<<4; __libc_write: .section .gnu.linkonce.t.__i686.get_pc_thunk.cx,"ax",@progbits; .globl __i686.get_pc_thunk.cx; .hidden __i686.get_pc_thunk.cx; .type __i686.get_pc_thunk.cx,@function; __i686.get_pc_thunk.cx: movl (%esp), %ecx; ret; .size __i686.get_pc_thunk.cx, . - __i686.get_pc_thunk.cx; .previous; call __i686.get_pc_thunk.cx; addl $_GLOBAL_OFFSET_TABLE_, %ecx; cmpl $0, __pthread_multiple_threads@GOTOFF(%ecx); jne .Lpseudo_cancel; movl %ebx, %edx; .LSAVEBX1: movl 8(%esp), %ecx; movl 8 -4(%esp), %ebx; movl $4, %eax; int $0x80 ; movl %edx, %ebx; .LRESTBX1:; cmpl $-4095, %eax; jae 0f; ret; .Lpseudo_cancel: call __pthread_enable_asynccancel; pushl %eax; movl %ebx, %edx; movl 12(%esp), %ecx; movl 12 -4(%esp), %ebx; movl $4, %eax; int $0x80 ; movl %edx, %ebx; xchgl (%esp), %eax; call __pthread_disable_asynccancel; popl %eax; cmpl $-4095, %eax; jae 0f; .Lpseudo_end:
ret
0:pushl %ebx; .section .gnu.linkonce.t.__i686.get_pc_thunk.bx,"ax",@progbits; .globl __i686.get_pc_thunk.bx; .hidden __i686.get_pc_thunk.bx; .type __i686.get_pc_thunk.bx,@function; __i686.get_pc_thunk.bx: movl (%esp), %ebx; ret; .size __i686.get_pc_thunk.bx, . - __i686.get_pc_thunk.bx; .previous; call __i686.get_pc_thunk.bx; addl $_GLOBAL_OFFSET_TABLE_, %ebx; xorl %edx, %edx; subl %eax, %edx; pushl %edx; ; call __errno_location@PLT; ; popl %ecx; popl %ebx; movl %ecx, (%eax); orl $-1, %eax; jmp .Lpseudo_end; .size __libc_write,.-__libc_write;

.weak __write ; __write = __libc_write

.weak write ; write = __libc_write

/* 3 */

.text; .globl __libc_write; .type __libc_write,@function; .align 1<<4; __libc_write: .section .gnu.linkonce.t.__i686.get_pc_thunk.cx,"ax",@progbits; .globl __i686.get_pc_thunk.cx; .hidden __i686.get_pc_thunk.cx; .type __i686.get_pc_thunk.cx,@function; __i686.get_pc_thunk.cx: movl (%esp), %ecx; ret; .size __i686.get_pc_thunk.cx, . - __i686.get_pc_thunk.cx; .previous; call __i686.get_pc_thunk.cx; addl $_GLOBAL_OFFSET_TABLE_, %ecx; cmpl $0, __pthread_multiple_threads@GOTOFF(%ecx); jne .Lpseudo_cancel; pushl %ebx; .LPUSHBX1: movl 16(%esp), %edx; movl 16 -4(%esp), %ecx; movl 16 -4 -4(%esp), %ebx; movl $4, %eax; int $0x80 ; popl %ebx; .LPOPBX1:; cmpl $-4095, %eax; jae 0f; ret; .Lpseudo_cancel: call __pthread_enable_asynccancel; pushl %eax; pushl %ebx; .LPUSHBX2: movl 20(%esp), %edx; movl 20 -4(%esp), %ecx; movl 20 -4 -4(%esp), %ebx; movl $4, %eax; int $0x80 ; popl %ebx; .LPOPBX2:; xchgl (%esp), %eax; call __pthread_disable_asynccancel; popl %eax; cmpl $-4095, %eax; jae 0f; .Lpseudo_end:
ret
0:pushl %ebx; .section .gnu.linkonce.t.__i686.get_pc_thunk.bx,"ax",@progbits; .globl __i686.get_pc_thunk.bx; .hidden __i686.get_pc_thunk.bx; .type __i686.get_pc_thunk.bx,@function; __i686.get_pc_thunk.bx: movl (%esp), %ebx; ret; .size __i686.get_pc_thunk.bx, . - __i686.get_pc_thunk.bx; .previous; call __i686.get_pc_thunk.bx; addl $_GLOBAL_OFFSET_TABLE_, %ebx; xorl %edx, %edx; subl %eax, %edx; pushl %edx; ; call __errno_location@PLT; ; popl %ecx; popl %ebx; movl %ecx, (%eax); orl $-1, %eax; jmp .Lpseudo_end; .size __libc_write,.-__libc_write;

.weak __write ; __write = __libc_write

.weak write ; write = __libc_write

/* 4 */

.text; .globl __libc_write; .type __libc_write,@function; .align 1<<4; __libc_write: .section .gnu.linkonce.t.__i686.get_pc_thunk.cx,"ax",@progbits; .globl __i686.get_pc_thunk.cx; .hidden __i686.get_pc_thunk.cx; .type __i686.get_pc_thunk.cx,@function; __i686.get_pc_thunk.cx: movl (%esp), %ecx; ret; .size __i686.get_pc_thunk.cx, . - __i686.get_pc_thunk.cx; .previous; call __i686.get_pc_thunk.cx; addl $_GLOBAL_OFFSET_TABLE_, %ecx; cmpl $0, __pthread_multiple_threads@GOTOFF(%ecx); jne .Lpseudo_cancel; pushl %esi; .LPUSHSI1: pushl %ebx; .LPUSHBX1: movl 24(%esp), %esi; movl 24 -4(%esp), %edx; movl 24 -4 -4(%esp), %ecx; movl 24 -4 -4 -4(%esp), %ebx; movl $4, %eax; int $0x80 ; popl %ebx; .LPOPBX1:; popl %esi; .LPOPSI1:; cmpl $-4095, %eax; jae 0f; ret; .Lpseudo_cancel: call __pthread_enable_asynccancel; pushl %eax; pushl %esi; .LPUSHSI2: pushl %ebx; .LPUSHBX2: movl 28(%esp), %esi; movl 28 -4(%esp), %edx; movl 28 -4 -4(%esp), %ecx; movl 28 -4 -4 -4(%esp), %ebx; movl $4, %eax; int $0x80 ; popl %ebx; .LPOPBX2:; popl %esi; .LPOPSI2:; xchgl (%esp), %eax; call __pthread_disable_asynccancel; popl %eax; cmpl $-4095, %eax; jae 0f; .Lpseudo_end:
ret
0:pushl %ebx; .section .gnu.linkonce.t.__i686.get_pc_thunk.bx,"ax",@progbits; .globl __i686.get_pc_thunk.bx; .hidden __i686.get_pc_thunk.bx; .type __i686.get_pc_thunk.bx,@function; __i686.get_pc_thunk.bx: movl (%esp), %ebx; ret; .size __i686.get_pc_thunk.bx, . - __i686.get_pc_thunk.bx; .previous; call __i686.get_pc_thunk.bx; addl $_GLOBAL_OFFSET_TABLE_, %ebx; xorl %edx, %edx; subl %eax, %edx; pushl %edx; ; call __errno_location@PLT; ; popl %ecx; popl %ebx; movl %ecx, (%eax); orl $-1, %eax; jmp .Lpseudo_end; .size __libc_write,.-__libc_write;

.weak __write ; __write = __libc_write

.weak write ; write = __libc_write

/* 5 */

.text; .globl __libc_write; .type __libc_write,@function; .align 1<<4; __libc_write: .section .gnu.linkonce.t.__i686.get_pc_thunk.cx,"ax",@progbits; .globl __i686.get_pc_thunk.cx; .hidden __i686.get_pc_thunk.cx; .type __i686.get_pc_thunk.cx,@function; __i686.get_pc_thunk.cx: movl (%esp), %ecx; ret; .size __i686.get_pc_thunk.cx, . - __i686.get_pc_thunk.cx; .previous; call __i686.get_pc_thunk.cx; addl $_GLOBAL_OFFSET_TABLE_, %ecx; cmpl $0, __pthread_multiple_threads@GOTOFF(%ecx); jne .Lpseudo_cancel; pushl %edi; .LPUSHDI1: pushl %esi; .LPUSHSI1: pushl %ebx; .LPUSHBX1: movl 32(%esp), %edi; movl 32 -4(%esp), %esi; movl 32 -4 -4(%esp), %edx; movl 32 -4 -4 -4(%esp), %ecx; movl 32 -4 -4 -4 -4(%esp), %ebx; movl $4, %eax; int $0x80 ; popl %ebx; .LPOPBX1:; popl %esi; .LPOPSI1:; popl %edi; .LPOPDI1:; cmpl $-4095, %eax; jae 0f; ret; .Lpseudo_cancel: call __pthread_enable_asynccancel; pushl %eax; pushl %edi; .LPUSHDI2: pushl %esi; .LPUSHSI2: pushl %ebx; .LPUSHBX2: movl 36(%esp), %edi; movl 36 -4(%esp), %esi; movl 36 -4 -4(%esp), %edx; movl 36 -4 -4 -4(%esp), %ecx; movl 36 -4 -4 -4 -4(%esp), %ebx; movl $4, %eax; int $0x80 ; popl %ebx; .LPOPBX2:; popl %esi; .LPOPSI2:; popl %edi; .LPOPDI2:; xchgl (%esp), %eax; call __pthread_disable_asynccancel; popl %eax; cmpl $-4095, %eax; jae 0f; .Lpseudo_end:
ret
0:pushl %ebx; .section .gnu.linkonce.t.__i686.get_pc_thunk.bx,"ax",@progbits; .globl __i686.get_pc_thunk.bx; .hidden __i686.get_pc_thunk.bx; .type __i686.get_pc_thunk.bx,@function; __i686.get_pc_thunk.bx: movl (%esp), %ebx; ret; .size __i686.get_pc_thunk.bx, . - __i686.get_pc_thunk.bx; .previous; call __i686.get_pc_thunk.bx; addl $_GLOBAL_OFFSET_TABLE_, %ebx; xorl %edx, %edx; subl %eax, %edx; pushl %edx; ; call __errno_location@PLT; ; popl %ecx; popl %ebx; movl %ecx, (%eax); orl $-1, %eax; jmp .Lpseudo_end; .size __libc_write,.-__libc_write;

.weak __write ; __write = __libc_write

.weak write ; write = __libc_write

/* 6 */

.text; .globl __libc_write; .type __libc_write,@function; .align 1<<4; __libc_write: .section .gnu.linkonce.t.__i686.get_pc_thunk.cx,"ax",@progbits; .globl __i686.get_pc_thunk.cx; .hidden __i686.get_pc_thunk.cx; .type __i686.get_pc_thunk.cx,@function; __i686.get_pc_thunk.cx: movl (%esp), %ecx; ret; .size __i686.get_pc_thunk.cx, . - __i686.get_pc_thunk.cx; .previous; call __i686.get_pc_thunk.cx; addl $_GLOBAL_OFFSET_TABLE_, %ecx; cmpl $0, __pthread_multiple_threads@GOTOFF(%ecx); jne .Lpseudo_cancel; pushl %ebp; .LPUSHBP1: pushl %edi; .LPUSHDI1: pushl %esi; .LPUSHSI1: pushl %ebx; .LPUSHBX1: movl 36(%esp), %ebp; movl 36 -4(%esp), %edi; movl 36 -4 -4(%esp), %esi; movl 36 -4 -4 -4(%esp), %edx; movl 36 -4 -4 -4 -4(%esp), %ecx; movl 36 -4 -4 -4 -4 -4(%esp), %ebx; movl $4, %eax; int $0x80 ; popl %ebx; .LPOPBX1:; popl %esi; .LPOPSI1:; popl %edi; .LPOPDI1:; popl %ebp; .LPOPBP1:; cmpl $-4095, %eax; jae 0f; ret; .Lpseudo_cancel: call __pthread_enable_asynccancel; SAVE_OLDTYPE_6 PUSHCARGS_6 DOCARGS_6 movl $4, %eax; int $0x80 POPCARGS_6; POPSTATE_6 cmpl $-4095, %eax; jae 0f; .Lpseudo_end:
ret
0:pushl %ebx; .section .gnu.linkonce.t.__i686.get_pc_thunk.bx,"ax",@progbits; .globl __i686.get_pc_thunk.bx; .hidden __i686.get_pc_thunk.bx; .type __i686.get_pc_thunk.bx,@function; __i686.get_pc_thunk.bx: movl (%esp), %ebx; ret; .size __i686.get_pc_thunk.bx, . - __i686.get_pc_thunk.bx; .previous; call __i686.get_pc_thunk.bx; addl $_GLOBAL_OFFSET_TABLE_, %ebx; xorl %edx, %edx; subl %eax, %edx; pushl %edx; ; call __errno_location@PLT; ; popl %ecx; popl %ebx; movl %ecx, (%eax); orl $-1, %eax; jmp .Lpseudo_end; .size __libc_write,.-__libc_write;

.weak __write ; __write = __libc_write

.weak write ; write = __libc_write

/* Liste */

accept
close
connect
fcntl
fsync
llseek
lseek
msgrcv
msgsnd
msync
nanosleep
open64
open
pause
pread64
pread
pwrite64
pwrite
read
recvfrom
recvmsg
recv
sendmsg
send
sendto
sigwait
tcdrain
wait
waitpid
write

#endif

/* 1 */

#ifdef MA__LIBPTHREAD
#  include <asm/unistd.h>
#endif

/* GCC may define `__i686' as a macro, which interferes with its use in
   assembly code.  See http://sourceware.org/bugzilla/show_bug.cgi?id=4507
   for details.  */
#if (defined __ASSEMBLER__ ) && (defined __i686)
# undef __i686
#endif

#define SYSCALL1(name) \
    .text;\
    .globl __libc_##name;\
    .type __libc_##name,@function;\
    .align 1<<4;\
__libc_##name:\
    .section .gnu.linkonce.t.__i686.get_pc_thunk.cx,"ax",@progbits;\
    .globl __i686.get_pc_thunk.cx;\
    .hidden __i686.get_pc_thunk.cx;\
    .type __i686.get_pc_thunk.cx,@function;\
__i686.get_pc_thunk.cx:\
        movl (%esp), %ecx;\
        ret;\
    .size __i686.get_pc_thunk.cx, . - __i686.get_pc_thunk.cx;\
    .previous;\
        call __i686.get_pc_thunk.cx;\
        addl $_GLOBAL_OFFSET_TABLE_, %ecx;\
        cmpl $0, __pthread_multiple_threads@GOTOFF(%ecx);\
        jne .Lpseudo_cancel;\
        movl %ebx, %edx;\
.LSAVEBX1:\
        movl 4(%esp), %ebx;\
        movl $__NR_##name, %eax;\
        int $0x80 ;\
        movl %edx, %ebx;\
.LRESTBX1:;\
        cmpl $-4095, %eax;\
        jae 0f;\
        ret;\
.Lpseudo_cancel:\
        call __pthread_enable_asynccancel;\
        movl %eax, %ecx;\
        movl %ebx, %edx;\
        movl 4(%esp), %ebx;\
        movl $__NR_##name, %eax;\
        int $0x80 ;\
        movl %edx, %ebx;\
        pushl %eax;\
        movl %ecx, %eax;\
        call __pthread_disable_asynccancel;\
        popl %eax;\
        cmpl $-4095, %eax;\
        jae 0f;\
.Lpseudo_end: \
        ret ;\
0:\
        pushl %ebx;\
    .section .gnu.linkonce.t.__i686.get_pc_thunk.bx,"ax",@progbits;\
    .globl __i686.get_pc_thunk.bx;\
    .hidden __i686.get_pc_thunk.bx;\
    .type __i686.get_pc_thunk.bx,@function;\
__i686.get_pc_thunk.bx:\
        movl (%esp), %ebx;\
        ret;\
    .size __i686.get_pc_thunk.bx, . - __i686.get_pc_thunk.bx;\
    .previous;\
        call __i686.get_pc_thunk.bx;\
        addl $_GLOBAL_OFFSET_TABLE_, %ebx;\
        xorl %edx, %edx;\
        subl %eax, %edx;\
        pushl %edx;\
        ;\
        call __errno_location@PLT;\
        ;\
        popl %ecx;\
        popl %ebx;\
        movl %ecx, (%eax);\
        orl $-1, %eax;\
        jmp .Lpseudo_end;\
    .size __libc_##name,.-__libc_##name; \
\
.weak __##name ; __##name = __libc_##name; \
\
.weak name ; name = __libc_##name

/* 3 */

#define SYSCALL3(name)  SYSCALL3_ext(name, __NR_##name)
#define SYSCALL3_ext(name,nr) \
    .text; \
    .globl __libc_##name; \
    .type __libc_##name,@function; \
    .align 1<<4; \
__libc_##name: \
    .section .gnu.linkonce.t.__i686.get_pc_thunk.cx,"ax",@progbits;\
    .globl __i686.get_pc_thunk.cx;\
    .hidden __i686.get_pc_thunk.cx;\
    .type __i686.get_pc_thunk.cx,@function;\
__i686.get_pc_thunk.cx:\
        movl (%esp), %ecx;\
        ret;\
    .size __i686.get_pc_thunk.cx, . - __i686.get_pc_thunk.cx;\
    .previous;\
        call __i686.get_pc_thunk.cx;\
        addl $_GLOBAL_OFFSET_TABLE_, %ecx;\
        cmpl $0, __pthread_multiple_threads@GOTOFF(%ecx);\
        jne .Lpseudo_cancel;\
        pushl %ebx;\
.LPUSHBX1:\
        movl 16(%esp), %edx;\
        movl 16 -4(%esp), %ecx;\
        movl 16 -4 -4(%esp), %ebx;\
        movl $nr, %eax;\
        int $0x80 ;\
        popl %ebx;\
.LPOPBX1:;\
        cmpl $-4095, %eax;\
        jae 0f;\
        ret;\
.Lpseudo_cancel:\
        call __pthread_enable_asynccancel;\
        pushl %eax;\
        pushl %ebx;\
.LPUSHBX2:\
        movl 20(%esp), %edx;\
        movl 20 -4(%esp), %ecx;\
        movl 20 -4 -4(%esp), %ebx;\
        movl $nr, %eax;\
        int $0x80 ;\
        popl %ebx;\
.LPOPBX2:;\
        xchgl (%esp), %eax;\
        call __pthread_disable_asynccancel;\
        popl %eax;\
        cmpl $-4095, %eax;\
        jae 0f;\
.Lpseudo_end: \
        ret;\
0:\
        pushl %ebx;\
    .section .gnu.linkonce.t.__i686.get_pc_thunk.bx,"ax",@progbits;\
    .globl __i686.get_pc_thunk.bx;\
    .hidden __i686.get_pc_thunk.bx;\
    .type __i686.get_pc_thunk.bx,@function;\
__i686.get_pc_thunk.bx:\
        movl (%esp), %ebx;\
        ret;\
    .size __i686.get_pc_thunk.bx, . - __i686.get_pc_thunk.bx;\
    .previous;\
        call __i686.get_pc_thunk.bx;\
        addl $_GLOBAL_OFFSET_TABLE_, %ebx;\
        xorl %edx, %edx;\
        subl %eax, %edx;\
        pushl %edx;\
        ;\
        call __errno_location@PLT;\
        ;\
        popl %ecx;\
        popl %ebx;\
        movl %ecx, (%eax);\
        orl $-1, %eax;\
        jmp .Lpseudo_end;\
    .size __libc_##name,.-__libc_##name;\
 \
\
.weak __##name ; __##name = __libc_##name; \
\
.weak name ; name = __libc_##name


