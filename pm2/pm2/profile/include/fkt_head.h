/*	fkt_setup.c - user-level library for calls to fkt kernel routines


	Copyright (C) 2001  Robert D. Russell -- rdr@unh.edu

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


*/

/*	fkt = Fast Kernel Tracing */

#ifndef __FKT_HEAD__
#define __FKT_HEAD__

#include "/usr/src/linux/include/linux/unistd.h"


/*	called once to set up tracing.
	includes mallocing the buffers to hold the trace.
	returns number of 32-bit ints in buffer (>0) if all ok, else -1.
*/
/*	int fkt_setup( unsigned int keymask ) */
	_syscall1(int, fkt_setup, unsigned int, keymask)


/*	called once to end up tracing.
	includes freeing the buffers.
	returns 0 if all ok, -1.
*/
/*	int fkt_endup( void ) */
	_syscall0(int, fkt_endup)


/*	called to enable/disable key sets.
	returns old key set if all ok, else -1.
*/
/*	int fkt_keychange( int how, unsigned int keymask ) */
	_syscall2(int, fkt_keychange, int, how, unsigned int, keymask)


/*	called to reset tracing.
	returns number of 32-bit ints in buffer (>0) if all ok, else -1.
*/
/*	int fkt_reset( unsigned int keymask ) */
	_syscall1(int, fkt_reset, unsigned int, keymask)


/*	called repeatedly to get copy of current buffer.
	returns nints >= 0 if all ok, else -1.
*/
/* int fkt_getcopy( int maxints, unsigned int *buffer ) */
	_syscall2(int, fkt_getcopy, int, maxints, unsigned int *, buffer)


/*	called to trigger a kernel probe with no parameters */
/* int fkt_probe0( unsigned int keymask, unsigned int code ); */
	_syscall2(int, fkt_probe0, unsigned int, keymask, unsigned int, code)


/*	called to trigger a kernel probe with one parameter */
/* int fkt_probe1( unsigned int keymask, unsigned int code,
							unsigned int p1 ); */
	_syscall3(int, fkt_probe1, unsigned int, keymask, unsigned int, code, unsigned int, p1)


/*	called to trigger a kernel probe with two parameters */
/* int fkt_probe2( unsigned int keymask, unsigned int code,
							unsigned int p1, unsigned int p2 ); */
	_syscall4(int, fkt_probe2, unsigned int, keymask, unsigned int, code, unsigned int, p1, unsigned int, p2)


#endif
