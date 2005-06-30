
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

	.text
	.align 4
	
#if defined(FREEBSD_SYS) || defined(WIN_SYS)
	.globl _my_setjmp
_my_setjmp:
#else
	.globl my_setjmp
my_setjmp:
#endif
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


