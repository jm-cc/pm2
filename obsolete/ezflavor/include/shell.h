
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

#ifndef SHELL_IS_DEF
#define SHELL_IS_DEF

#include <unistd.h>
#include <stdlib.h>


// These three functions return the pid of the child process
pid_t exec_single_cmd(int *output_fd, char *argv[]);

pid_t exec_single_cmd_args(int *output_fd, char *cmd, char *arg, ...);

pid_t exec_single_cmd_fmt(int *output_fd, char *fmt, ...);

int exec_wait(pid_t pid);

#endif
