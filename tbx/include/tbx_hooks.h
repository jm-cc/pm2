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


#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>


#ifndef __TBX_HOOKS_H__
#define __TBX_HOOKS_H__


#ifdef TBX_C

int (*tbx_open) (const char *pathname, int flags, ...) = open;
int (*tbx_close) (int fd) = close;
ssize_t(*tbx_read) (int fd, void *buf, size_t count) = read;
ssize_t(*tbx_write) (int fd, const void *buf, size_t count) = write;
int (*tbx_nanosleep) (const struct timespec * req, struct timespec * rem) = nanosleep;

#else

extern int (*tbx_open) (const char *pathname, int flags, ...);
extern int (*tbx_close) (int fd);
extern ssize_t(*tbx_read) (int fd, void *buf, size_t count);
extern ssize_t(*tbx_write) (int fd, const void *buf, size_t count);
extern int (*tbx_nanosleep) (const struct timespec * req, struct timespec * rem);

#endif				/* TBX_C */


#endif /** __TBX_HOOKS_H__ **/
