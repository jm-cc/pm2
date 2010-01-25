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


#ifndef __ASM_I386_MARCEL_COMPAREEXCHANGE_H__
#define __ASM_I386_MARCEL_COMPAREEXCHANGE_H__


#ifdef __MARCEL_KERNEL__
#include "asm/linux_system.h"
#endif


/** Public macros **/
#define MA_HAVE_COMPAREEXCHANGE 1
#define MA_HAVE_FULLCOMPAREEXCHANGE 1


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
#define pm2_compareexchange(p,o,n,s) __ma_cmpxchg(p,(o),(n),(s))


#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_I386_MARCEL_COMPAREEXCHANGE_H__ **/
