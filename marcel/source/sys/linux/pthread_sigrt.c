
/* Linuxthreads - a simple clone()-based implementation of Posix        */
/* threads for Linux.                                                   */
/* Copyright (C) 1996 Xavier Leroy (Xavier.Leroy@inria.fr)              */
/*                                                                      */
/* This program is free software; you can redistribute it and/or        */
/* modify it under the terms of the GNU Library General Public License  */
/* as published by the Free Software Foundation; either version 2       */
/* of the License, or (at your option) any later version.               */
/*                                                                      */
/* This program is distributed in the hope that it will be useful,      */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU Library General Public License for more details.                 */


#include "marcel.h"


#ifdef MA__LIBPTHREAD


#ifdef __GLIBC__
/* Return number of available real-time signal with highest priority.  */
int __libc_current_sigrtmin(void)
{
	MARCEL_LOG("SIGRT is not supported in the Marcel software");
	errno = ENOTSUP;
	return -1;
}

/* Return number of available real-time signal with lowest priority.  */
int __libc_current_sigrtmax(void)
{
	MARCEL_LOG("SIGRT is not supported in the Marcel software");
	errno = ENOTSUP;
	return -1;
}

#if !HAVE_DECL___LIBC_ALLOCATE_RTSIG
int __libc_allocate_rtsig(int high TBX_UNUSED);
#endif
int __libc_allocate_rtsig(int high TBX_UNUSED)
{
	MARCEL_LOG("SIGRT is not supported in the Marcel software");
	return -1;
}

extern int __pthread_clock_settime(clockid_t, const struct timespec *);
int __pthread_clock_settime(clockid_t clock_id TBX_UNUSED,
			    const struct timespec *tp TBX_UNUSED)
{
	MARCEL_LOG_IN();
	/* Extension Real-Time non supportée */
	MARCEL_LOG_RETURN(ENOTSUP);
}

extern int __pthread_clock_gettime(clockid_t, struct timespec *);
int __pthread_clock_gettime(clockid_t clock_id TBX_UNUSED, struct timespec *tp TBX_UNUSED)
{
	MARCEL_LOG_IN();
	/* Extension Real-Time non supportée */
	MARCEL_LOG_RETURN(ENOTSUP);
}
#endif


#endif				/* MA__LIBPTHREAD */
