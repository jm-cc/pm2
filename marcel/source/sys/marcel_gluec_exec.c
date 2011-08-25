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


#include <limits.h>
#include <stdarg.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include "marcel.h"


#define libc_exec_call(func, ...)				\
	do {							\
		int ret;					\
		marcel_sig_stop_itimer();			\
		ret = MA_REALSYM(func)(__VA_ARGS__);		\
		marcel_sig_reset_timer();			\
		return ret;					\
	} while (0);


DEF_MARCEL_PMARCEL(pid_t, wait, (int *status), (status),
{
	pid_t p;
	int old = __pmarcel_enable_asynccancel();
	p = marcel_wait4(-1, status, 0, NULL);
	__pmarcel_disable_asynccancel(old);
	return p;
})
DEF_C(pid_t, wait, (int *status), (status))

DEF_MARCEL_PMARCEL(pid_t, waitpid, (pid_t wpid, int *status, int options),
		   (wpid, status, options),
{
	pid_t p;
	int old = __pmarcel_enable_asynccancel();
	p = marcel_wait4(wpid, status, options, NULL);
	__pmarcel_disable_asynccancel(old);
	return p;
})
DEF_C(pid_t, waitpid, (pid_t wpid, int *status, int options), (wpid, status, options))

DEF_MARCEL_PMARCEL(int, waitid, (idtype_t idtype, id_t id, siginfo_t *infop, int options),
		   (idtype, id, infop, options),
{
	int retval;
	int old = __pmarcel_enable_asynccancel();
	marcel_extlib_protect();
	retval = MA_REALSYM(waitid)(idtype, id, infop, options);
	marcel_extlib_unprotect();
	__pmarcel_disable_asynccancel(old);
	return retval;
})
DEF_C(int, waitid, (idtype_t idtype, id_t id, siginfo_t *infop, int options),
      (idtype, id, infop, options))

DEF_MARCEL_PMARCEL(pid_t, wait3, (int *status, int options, struct rusage *rusage),
		   (status, options, rusage),
{
	return marcel_wait4(-1, status, options, rusage);
})
DEF_C(pid_t, wait3, (int *status, int options, struct rusage *rusage), (status, options, rusage))

DEF_MARCEL_PMARCEL(pid_t, wait4, (pid_t wpid, int *status, int options, struct rusage *rusage),
		   (wpid, status, options, rusage),
{
	/** syscall returns child not found on Darwin (errno = ECHILD) **/
	MA_CALL_LIBC_FUNC(pid_t, MA_REALSYM(wait4), wpid, status, options, rusage);
})
DEF_C(pid_t, wait4, (pid_t wpid, int *status, int options, struct rusage *rusage), (wpid, status, options, rusage))

DEF_MARCEL_PMARCEL_VAARGS(int, execl, (const char *path, const char *arg, ...),
{
	va_list exec_args;
	size_t arg_num;
	char *argv[_POSIX_ARG_MAX];

	arg_num = 1;
	argv[0] = (char *)arg;
	va_start(exec_args, arg);
	do {
		argv[arg_num] = va_arg(exec_args, char *);
	} while (argv[arg_num ++]);
	va_end(exec_args);
	
	return marcel_execv(path, argv);
})
DEF_STRONG_ALIAS(marcel_execl, execl)

DEF_MARCEL_PMARCEL_VAARGS(int, execlp, (const char *file, const char *arg, ...),
{
	va_list exec_args;
	size_t arg_num;
	char *argv[_POSIX_ARG_MAX];

	arg_num = 1;
	argv[0] = (char *)arg;
	va_start(exec_args, arg);
	do {
		argv[arg_num] = va_arg(exec_args, char *);
	} while (argv[arg_num ++]);
	va_end(exec_args);
	
	return marcel_execvp(file, argv);
})
DEF_STRONG_ALIAS(marcel_execlp, execlp)

DEF_MARCEL_PMARCEL_VAARGS(int, execle, (const char *path, const char *arg, ...),
{
	va_list exec_args;
	size_t arg_num;
	char *argv[_POSIX_ARG_MAX];
	char **envp;

	arg_num = 1;
	argv[0] = (char *)arg;
	va_start(exec_args, arg);
	do {
		argv[arg_num] = va_arg(exec_args, char *);
	} while (argv[arg_num ++]);
	envp = va_arg(exec_args, char **);
	va_end(exec_args);
	
	return marcel_execve(path, argv, envp);
})
DEF_STRONG_ALIAS(marcel_execle, execle)

DEF_MARCEL_PMARCEL(int, execve, (const char *name, char *const argv[], char *const envp[]),
	   (name, argv, envp),
{
	libc_exec_call(execve, name, argv, envp);
})
DEF_C(int, execve, (const char *name, char *const argv[], char *const envp[]), (name, argv, envp))

DEF_MARCEL_PMARCEL(int, execv, (const char *name, char *const argv[]), (name, argv),
{
	libc_exec_call(execv, name, argv);
})
DEF_C(int, execv, (const char *name, char *const argv[]), (name, argv))

DEF_MARCEL_PMARCEL(int, execvp, (const char *name, char *const argv[]), (name, argv),
{
	libc_exec_call(execvp, name, argv);
})
DEF_C(int, execvp, (const char *name, char *const argv[]), (name, argv))

DEF_MARCEL_PMARCEL(int, system, (const char *line), (line),
{
	int ret;
	int old = __pmarcel_enable_asynccancel();
	marcel_sig_stop_itimer();
	ret = MA_REALSYM(system)(line);
	marcel_sig_reset_timer();
        __pmarcel_disable_asynccancel(old);
	return ret;
})
DEF_C(int, system, (const char *line), (line))

#if HAVE_DECL_POSIX_SPAWN
DEF_MARCEL_PMARCEL(int, posix_spawn, (pid_t * __restrict pid, const char *__restrict path,
			      const posix_spawn_file_actions_t * file_actions,
			      const posix_spawnattr_t * __restrict attrp, char *const argv[],
			      char *const envp[]),
	   (pid, path, file_actions, attrp, argv, envp),
{
	libc_exec_call(posix_spawn, pid, path, file_actions, attrp, argv, envp);
})
DEF_C(int, posix_spawn, (pid_t * __restrict pid, const char *__restrict path,
			    const posix_spawn_file_actions_t * file_actions,
			    const posix_spawnattr_t * __restrict attrp, char *const argv[],
			    char *const envp[]),
	 (pid, path, file_actions, attrp, argv, envp))

DEF_MARCEL_PMARCEL(int, posix_spawnp, (pid_t * __restrict pid, const char *__restrict file,
			       const posix_spawn_file_actions_t * file_actions,
			       const posix_spawnattr_t * __restrict attrp, char *const argv[],
			       char *const envp[]),
	   (pid, file, file_actions, attrp, argv, envp),
{
	libc_exec_call(posix_spawnp, pid, file, file_actions, attrp, argv, envp);
})
DEF_C(int, posix_spawnp, (pid_t * __restrict pid, const char *__restrict file,
			     const posix_spawn_file_actions_t * file_actions,
			     const posix_spawnattr_t * __restrict attrp, char *const argv[],
			     char *const envp[]),
	 (pid, file, file_actions, attrp, argv, envp))
#endif
