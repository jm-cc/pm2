 




































	.text
	.align 4
	

	.globl my_setjmp
my_setjmp:

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








	








