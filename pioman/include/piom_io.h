/* -*- Mode: C; c-basic-offset:4 ; -*- */
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

#ifndef PIOM_IO_H
#define PIOM_IO_H

#include <unistd.h>
#include <sys/time.h>
#ifdef __MINGW32__
#include <winsock.h>
#else
#include <sys/uio.h>
#endif
#include "tbx_compiler.h"

#define piom_read piom_task_read
#define piom_readv piom_task_readv
#define piom_write piom_task_write
#define piom_writev piom_task_writev
#define piom_select piom_task_select
#define piom_read_exactly piom_task_read_exactly
#define piom_readv_exactly piom_task_readv_exactly
#define piom_write_exactly piom_task_write_exactly
#define piom_writev_exactly piom_task_writev_exactly


void piom_io_init(void);
void piom_io_stop(void);

#endif /* PIOM_IO_H */
