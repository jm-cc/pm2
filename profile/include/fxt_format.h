
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

#ifndef FXT_FORMAT_H
#define FXT_FORMAT_H

#if defined(MARCEL)
/****************************************************************/
#  define FXT_MARCEL
#  include "marcel.h"
#  define FXT_PERTHREAD_GET(var) SELF_GETMEM(var)
#  define FXT_PERTHREAD_SET(var, value) SELF_SETMEM(var, value)
#  define FXT_PERTHREAD_DEFINE(type, var)
typedef marcel_t fxt_thread_t;
#elif defined(KERNEL)
/****************************************************************/
#  define FXT_KERNEL
#  error FxT not ready for kernel support yet.
#else
/****************************************************************/
#  if ! defined(_PTHREAD)
#  warning FxT assuming POSIX threads with __thread support
#  endif
#  define FXT_POSIX
#  define FXT_PERTHREAD_GET(var) var
#  define FXT_PERTHREAD_SET(var, value) var=(value)
#  define FXT_PERTHREAD_DEFINE(type, var) __thread type var;
typedef pthread_t fxt_thread_t;
#endif

// Données organisées par zones dans la trace
struct fxt_zone {
	void* last_full_event;
	void* start;
	void* end;
	fxt_thread_t tid;
};

// Variables par thread (ie faire toto=th_buff revient à faire un
// get_specifique ou équivalent):

struct fxt_perthread {
	/* Les deux champs suivant doivent rester dans cet ordre pour 
	   pouvoir être lu en une seule instruction atomique */
	void* free;
	struct fxt_zone* cur_zone;
};

#endif
