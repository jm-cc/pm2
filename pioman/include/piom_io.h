
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

#include <unistd.h>
#include <sys/time.h>
#ifdef __MINGW32__
#include <winsock.h>
#else
#include <sys/uio.h>
#endif
#include "tbx_compiler.h"

int piom_read(int fildes, void *buf, size_t nbytes);

int piom_write(int fildes, const void *buf, size_t nbytes);

int piom_select(int nfds, fd_set * __restrict rfds,
		 fd_set * __restrict wfds);

#ifndef __MINGW32__
int piom_readv(int fildes, const struct iovec *iov, int iovcnt);

int piom_writev(int fildes, const struct iovec *iov, int iovcnt);
#endif

/* To force reading/writing an exact number of bytes */

int piom_read_exactly(int fildes, void *buf, size_t nbytes);

int piom_write_exactly(int fildes, const void *buf, size_t nbytes);

#ifndef __MINGW32__
int piom_readv_exactly(int fildes, const struct iovec *iov, int iovcnt);

int piom_writev_exactly(int fildes, const struct iovec *iov, int iovcnt);
#endif

void piom_io_init(void);
void piom_io_stop(void);
