	.file	"dsm.c"
	.version	"01.01"
gcc2_compiled.:
.section	.rodata
	.align 32
.LC7:
	.string	"/home/bouge/Recherche/PM2/pm2/modules/dsm/include/dsm_lock.h"
/APP
	.data
	.align 4096
/NO_APP
.globl dsm_data_begin
.data
	.type	 dsm_data_begin,@object
	.size	 dsm_data_begin,1
dsm_data_begin:
.globl shvar
	.align 4
	.type	 shvar,@object
	.size	 shvar,4
shvar:
	.long 0
.globl dsm_data_end
	.type	 dsm_data_end,@object
	.size	 dsm_data_end,1
dsm_data_end:
/APP
	.align 4096
.section	.rodata
.LC11:
	.string	"Thread %d on node %d\n"
	.align 32
.LC12:
	.string	"Thread %d from node %d finished on node %d: from %d to %d!\n"
.LC13:
	.string	"shvar=%d\n"
/NO_APP
.text
	.align 4
.globl marcel_main
	.type	 marcel_main,@function
marcel_main:
	pushl %ebp
	movl %esp,%ebp
	subl $48,%esp
	pushl %esi
	pushl %ebx
	movl 12(%ebp),%ebx
	movl $0,-36(%ebp)
	addl $-8,%esp
	pushl $service
	pushl $service_id
	call pm2_rawrpc_register
	addl $-12,%esp
	pushl $0
	call dsm_set_default_protocol
	addl $32,%esp
	addl $-8,%esp
	pushl %ebx
	leal 8(%ebp),%eax
	pushl %eax
	call common_init
	addl $16,%esp
	cmpl $0,__pm2_self
	jne .L133
	movl $1,%esi
	cmpl __pm2_conf_size,%esi
	jae .L141
	leal -32(%ebp),%ebx
	.align 4
.L138:
	addl $-4,%esp
	pushl $0
	pushl $0
	pushl %ebx
	call pm2_completion_init
	incl -36(%ebp)
	addl $-4,%esp
	pushl $0
	pushl service_id
	pushl %esi
	call pm2_rawrpc_begin
	addl $32,%esp
	pushl $1
	leal -36(%ebp),%eax
	pushl %eax
	pushl $1
	pushl $2
	call pm2_pack_int
	addl $-4,%esp
	pushl %ebx
	pushl $1
	pushl $2
	call pm2_pack_completion
	addl $32,%esp
	call pm2_rawrpc_end
	addl $-12,%esp
	pushl %ebx
	call pm2_completion_wait
	addl $16,%esp
	incl %esi
	cmpl __pm2_conf_size,%esi
	jb .L138
.L141:
	addl $-8,%esp
	pushl shvar
	pushl $.LC13
	call marcel_printf
	call pm2_halt
	addl $16,%esp
.L133:
	call pm2_exit
	xorl %eax,%eax
	leal -56(%ebp),%esp
	popl %ebx
	popl %esi
	leave
	ret
.Lfe1:
	.size	 marcel_main,.Lfe1-marcel_main
	.comm	LRPC_CONSOLE_THREADS,4,4
	.comm	LRPC_CONSOLE_PING,4,4
	.comm	LRPC_CONSOLE_USER,4,4
	.comm	LRPC_CONSOLE_MIGRATE,4,4
	.comm	LRPC_CONSOLE_FREEZE,4,4
	.comm	DSM_LRPC_READ_PAGE_REQ,4,4
	.comm	DSM_LRPC_WRITE_PAGE_REQ,4,4
	.comm	DSM_LRPC_SEND_PAGE,4,4
	.comm	DSM_LRPC_INVALIDATE_REQ,4,4
	.comm	DSM_LRPC_INVALIDATE_ACK,4,4
	.comm	DSM_LRPC_SEND_DIFFS,4,4
	.comm	DSM_LRPC_LOCK,4,4
	.comm	DSM_LRPC_UNLOCK,4,4
	.comm	DSM_LRPC_MULTIPLE_READ_PAGE_REQ,4,4
	.comm	DSM_LRPC_MULTIPLE_WRITE_PAGE_REQ,4,4
	.comm	DSM_LRPC_SEND_MULTIPLE_PAGES_READ,4,4
	.comm	DSM_LRPC_SEND_MULTIPLE_PAGES_WRITE,4,4
	.comm	DSM_LRPC_SEND_MULTIPLE_DIFFS,4,4
	.comm	DSM_LRPC_HBRC_DIFFS,4,4
	.comm	service_id,4,4
	.align 4
.globl f
	.type	 f,@function
f:
	subl $48,%esp
	pushl %edi
	pushl %esi
	pushl %ebx
	movl __pm2_self,%esi
	pushl $1
	leal 16(%esp),%eax
	pushl %eax
	pushl $1
	pushl $2
	call pm2_unpack_int
	addl $-4,%esp
	leal 36(%esp),%ebx
	pushl %ebx
	pushl $1
	pushl $2
	call pm2_unpack_completion
	addl $32,%esp
	call mad_recvbuf_receive
	addl $-12,%esp
	movl _pm2_mad_key,%edx
/APP
	movl %esp, %eax
/NO_APP
	andb $112,%al
	orl $65392,%eax
	pushl (%eax,%edx,4)
	call marcel_sem_V
	addl $-4,%esp
	pushl %esi
	pushl 36(%esp)
	pushl $.LC11
	call marcel_printf
	movl shvar,%edx
	addl $32,%esp
	movl $19,%ecx
	.align 4
.L128:
	movl shvar,%eax
	leal 1(%eax),%edi
	movl %edi,shvar
	decl %ecx
	jns .L128
	movl %edi,%eax
	addl $-4,%esp
	pushl %eax
	pushl %edx
	pushl __pm2_self
	pushl %esi
	pushl 32(%esp)
	pushl $.LC12
	pushl $__iob+32
	call marcel_fprintf
	addl $32,%esp
	addl $-12,%esp
	pushl %ebx
	call pm2_completion_signal
	addl $16,%esp
	popl %ebx
	popl %esi
	popl %edi
	addl $48,%esp
	ret
.Lfe2:
	.size	 f,.Lfe2-f
	.align 4
	.type	 service,@function
service:
	subl $12,%esp
	addl $-8,%esp
	pushl $0
	pushl $f
	call pm2_thread_create
	addl $16,%esp
	addl $12,%esp
	ret
.Lfe3:
	.size	 service,.Lfe3-service
	.ident	"GCC: (GNU) 2.95.2 19991024 (release)"
