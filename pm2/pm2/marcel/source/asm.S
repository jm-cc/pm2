
#ifdef X86_ARCH

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

#endif

#if defined(UNICOS_SYS) && defined(ALPHA_ARCH)
        .ident asm_asm ; debut du module
 
        .psect get_sp@code,code,cache
get_sp::
        stq r30, 0(r16)
        ret r31, (r26)
        .endp
 
 
        .psect set_sp@code,code,cache
set_sp::
        addq r16,r31,r30
        ret r31, (r26)
 
        .endp    ;fin du module
 
        .psect set_fp@code,code,cache
set_fp::
        addq r16,r31,r15
        ret r31, (r26)
 
        .endp    ;fin du module
 
 
        .psect get_fp@code,code,cache
get_fp::
        stq r15, 0(r16)
        ret r31, (r26)
        .endp
 
        .end
#endif


#ifdef RS6K_ARCH

/* long get_sp() */
	.globl get_sp
	.globl .get_sp
get_sp:
.get_sp:
	mr 3, 1
	br

/* void set_sp(long) */
	.globl set_sp
	.globl .set_sp
set_sp:
.set_sp:
	mr 1, 3
	br

#endif

	
#ifdef PPC_ARCH

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
#endif


#ifdef SPARC_ARCH

#include <sys/trap.h>

	.seg	"text"

	.global	call_ST_FLUSH_WINDOWS
call_ST_FLUSH_WINDOWS:
	retl
	ta	ST_FLUSH_WINDOWS

#endif


#if defined(ALPHA_ARCH) && defined(OSF_SYS)

	.text
	.align 4
	.file 2 "asm.S"

	.globl set_sp

!       void set_sp(long *new_sp)
!
!       sets the sp
        .ent set_sp
set_sp:
	addq $16,$31, $sp
	ret $31, ($26), 1
	.end set_sp

	.globl get_sp

!       void get_sp(long *current_sp)
!
!       gets the sp
        .ent get_sp
get_sp:
	stq $sp, 0($16)
	ret $31, ($26), 1
	.end get_sp

	.globl setjmp

!       int setjmp(jmp_buf b)
!
        .ent setjmp
setjmp:
	stq $1,  0($16)		/* Save registers. */
	stq $2,  8($16)
	stq $3, 16($16)
	stq $4, 24($16)
	stq $5, 32($16)
	stq $6, 40($16)
	stq $7, 48($16)
	stq $8, 56($16)
	stq $9, 64($16)
	stq $10,72($16)
	stq $11,80($16)
	stq $12,88($16)
	stq $13,96($16)
	stq $14,104($16)
	stq $15,112($16)
	stq $17,120($16)
	stq $18,128($16)
	stq $19,136($16)
	stq $20,144($16)
	stq $21,152($16)
	stq $22,160($16)
	stq $23,168($16)
	stq $24,176($16)
	stq $25,184($16)
	stq $26,192($16)
	stq $27,200($16)
	stq $29,208($16)
	stq $30,216($16)
	stq $31,224($16)
	ldil	$0, 0
	ret $31, ($26), 1
	.end setjmp

	.globl longjmp

!       int longjmp(jmp_buf b, )
!
        .ent longjmp
longjmp:
	mov $17, $0
	ldq $1,  0($16)		/* Load registers. */
	ldq $2,  8($16)
	ldq $3, 16($16)
	ldq $4, 24($16)
	ldq $5, 32($16)
	ldq $6, 40($16)
	ldq $7, 48($16)
	ldq $8, 56($16)
	ldq $9, 64($16)
	ldq $10,72($16)
	ldq $11,80($16)
	ldq $12,88($16)
	ldq $13,96($16)
	ldq $14,104($16)
	ldq $15,112($16)
	ldq $17,120($16)
	ldq $18,128($16)
	ldq $19,136($16)
	ldq $20,144($16)
	ldq $21,152($16)
	ldq $22,160($16)
	ldq $23,168($16)
	ldq $24,176($16)
	ldq $25,184($16)
	ldq $26,192($16)
	ldq $27,200($16)
	ldq $29,208($16)
	ldq $30,216($16)
	ldq $31,224($16)
	ret $31, ($26), 1
	.end longjmp

#endif

