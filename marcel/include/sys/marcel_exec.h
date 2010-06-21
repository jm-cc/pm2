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


/* This disables Marcel's timer before calling
 * FUNCTION such that the new program doesn't get SIGALRM signals.  */


#ifndef __MARCEL_EXEC_H__
#define __MARCEL_EXEC_H__


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


#ifdef CONFIG_PUK_PUKABI
int ma_execve(const char *name, char *const argv[], char *const envp[]);
int ma_execv(const char *name, char *const argv[]);
int ma_execvp(const char *name, char *const argv[]);
int ma_system(const char *command);

#ifdef HAVE__POSIX_SPAWN
#include <spawn.h>

int ma_posix_spawn(pid_t * restrict pid, const char *restrict path,
		   const posix_spawn_file_actions_t * file_actions,
		   const posix_spawnattr_t * restrict attrp, char *const argv[restrict],
		   char *const envp[restrict]);
int ma_posix_spawnp(pid_t * restrict pid, const char *restrict file,
		    const posix_spawn_file_actions_t * file_actions,
		    const posix_spawnattr_t * restrict attrp, char *const argv[restrict],
		    char *const envp[restrict]);
#endif
#endif


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_EXEC_H__ **/
