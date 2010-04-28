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

#ifndef PIOM_IO_TASK_H
#define PIOM_IO_TASK_H

int piom_task_read(int fildes, void *buf, size_t nbytes);

int piom_task_write(int fildes, const void *buf, size_t nbytes);
int piom_task_select(int nfds, fd_set * __restrict rfds,
		     fd_set * __restrict wfds);

void piom_io_task_init(void);
void piom_io_task_stop(void);

#endif /* PIOM_IO_TASK_H */
