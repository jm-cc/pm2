/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2008 "the PM2 team" (see AUTHORS file)
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

#if defined(MM_MAMI_ENABLED) || defined(MM_HEAP_ENABLED)

#ifndef MM_HELPER_H
#define MM_HELPER_H

#include <sys/syscall.h>

/* The following is normally defined in <linux/mempolicy.h>. But that file is not always available. */
/* Policies */
enum {
	MPOL_DEFAULT,
	MPOL_PREFERRED,
	MPOL_BIND,
	MPOL_INTERLEAVE,
	MPOL_MAX,	/* always last member of enum */
};
/* Flags for mbind */
#  define MPOL_MF_STRICT	(1<<0)	/* Verify existing pages in the mapping */
#  define MPOL_MF_MOVE		(1<<1)	/* Move pages owned by this process to conform to mapping */

#if !defined(__NR_move_pages)
#  if defined(X86_64_ARCH)
#    define __NR_move_pages 279
#  elif defined(IA64_ARCH)
#    define __NR_move_pages 1276
#  elif defined(X86_ARCH)
#    define __NR_move_pages 317
#  elif defined(PPC_ARCH)
#    define __NR_move_pages 301
#  elif defined(PPC64_ARCH)
#    define __NR_move_pages 301
#  else
#    error Syscall move pages undefined
#  endif
#endif /* __NR_move_pages */

#if !defined(__NR_mbind)
#  if defined(X86_64_ARCH)
#    define __NR_mbind 237
#  elif defined(IA64_ARCH)
#    define __NR_mbind 1259
#  elif defined(X86_ARCH)
#    define __NR_mbind 274
#  elif defined(PPC_ARCH)
#    define __NR_mbind 259
#  elif defined(PPC64_ARCH)
#    define __NR_mbind 259
#  else
#    error Syscall mbind undefined
#  endif
#endif /* __NR_mbind */

#if !defined(__NR_set_mempolicy)
#  if defined(X86_64_ARCH)
#    define __NR_set_mempolicy 238
#  elif defined(IA64_ARCH)
#    define __NR_set_mempolicy 1261
#  elif defined(X86_ARCH)
#    define __NR_set_mempolicy 276
#  elif defined(PPC_ARCH)
#    define __NR_set_mempolicy 261
#  elif defined(PPC64_ARCH)
#    define __NR_set_mempolicy 261
#  else
#    error Syscall set_mempolicy undefined
#  endif
#endif /* __NR_set_mempolicy */

extern
int _mm_mbind_call(void *start, unsigned long len, int mode,
                   const unsigned long *nmask, unsigned long maxnode, unsigned flags);

extern
int _mm_mbind(void *start, unsigned long len, int mode,
              const unsigned long *nmask, unsigned long maxnode, unsigned flags);

extern
int _mm_move_pages(void **pageaddrs, int pages, int *nodes, int *status, int flag);

extern
int _mm_use_synthetic_topology(void);

#endif /* MM_HELPER_H */
#endif /* MM_MAMI_ENABLED || MM_HEAP_ENABLED */
