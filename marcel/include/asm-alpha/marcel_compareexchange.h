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


#ifndef __MARCEL_COMPAREEXCHANGE_H__
#define __MARCEL_COMPAREEXCHANGE_H__


#ifdef OSF_SYS
#include "asm-generic/marcel_compareexchange.h"
#else
#include "asm/linux_system.h"
#define MA_HAVE_COMPAREEXCHANGE 1
#define MA_HAVE_FULLCOMPAREEXCHANGE 1
#endif


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
#ifndef OSF_SYS
#define pm2_compareexchange(p,o,n,s) __ma_cmpxchg((p),(o),(n),(s))
#endif


#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_COMPAREEXCHANGE_H__ **/
