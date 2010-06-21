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


#ifndef __MARCEL_VALGRIND_H__
#define __MARCEL_VALGRIND_H__


#ifdef PM2VALGRIND
#include <valgrind/valgrind.h>
#include <valgrind/memcheck.h>
#endif

#ifndef VALGRIND_STACK_REGISTER
#define VALGRIND_STACK_REGISTER(start, end) ((void)0)
#endif

#ifndef VALGRIND_STACK_DEREGISTER
#define VALGRIND_STACK_DEREGISTER(id) ((void)0)
#endif


#endif /** __MARCEL_VALGRIND_H__ **/
