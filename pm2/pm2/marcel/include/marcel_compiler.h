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

#section compiler
// AD: not all systems define __THROW
#if !defined(__THROW)
# if defined __cplusplus && (__GNUC__ >= 3 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 8))
#  define __THROW throw ()
# else
#  define __THROW
# endif
#endif

// AD: not all systems define __BEGIN|END_DECLS
#if !(defined(__BEGIN_DECLS) && defined(__END_DECLS))
#ifdef  __cplusplus
# define __BEGIN_DECLS extern "C" {
# define __END_DECLS   }
#else
# define __BEGIN_DECLS
# define __END_DECLS
#endif
#endif

#section marcel_compiler
#define ma_barrier() tbx_barrier()

#section marcel_macros
#define __ma_cacheline_aligned
