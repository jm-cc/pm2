
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

#ifndef MARCEL_IO_EST_DEF
#define MARCEL_IO_EST_DEF

#include <unistd.h>
#include <sys/time.h>
#include <sys/uio.h>

void marcel_io_init(void);

int marcel_read(int fildes, void *buf, size_t nbytes);

int marcel_write(int fildes, const void *buf, size_t nbytes);

int marcel_select(int nfds, fd_set *rfds, fd_set *wfds);

int marcel_readv(int fildes, const struct iovec *iov, int iovcnt);

int marcel_writev(int fildes, const struct iovec *iov, int iovcnt);

/* To force reading/writing an exact number of bytes */

int marcel_read_exactly(int fildes, void *buf, size_t nbytes);

int marcel_write_exactly(int fildes, const void *buf, size_t nbytes);

int marcel_readv_exactly(int fildes, const struct iovec *iov, int iovcnt);

int marcel_writev_exactly(int fildes, const struct iovec *iov, int iovcnt);

#endif
