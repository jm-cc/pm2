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


#ifndef __ASM_SPARC64_MARCEL_TESTANDSET_H__
#define __ASM_SPARC64_MARCEL_TESTANDSET_H__


/** Public macros **/
#ifndef MARCEL_TESTANDSET___MACROS_H
#define MARCEL_TESTANDSET___MACROS_H

#include "tbx_compiler.h"
#define MA_HAVE_TESTANDSET 1


#endif /* MARCEL_TESTANDSET___MACROS_H */



#ifdef __MARCEL_KERNEL__


/** Internal macros **/
#ifndef MARCEL_TESTANDSET___MARCEL_MACROS_H
#define MARCEL_TESTANDSET___MARCEL_MACROS_H

#include "tbx_compiler.h"
#define pm2_spinlock_release(spinlock) \
  __asm__ __volatile__("stbar\n\tstb %1,%0" : "=m"(*(spinlock)) : "r"(0):"memory")


#endif /* MARCEL_TESTANDSET___MARCEL_MACROS_H */



/** Internal functions **/
#ifndef MARCEL_TESTANDSET___MARCEL_FUNCTIONS_H
#define MARCEL_TESTANDSET___MARCEL_FUNCTIONS_H

#include "tbx_compiler.h"
static __tbx_inline__ unsigned pm2_spinlock_testandset(volatile unsigned *spinlock);

#endif /* MARCEL_TESTANDSET___MARCEL_FUNCTIONS_H */



#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_SPARC64_MARCEL_TESTANDSET_H__ **/
