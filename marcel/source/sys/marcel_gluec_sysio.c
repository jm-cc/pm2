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
#include "marcel_pmarcel.h"


#define ma_cancellable_call(ver, ret, name, proto, ...)			\
	ret MARCEL_NAME(name) proto {					\
		ret res;						\
		if (tbx_unlikely(!marcel_createdthreads())) {		\
			res = MA_REALSYM(name)(__VA_ARGS__);		\
		} else {						\
			int old = __pmarcel_enable_asynccancel();	\
			marcel_extlib_protect();			\
			res = MA_REALSYM(name)(__VA_ARGS__);		\
			marcel_extlib_unprotect();			\
			__pmarcel_disable_asynccancel(old);		\
		}							\
		return res;						\
	}								\
									\
	DEF_STRONG_ALIAS(MARCEL_NAME(name), LPT_NAME(name))		\
	versioned_symbol(libpthread, LPT_NAME(name), LIBC_NAME(name), ver); \
	DEF___LIBC(ret, name, proto, (__VA_ARGS__))

#define ma_cancellable_syscall(ver, ret, name, proto, ...)		\
	ret MARCEL_NAME(name) proto {					\
		ret res;						\
		if (tbx_unlikely(!marcel_createdthreads())) {		\
			res = syscall(SYS_##name, __VA_ARGS__);		\
		} else {						\
			int old = __pmarcel_enable_asynccancel();	\
			res = syscall(SYS_##name, __VA_ARGS__);		\
			__pmarcel_disable_asynccancel(old);		\
		}							\
		return res;						\
	}								\
									\
	DEF_STRONG_ALIAS(MARCEL_NAME(name), LPT_NAME(name))		\
	versioned_symbol(libpthread, LPT_NAME(name), LIBC_NAME(name), ver); \
	DEF___LIBC(ret, name, proto, (__VA_ARGS__))



ma_cancellable_call(GLIBC_2_0, int, tcdrain, (int fd), fd)

ma_cancellable_call(GLIBC_2_0, int, fsync, (int fd), fd)

ma_cancellable_call(GLIBC_2_0, int, msync, (void *addr, size_t len, int flags), addr, len, flags)

ma_cancellable_call(GLIBC_2_0, int, poll, (struct pollfd *fds, nfds_t nfds, int timeout), fds, nfds, timeout)

ma_cancellable_call(GLIBC_2_0, int, select, (int nfds, fd_set *readfds, fd_set *writefds, 
					     fd_set *exceptfds, struct timeval *timeout), 
		    nfds, readfds, writefds, exceptfds, timeout)

DEC_LPT(int, open, (const char *pathname, int flags, ...));
int marcel_open(const char *pathname, int flags, ...)
{
	va_list args;
	mode_t mode;
	int ret;
	
	if (flags & O_CREAT) {
		va_start(args, flags);
		mode = va_arg(args, mode_t);
		va_end(args);
	} else
		mode = 0; /* mode is ignored by syscall */

	if (tbx_unlikely(!marcel_createdthreads())) {
		ret = syscall(SYS_open, pathname, flags, mode);
	} else {
		int old = __pmarcel_enable_asynccancel();
		ret = syscall(SYS_open, pathname, flags, mode);
		__pmarcel_disable_asynccancel(old);
	}

	return ret;
}
DEF_STRONG_ALIAS(MARCEL_NAME(open), MARCEL_NAME(open64))
DEF_STRONG_ALIAS(MARCEL_NAME(open), LPT_NAME(open64))
DEF_STRONG_ALIAS(MARCEL_NAME(open), LPT_NAME(open))
versioned_symbol(libpthread, MARCEL_NAME(open64), LIBC_NAME(open64), GLIBC_2_2);
versioned_symbol(libpthread, MARCEL_NAME(open), LIBC_NAME(open), GLIBC_2_0);
DEF___LIBC(int, open, (const char *path, int flags, mode_t mode), (path, flags, mode))
DEF___LIBC(int, open64, (const char *path, int flags, mode_t mode), (path, flags, mode))


DEC_LPT(int, creat, (const char *pathname, mode_t mode));
DEF_MARCEL(int, creat, (const char *pathname, mode_t mode), (pathname, mode), 
{
	return marcel_open(pathname, O_CREAT|O_WRONLY|O_TRUNC, mode);
})
DEF_STRONG_ALIAS(MARCEL_NAME(creat), MARCEL_NAME(creat64))
versioned_symbol(libpthread, MARCEL_NAME(creat64), LIBC_NAME(creat64), GLIBC_2_2);
versioned_symbol(libpthread, MARCEL_NAME(creat), LIBC_NAME(creat), GLIBC_2_0);



/** file ops are required during init and thus need to be initialized correctly */
ma_cancellable_syscall(GLIBC_2_0, off_t, lseek, (int fd, off_t offset, int whence), fd, offset, whence)

ma_cancellable_syscall(GLIBC_2_0, int, close, (int fd), fd)

ma_cancellable_syscall(GLIBC_2_0, int, fcntl, (int fd, int cmd, long arg), fd, cmd, arg)
