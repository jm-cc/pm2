/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 the PM2 team (see AUTHORS file)
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
 *
 *
 * Use syscall method instead of libc functions
 *
 */


#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include "marcel.h"


#ifdef MA__LIBPTHREAD


int ma_open(const char *path, int oflag, ...)
{
	va_list _args;
	mode_t _mode;
	int _ret;

	if (oflag & O_CREAT) {
		va_start(_args, oflag);
		_mode = va_arg(_args, mode_t);
		_ret = syscall(SYS_open, path, oflag, _mode);
		va_end(_args);
	} else
		_ret = syscall(SYS_open, path, oflag);

	return _ret;
}

int ma_close(int fildes)
{
	return syscall(SYS_close, fildes);
}

ssize_t ma_read(int fd, void *buf, size_t count)
{
	return syscall(SYS_read, fd, buf, count);
}

ssize_t ma_write(int fd, const void *buf, size_t count)
{
	return syscall(SYS_write, fd, buf, count);
}

int ma_nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
	return syscall(SYS_nanosleep, rqtp, rmtp);
}


#endif				/* MA__LIBPTHREAD */
