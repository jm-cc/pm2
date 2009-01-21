
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

#section common
#include "tbx_compiler.h"
#section macros
#define MA_HAVE_TESTANDSET 1

#section marcel_functions

static __tbx_inline__ unsigned 
pm2_spinlock_testandset(volatile unsigned *spinlock) ;


#section marcel_inline

#ifndef __INTEL_COMPILER
static __tbx_inline__ unsigned pm2_spinlock_testandset(volatile unsigned *spinlock)
{
        register long result;

        __asm__ __volatile__ (
                "mov ar.ccv=r0\n"
                ";;\n"
                "cmpxchg4.acq %0=[%2],%1,ar.ccv\n"
                : "=r"(result) : "r"(1), "r"(spinlock) : "ar.ccv", "memory");
        return result;
}
#endif

#section marcel_macros
#define pm2_spinlock_release(spinlock) do { ma_mb(); (*(spinlock) = 0); } while(0)


