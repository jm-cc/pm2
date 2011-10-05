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


#ifndef __SYS_MARCEL_GLUEC_REALSYM_H__
#define __SYS_MARCEL_GLUEC_REALSYM_H__


#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <termios.h>
#include <poll.h>
#include "tbx.h"
#include "marcel_config.h"
#ifdef MARCEL_LIBPTHREAD
#include "sys/os/marcel_gluec_realsym.h"
#endif


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Symbol identifiers **/
#ifdef MARCEL_LIBPTHREAD
#  define MA_REALSYM(name) ma_realsym_##name

#  ifdef MARCEL_REALSYM_C
#    define DEC_MA_REALSYM(retval, name, proto)		\
	retval (*MA_REALSYM(name)) proto = NULL;
#  else
#    define DEC_MA_REALSYM(retval, name, proto)	\
	extern retval (*MA_REALSYM(name)) proto;
#  endif

#  define DEF_MA_REALSYM(handle, name) {				\
		MA_REALSYM(name) = (typeof(MA_REALSYM(name)))dlsym(handle, #name); \
		if (NULL == MA_REALSYM(name))	{			\
			fprintf(stderr, "realsym: %s cannot be dlsymed\n", #name); \
			abort();					\
		}							\
	}
#else
#  define MA_REALSYM(name) name
#endif /** MARCEL_LIBPTHREAD **/

#define MA_CALL_LIBC_FUNC(rtype, func, ...)				\
	do {								\
		marcel_extlib_protect();				\
		rtype __ma_status = func(__VA_ARGS__);			\
		marcel_extlib_unprotect();				\
		return __ma_status;					\
	} while (0)


#ifdef MARCEL_LIBPTHREAD
/** memory **/
DEC_MA_REALSYM(void*, mmap, (void *addr, size_t length, int prot, int flags, int fd, off_t offset))
DEC_MA_REALSYM(int, munmap, (void *addr, size_t length))

/** exec **/
DEC_MA_REALSYM(int, execve, (const char *name, char *const argv[], char *const envp[]))
DEC_MA_REALSYM(int, execv, (const char *name, char *const argv[]))
DEC_MA_REALSYM(int, execvp, (const char *name, char *const argv[]))
DEC_MA_REALSYM(int, system, (const char *command))
DEC_MA_REALSYM(pid_t, fork, (void))
DEC_MA_REALSYM(int, waitid, (idtype_t idtype, id_t id, siginfo_t *infop, int options))
DEC_MA_REALSYM(pid_t, wait4, (pid_t pid, int *status, int options, struct rusage *rusage))
#if HAVE_DECL_POSIX_SPAWN
#include <spawn.h>
DEC_MA_REALSYM(int, posix_spawn, 
	       (pid_t * __restrict pid, const char *__restrict path,
		const posix_spawn_file_actions_t * file_actions,
		const posix_spawnattr_t * __restrict attrp, char *const argv[],
		char *const envp[]))
DEC_MA_REALSYM(int, posix_spawnp, 
	       (pid_t * __restrict pid, const char *__restrict file,
		const posix_spawn_file_actions_t * file_actions,
		const posix_spawnattr_t * __restrict attrp, char *const argv[],
		char *const envp[]))
#endif

/** sysio **/
DEC_MA_REALSYM(ssize_t, readv, (int fd, const struct iovec *iov, int iovcnt))
DEC_MA_REALSYM(ssize_t, writev, (int fd, const struct iovec *iov, int iovcnt))
DEC_MA_REALSYM(int, fsync, (int fd))
DEC_MA_REALSYM(int, tcdrain, (int fd))
DEC_MA_REALSYM(int, msync, (void *addr, size_t len, int flags))
DEC_MA_REALSYM(int, select, (int nfds, fd_set *readfds, fd_set *writefds, 
			     fd_set *exceptfds, struct timeval *timeout))
DEC_MA_REALSYM(int, poll, (struct pollfd *fds, nfds_t nfds, int timeout))
DEC_MA_REALSYM(ssize_t, pread,(int fd, void *buf, size_t count, off_t pos))
DEC_MA_REALSYM(ssize_t, pwrite, (int fd, const void *buf, size_t count, off_t pos))
DEC_MA_REALSYM(int, connect, (int sockfd, const struct sockaddr * serv_addr, socklen_t addrlen))
DEC_MA_REALSYM(int, accept, (int sockfd, struct sockaddr * addr, socklen_t * addrlen))
DEC_MA_REALSYM(ssize_t, sendto, (int s, const void *buf, size_t len, int flags, 
				 const struct sockaddr * to, socklen_t tolen))
DEC_MA_REALSYM(ssize_t, sendmsg, (int socket, const struct msghdr *message, int flags))
DEC_MA_REALSYM(ssize_t, recvfrom, (int s, void *buf, size_t len, int flags, 
				   struct sockaddr * from, socklen_t * fromlen))
DEC_MA_REALSYM(ssize_t, recvmsg, (int socket, struct msghdr * message, int flags))
#endif /** MARCEL_LIBPTHREAD **/


/** Internal interface **/
void ma_realsym_init(void);
void ma_realsym_exit(void);


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __SYS_MARCEL_GLUEC_REALSYM_H__ **/
