
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

#if defined(LINUX_SYS)

/* long get_sp() */
	.globl get_sp
	.globl .get_sp
get_sp:
.get_sp:
	mr 3, 1
	blr

/* void set_sp(long) */
	.globl set_sp
	.globl .set_sp
set_sp:
.set_sp:
	mr 1, 3
	blr

/* long get_fp() */
	.globl get_fp
	.globl .get_fp
get_fp:
.get_fp:
	mr 3, 31
	blr

/* void set_fp(long) */
	.globl set_fp
	.globl .set_fp
set_fp:
.set_fp:
	mr 31, 3
	blr
#endif

#if defined(DARWIN_SYS)

/* long get_sp() */
	.globl _get_sp
	.globl .get_sp
_get_sp:
.get_sp:
	mr r3, r1
	blr

/* void set_sp(long) */
	.globl _set_sp
	.globl .set_sp
_set_sp:
.set_sp:
	mr r1, r3
	blr

/* long get_fp() */
	.globl get_fp
	.globl .get_fp
get_fp:
.get_fp:
	mr r3, r31
	blr

/* void set_fp(long) */
	.globl set_fp
	.globl .set_fp
set_fp:
.set_fp:
	mr r31, r3
	blr

#endif

/* void setjmp(jmp_buf) */
/*
	.globl setjmp
	.globl .setjmp
setjmp:
.setjmp:
	mflr 0
	stw 0,0(3)
	stw 1,4(3)
	stw 2,8(3)
	stw 3,12(3)
	stw 4,16(3)
	stw 5,20(3)
	stw 6,24(3)
	stw 7,28(3)
	stw 8,32(3)
	stw 9,36(3)
	stw 10,40(3)
	stw 11,44(3)
	stw 12,48(3)
	stw 13,52(3)
	stw 14,56(3)
	stw 15,60(3)
	stw 16,64(3)
	stw 17,68(3)
	stw 18,72(3)
	stw 19,76(3)
	stw 20,80(3)
	stw 21,84(3)
	stw 22,88(3)
	stw 23,92(3)
	stw 24,96(3)
	stw 25,100(3)
	stw 26,104(3)
	stw 27,108(3)
	stw 28,112(3)
	stw 29,116(3)
	stw 30,120(3)
	stw 31,124(3)
	li 3,0
	blr
*/
/* void longjmp(jmp_buf, int) */
/*
	.globl longjmp
	.globl .longjmp
longjmp:
.longjmp:
	lwz 0,0(3)
	lwz 1,4(3)
	lwz 2,8(3)
	lwz 5,20(3)
	lwz 6,24(3)
	lwz 7,28(3)
	lwz 8,32(3)
	lwz 9,36(3)
	lwz 10,40(3)
	lwz 11,44(3)
	lwz 12,48(3)
	lwz 13,52(3)
	lwz 14,56(3)
	lwz 15,60(3)
	lwz 16,64(3)
	lwz 17,68(3)
	lwz 18,72(3)
	lwz 19,76(3)
	lwz 20,80(3)
	lwz 21,84(3)
	lwz 22,88(3)
	lwz 23,92(3)
	lwz 24,96(3)
	lwz 25,100(3)
	lwz 26,104(3)
	lwz 27,108(3)
	lwz 28,112(3)
	lwz 29,116(3)
	lwz 30,120(3)
	lwz 31,124(3)
	mr 3,4
	mtlr 0
	blr
*/

