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
#include "tbx_compiler.h"
#include "sys/marcel_flags.h"
#include <sys/types.h>
#include <sys/uio.h>


/** Public functions **/
int marcel_read(int fildes, void *buf, size_t nbytes);
int marcel_write(int fildes, const void *buf, size_t nbytes);
int marcel_select(int nfds, fd_set * __restrict rfds, fd_set * __restrict wfds);
int marcel_readv(int fildes, const struct iovec *iov, int iovcnt);
int marcel_writev(int fildes, const struct iovec *iov, int iovcnt);

/* To force reading/writing an exact number of bytes */
int marcel_read_exactly(int fildes, void *buf, size_t nbytes);
int marcel_write_exactly(int fildes, const void *buf, size_t nbytes);
int marcel_readv_exactly(int fildes, const struct iovec *iov, int iovcnt);
int marcel_writev_exactly(int fildes, const struct iovec *iov, int iovcnt);

/*  Still here, but do not use it! */
int __tbx_deprecated__ tselect(int width, fd_set * __restrict readfds,
			       fd_set * __restrict writefds,
			       fd_set * __restrict exceptfds);


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal marcel functions **/
void ma_io_init(void);


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_IO_H__ **/
