#	fut_header.S
#
#	fut = Fast User Tracing
#
#	uses 3 externals:
#		unsigned fut_active;			0 when inactive, non-zero when active
#		unsigned *fut_last_slot;		pointer to slot beyond end of buffer
#		unsigned *fut_next_slot;		pointer to next free slot in buffer
#
#
#	uses only registers %eax, %ecx and %edx:
#		use %ecx as pointer to next slot in buffer
#		use %edx as pointer to caller stack and as scratch register
#		use %eax as scratch register
#
#
#	call is:	fut_header( header, p1, p2, ... );
#	where:
#				header low byte has total number of bytes in this probe entry
#							= (3 + number_of_parameters)*4
#				header high 3 bytes have entry identification code
#
#
#	format of one probe entry is:
#				int 1		lo 32-bits of cycle clock
#				int 2		hi 32-bits of cycle clock
#				int 3		full 32-bit header from caller
#				int 4		full 32-bit p1 from caller
#				int 5		full 32-bit p2 from caller
#				...		...
#				int 3+n		full 32-bit pn from caller
#
#
#	assumptions (and invariants):
#		value in fut_next_slot is a multiple of 4
#		value in fut_last_slot is a multiple of 4
#		value in low byte of header is multiple of 4
#
#
#	fut_active contains the currently active set of probe keys.
#	bit x in fut_active is 1 if probes in keyset x are active.
#	fut_active is set  when tracing is actived by fut_setup();  it is reset
#	to 0 when tracing is deactived by fut_endup().
#	fut_active is also reset to 0 by the first thread that fills the buffer.
#

#include <linux/linkage.h>


#	Offsets into caller stack when in these routines
caller_code	= 8

#	Offsets into probe record
probe_lo_cycles	= 0
probe_hi_cycles	= 4
probe_code		= 8
probe_param1	= 12



	.text
	.align	4
	.globl	fut_header
fut_header:							# normal entry point to take a probe
	pushl	%ebx					# save this register
	movzbl	caller_code(%esp),%edx	# get number of bytes needed by this record
	movl	fut_next_slot,%eax		# get pointer to next free slot
1:
	movl	%eax,%ecx				# also need this in ecx
	addl	%edx,%ecx				# add on bytes needed by this record
#ifdef __SMP__
	lock
#endif
	cmpxchg	%ecx,fut_next_slot		# atomically update ptr to next free slot
	jnz		1b						# jump back if did not get it
	cmp		%ecx,fut_last_slot		# are we off the end of the buffer?
	jb		5f						# jump if below (not enough free bytes)
	movl	%ecx,%ebx				# keep pointer to next free slot after this
	movl	%eax,%ecx				# point ecx at first slot of this record
	rdtsc							# get 64-bit cpu cycle counter in edx-eax
	movl	%eax,probe_lo_cycles(%ecx)	# low-order 32 bits from eax to 1st slot
	movl	%edx,probe_hi_cycles(%ecx)	# hi-order 32 bits from edx to 2nd slot
	leal	caller_code(%esp),%edx	# point edx at header value in call
	leal	probe_code(%ecx),%ecx	# point ecx at header slot in record
9:
	movl	(%edx),%eax				# pick up next value from caller
	movl	%eax,(%ecx)				# save that value into next record slot
	leal	4(%edx),%edx			# bump edx to point at next caller value
	leal	4(%ecx),%ecx			# bump ecx to point at next record slot
	cmpl	%ecx,%ebx				# see if record filled yet
	jne		9b						# if not, jump back to next loop iteration
6:
	popl	%ebx					# restore register
	ret								# return to caller

	.align	4
5:									# out of space in the buffer
	movl	$0,fut_active			# turn off all active bits
	movl	%eax,fut_next_slot		# repair damage to space left
	jmp		6b						# leave



	.globl	fut_trailer				# to see size of code generated
fut_trailer:
	.size   fut_header,fut_trailer-fut_header
