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


#ifndef __MARCEL_IO_H__
#define __MARCEL_IO_H__


#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/uio.h>
#include "tbx_compiler.h"
#include "marcel_config.h"


/** Public functions **/
DEC_MARCEL(ssize_t, read, (int fildes, void *buf, size_t nbytes));
DEC_MARCEL(ssize_t, readv, (int fildes, const struct iovec *iov, int iovcnt));
DEC_MARCEL(ssize_t, pread,(int fd, void *buf, size_t count, off_t pos));

DEC_MARCEL(ssize_t, write, (int fildes, const void *buf, size_t nbytes));
DEC_MARCEL(ssize_t, writev, (int fildes, const struct iovec *iov, int iovcnt));
DEC_MARCEL(ssize_t, pwrite, (int fd, const void *buf, size_t count, off_t pos));

DEC_MARCEL(int, fsync, (int fd));
DEC_MARCEL(int, tcdrain, (int fd));

DEC_MARCEL(int, connect, (int sockfd, const struct sockaddr * serv_addr, socklen_t addrlen));
DEC_MARCEL(int, accept, (int sockfd, struct sockaddr * addr, socklen_t * addrlen));

DEC_MARCEL(ssize_t, sendto, (int s, const void *buf, size_t len, int flags, const struct sockaddr * to, socklen_t tolen));
DEC_MARCEL(ssize_t, send, (int s, const void *buf, size_t len, int flags));
DEC_MARCEL(ssize_t, sendmsg, (int socket, const struct msghdr *message, int flags));

DEC_MARCEL(ssize_t, recvfrom, (int s, void *buf, size_t len, int flags, struct sockaddr * from, socklen_t * fromlen));
DEC_MARCEL(ssize_t, recv, (int s, void *buf, size_t len, int flags));
DEC_MARCEL(ssize_t, recvmsg, (int socket, struct msghdr * message, int flags));


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal marcel functions **/
void ma_io_init(void);


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_IO_H__ **/
