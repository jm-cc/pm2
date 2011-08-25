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


#ifndef __SYS_MARCEL_GLUEC_EXEC_H__
#define __SYS_MARCEL_GLUEC_EXEC_H__


#include <sys/types.h>
#include <sys/wait.h>
#include "marcel_config.h"
#include "sys/marcel_alias.h"


/** Public functions **/
DEC_MARCEL(pid_t, wait, (int *status));
DEC_MARCEL(pid_t, waitid, (idtype_t idtype, id_t id, siginfo_t *infop, int options));
DEC_MARCEL(pid_t, waitpid, (pid_t wpid, int *status, int options));
DEC_MARCEL(pid_t, wait3, (int *status, int options, struct rusage *rusage));
DEC_MARCEL(pid_t, wait4, (pid_t wpid, int *status, int options, struct rusage *rusage));
DEC_MARCEL(int, execl, (const char *path, const char *arg, ...));
DEC_MARCEL(int, execlp, (const char *file, const char *arg, ...));
DEC_MARCEL(int, execle, (const char *path, const char *arg, ...));
DEC_MARCEL(int, execve, (const char *name, char *const argv[], char *const envp[]));
DEC_MARCEL(int, execv, (const char *name, char *const argv[]));
DEC_MARCEL(int, execvp, (const char *name, char *const argv[]));
DEC_MARCEL(int, system, (const char *line));
#if HAVE_DECL_POSIX_SPAWN
#include <spawn.h>
DEC_MARCEL(int, posix_spawn, (pid_t * __restrict pid, const char *__restrict path,
			      const posix_spawn_file_actions_t * file_actions,
			      const posix_spawnattr_t * __restrict attrp, char *const argv[],
			      char *const envp[]));
DEC_MARCEL(int, posix_spawnp, (pid_t * __restrict pid, const char *__restrict file,
			       const posix_spawn_file_actions_t * file_actions,
			       const posix_spawnattr_t * __restrict attrp, char *const argv[],
			       char *const envp[]));
#endif


#endif /** __SYS_MARCEL_GLUEC_EXEC_H__ **/
