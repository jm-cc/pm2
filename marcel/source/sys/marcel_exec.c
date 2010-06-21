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


#include "marcel.h"


#ifdef CONFIG_PUK_PUKABI


int ma_execve(const char *name, char *const argv[], char *const envp[])
{
	int ret;

	marcel_sig_stop_itimer();
	ret = PUK_ABI_REAL(execve) (name, argv, envp);
	marcel_sig_reset_timer();

	return ret;
}

int ma_execv(const char *name, char *const argv[])
{
	int ret;

	marcel_sig_stop_itimer();
	ret = PUK_ABI_REAL(execv) (name, argv);
	marcel_sig_reset_timer();

	return ret;
}

int ma_execvp(const char *name, char *const argv[])
{
	int ret;

	marcel_sig_stop_itimer();
	ret = PUK_ABI_REAL(execvp) (name, argv);
	marcel_sig_reset_timer();

	return ret;
}

int ma_system(const char *command)
{
	int ret;

	marcel_sig_stop_itimer();
	ret = PUK_ABI_REAL(system) (command);
	marcel_sig_reset_timer();

	return ret;
}

#ifdef HAVE__POSIX_SPAWN
int ma_posix_spawn(pid_t * restrict pid, const char *restrict path,
		   const posix_spawn_file_actions_t * file_actions,
		   const posix_spawnattr_t * restrict attrp, char *const argv[restrict],
		   char *const envp[restrict])
{
	int ret;

	marcel_sig_stop_itimer();
	ret = PUK_ABI_REAL(posix_spawn) (pid, path, file_actions, attrp, argv, envp);
	marcel_sig_reset_timer();

	return ret;
}

int ma_posix_spawnp(pid_t * restrict pid, const char *restrict file,
		    const posix_spawn_file_actions_t * file_actions,
		    const posix_spawnattr_t * restrict attrp, char *const argv[restrict],
		    char *const envp[restrict])
{
	int ret;

	marcel_sig_stop_itimer();
	ret = PUK_ABI_REAL(posix_spawnp) (pid, file, file_actions, attrp, argv, envp);
	marcel_sig_reset_timer();

	return ret;
}
#endif


#endif
