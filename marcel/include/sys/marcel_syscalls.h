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


#include <unistd.h>
#include "sys/marcel_flags.h"
#include "marcel_config.h"


#ifndef __MARCEL_SYSCALLS_H__
#define __MARCEL_SYSCALLS_H__


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


#ifdef MA__LIBPTHREAD
int ma_open(const char *path, int oflag, ...);
int ma_close(int fildes);
ssize_t ma_read(int fd, void *buf, size_t count);
ssize_t ma_write(int fd, const void *buf, size_t count);
int ma_nanosleep(const struct timespec *rqtp, struct timespec *rmtp);
#endif				/* MA__LIBPTHREAD */


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __SYS_MARCEL_SYSCALLS_H__ **/
