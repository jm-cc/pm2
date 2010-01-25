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


#ifndef __MARCEL_COMPILER_H__
#define __MARCEL_COMPILER_H__


#include "sys/marcel_flags.h"


/** Public macros **/
#define ma_barrier() tbx_barrier()

#if defined(__MARCEL_KERNEL__) && (__GNUC__ < 3 || (__GNUC__ == 3 && __GNUC_MINOR__ < 3))
#error Your gcc is too old for Marcel, please upgrade to at least 3.3
#endif

/* pour les fonctions que l'on veut "extern inline" pour l'application, il
 * faut aussi fournir un symbole, dans marcel_extern.o */
#ifdef MARCEL_COMPILE_INLINE_FUNCTIONS
#  define MARCEL_INLINE TBX_EXTERN
#else
#  ifdef __MARCEL_KERNEL__
#    define MARCEL_INLINE TBX_EXTERN extern inline
#  else
#    define MARCEL_INLINE TBX_EXTERN extern
#  endif
#endif


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
#define __ma_cacheline_aligned


#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_COMPILER_H__ **/
