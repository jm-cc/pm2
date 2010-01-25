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


#ifndef __LINUX_LINKAGE_H__
#define __LINUX_LINKAGE_H__


#include "sys/marcel_flags.h"
#ifdef __MARCEL_KERNEL__
#include "asm/linux_linkage.h"
#endif


/** Public macros **/
#ifndef FASTCALL
#  define FASTCALL(x)	x
#  define fastcall
#endif


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
#ifdef __cplusplus
#  define CPP_ASMLINKAGE extern "C"
#else
#  define CPP_ASMLINKAGE
#endif

#ifndef asmlinkage
#  define asmlinkage CPP_ASMLINKAGE
#endif


#endif /** __MARCEL_KERNEL__ **/


#endif /** __LINUX_LINKAGE_H__ **/
