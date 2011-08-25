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
 */


#ifndef __SYS_MARCEL_GLUEC_SYSIO_H__
#define __SYS_MARCEL_GLUEC_SYSIO_H__


#include <unistd.h>
#include <sys/syscall.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <poll.h>
#include <unistd.h>


/** Public functions **/
DEC_MARCEL(off_t, lseek, (int fd, off_t offset, int whence));
DEC_MARCEL(int, close, (int fd));
DEC_MARCEL(int, fcntl, (int fd, int cmd, long arg));
DEC_MARCEL(int, msync, (void *addr, size_t len, int flags));
DEC_MARCEL(int, creat, (const char *pathname, mode_t mode));
DEC_MARCEL(int, open, (const char *path, int flags, ...));
DEC_MARCEL(int, select, (int nfds, fd_set *readfds, fd_set *writefds, 
			 fd_set *exceptfds, struct timeval *timeout));
DEC_MARCEL(int, poll, (struct pollfd *fds, nfds_t nfds, int timeout));


#endif /** __SYS_MARCEL_GLUEC_SYSIO_H__ **/
