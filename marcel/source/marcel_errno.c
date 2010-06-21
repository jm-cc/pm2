
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

#include "marcel.h"
#include <errno.h>

#undef errno
#pragma weak errno
DEF_MARCEL_PMARCEL(int *, __errno_location,(void),(),
{
	int * res;

#ifdef MA__PROVIDE_TLS
	extern __thread int errno;
	res=&errno;
#else
	static int _first_errno;

	if (ma_init_done[MA_INIT_TLS]) {
		res=&SELF_GETMEM(__errno);
	} else {
		res=&_first_errno;
	}
#endif
	return res;
})
#ifdef MA__LIBPTHREAD
versioned_symbol(libpthread, pmarcel___errno_location, __errno_location, GLIBC_2_0);
#endif
DEF___C(int *, __errno_location,(void),())

#undef h_errno
#pragma weak h_errno
DEF_MARCEL_PMARCEL(int *, __h_errno_location,(void),(),
{
	MARCEL_LOG_IN();
	int * res;

#ifdef MA__PROVIDE_TLS
	extern __thread int h_errno;
	res=&h_errno;
#else
	static int _first_h_errno;

	if (ma_init_done[MA_INIT_TLS]) {
		res=&SELF_GETMEM(__h_errno);
	} else {
		res=&_first_h_errno;
	}
#endif
	MARCEL_LOG_RETURN(res);
})
extern int *__h_errno_location(void);
DEF_C(int *, __h_errno_location,(void),())
DEF___C(int *, __h_errno_location,(void),())

