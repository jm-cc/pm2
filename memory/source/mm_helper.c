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

#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include "mm_helper.h"
#include "mm_debug.h"

int _mm_mbind(void *start, unsigned long len, int mode,
              const unsigned long *nmask, unsigned long maxnode, unsigned flags) {
  int err = 0;

  MEMORY_ILOG_IN();
  if (_mm_use_synthetic_topology()) {
      MEMORY_ILOG_OUT();
      return err;
  }
  mdebug_memory("binding on mask %lud\n", *nmask);
#if defined (X86_64_ARCH) && defined (X86_ARCH)
  err = syscall6(__NR_mbind, (long)start, len, mode, (long)nmask, maxnode, flags);
#else
  err = syscall(__NR_mbind, (long)start, len, mode, (long)nmask, maxnode, flags);
#endif
  if (err < 0) perror("(_mm_mbind) mbind");
  return err;
}

int _mm_move_pages(void **pageaddrs, int pages, int *nodes, int *status, int flag) {
  int err=0;

  MEMORY_ILOG_IN();
  if (_mm_use_synthetic_topology()) {
      MEMORY_ILOG_OUT();
      return err;
  }
  if (nodes) mdebug_memory("binding on numa node #%d\n", nodes[0]);

#if defined (X86_64_ARCH) && defined (X86_ARCH)
  err = syscall6(__NR_move_pages, 0, pages, pageaddrs, nodes, status, flag);
#else
  err = syscall(__NR_move_pages, 0, pages, pageaddrs, nodes, status, flag);
#endif
  if (err < 0) perror("(_mm_move_pages) move_pages");
  MEMORY_ILOG_OUT();
  return err;
}

int _mm_use_synthetic_topology(void) {
#ifdef MARCEL
  return marcel_use_synthetic_topology;
#else
  return 0;
#endif
}

#endif /* MM_MAMI_ENABLED || MM_HEAP_ENABLED */
