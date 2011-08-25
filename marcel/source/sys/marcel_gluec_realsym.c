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


#define MARCEL_REALSYM_C
#include <dlfcn.h>
#include "sys/marcel_gluec_stdio.h"
#include "sys/marcel_gluec_realsym.h"
#include "sys/marcel_utils.h"


#if defined(HAVE_GNU_LIB_NAMES_H) && HAVE_GNU_LIB_NAMES_H
#  include <gnu/lib-names.h>
#  define LIBC_FILE LIBC_SO
#endif


static void *libc_handle = NULL;


void ma_realsym_init(void)
{
	/** realsym_init already called **/
	if (libc_handle)
		return;

	libc_handle = dlopen(LIBC_FILE, RTLD_LAZY);
	if (! libc_handle)
		abort();

#ifdef MARCEL_LIBPTHREAD
	/** memory **/
	DEF_MA_REALSYM(libc_handle, mmap);
	DEF_MA_REALSYM(libc_handle, munmap);

	/** exec **/
	DEF_MA_REALSYM(libc_handle, execve);
	DEF_MA_REALSYM(libc_handle, execv);
	DEF_MA_REALSYM(libc_handle, execvp);
	DEF_MA_REALSYM(libc_handle, system);
	DEF_MA_REALSYM(libc_handle, fork);
	DEF_MA_REALSYM(libc_handle, waitid);
	DEF_MA_REALSYM(libc_handle, wait4);
#  if HAVE_DECL_POSIX_SPAWN
	DEF_MA_REALSYM(libc_handle, posix_spawn);
	DEF_MA_REALSYM(libc_handle, posix_spawnp);
#  endif

	/** sysio **/
	DEF_MA_REALSYM(libc_handle, readv);
	DEF_MA_REALSYM(libc_handle, writev);
	DEF_MA_REALSYM(libc_handle, fsync);
	DEF_MA_REALSYM(libc_handle, tcdrain);
	DEF_MA_REALSYM(libc_handle, msync);
	DEF_MA_REALSYM(libc_handle, select);
	DEF_MA_REALSYM(libc_handle, poll);
	DEF_MA_REALSYM(libc_handle, pread);
	DEF_MA_REALSYM(libc_handle, pwrite);
	DEF_MA_REALSYM(libc_handle, connect);
	DEF_MA_REALSYM(libc_handle, accept);
	DEF_MA_REALSYM(libc_handle, sendto);
	DEF_MA_REALSYM(libc_handle, sendmsg);
	DEF_MA_REALSYM(libc_handle, recvfrom);
	DEF_MA_REALSYM(libc_handle, recvmsg);
#endif /** MARCEL_LIBPTHREAD **/
}

void ma_realsym_exit(void)
{
	if (libc_handle) {
		dlclose(libc_handle);
		libc_handle = NULL;
	}
}
