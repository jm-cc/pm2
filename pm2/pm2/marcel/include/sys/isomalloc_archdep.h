
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

#ifndef ISOMALLOC_ARCHDEP_IS_DEF
#define ISOMALLOC_ARCHDEP_IS_DEF

#define ISOADDR_PAGES                 (128*1024)
#define THREAD_SLOT_SIZE              0x10000 /* 64 Ko */
#define SLOT_AREA_TOP                 (ISOADDR_AREA_TOP - DYN_DSM_AREA_SIZE)

#ifdef DSM
#define DYN_DSM_AREA_SIZE             (ISOADDR_PAGES * 4096)
#else
#define DYN_DSM_AREA_SIZE             0
#endif

#if defined(SOLARIS_SYS) && defined(SPARC_ARCH)

extern int __zero_fd;
#define ISOADDR_AREA_TOP       0xe0000000
#define IS_ON_MAIN_STACK(sp)   ((sp) > ISOADDR_AREA_TOP)
#define FILE_TO_MAP            __zero_fd
#define MMAP_MASK              (MAP_PRIVATE | MAP_FIXED)

#elif defined(LINUX_SYS) && defined(X86_ARCH)

#define ISOADDR_AREA_TOP       0x40000000
#define MAIN_STACK_BOT         0xa0000000
#define IS_ON_MAIN_STACK(sp)   ((sp) > MAIN_STACK_BOT)
#define FILE_TO_MAP            -1
#define MMAP_MASK              (MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS)

#elif defined(LINUX_SYS) && defined(PPC_ARCH)

#define ISOADDR_AREA_TOP       0x30000000
#define FILE_TO_MAP            -1
#define MMAP_MASK              (MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS)

#elif defined(SOLARIS_SYS) && defined(X86_ARCH)

extern int __zero_fd;
extern int main();
#define ISOADDR_AREA_TOP       0xafff0000
#define IS_ON_MAIN_STACK(sp)   ((sp) < (unsigned long)common_init)
#define FILE_TO_MAP            __zero_fd
#define MMAP_MASK              (MAP_PRIVATE | MAP_FIXED)

#elif defined(FREEBSD_SYS) && defined(X86_ARCH)

#define ISOADDR_AREA_TOP       0x40000000
#define MAIN_STACK_BOT         0xa0000000
#define IS_ON_MAIN_STACK(sp)   ((sp) > MAIN_STACK_BOT)
#define FILE_TO_MAP            __zero_fd
#define MMAP_MASK              (MAP_PRIVATE | MAP_FIXED)

#elif defined(AIX_SYS) && defined(RS6K_ARCH)

#define ISOADDR_AREA_TOP       0xcfff0000
#define FILE_TO_MAP            -1
#define MMAP_MASK              (MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS)

#elif defined(IRIX_SYS) && defined(MIPS_ARCH)

extern int __zero_fd;
#define ISOADDR_AREA_TOP       0x40000000
#define FILE_TO_MAP            __zero_fd
#define MMAP_MASK              (MAP_PRIVATE | MAP_FIXED)
#define IS_ON_MAIN_STACK(sp)   ((sp) > SLOT_AREA_TOP)

#elif defined(WIN_SYS) && defined(X86_ARCH)

extern void *ISOADDR_AREA_TOP;
#define MAIN_STACK_BOT         (ISOADDR_AREA_TOP)
#define ISOADDR_AREA_SIZE      (512 * SLOT_SIZE)
#define FILE_TO_MAP            -1
#define IS_ON_MAIN_STACK(sp)   ((sp) > MAIN_STACK_BOT)
#define MMAP_MASK              (MAP_PRIVATE | MAP_ANONYMOUS)

#elif defined(LINUX_SYS) && defined(ALPHA_ARCH)

#define ISOADDR_AREA_TOP       0x30000000000
#define MAIN_STACK_TOP         0x130000000
#define IS_ON_MAIN_STACK(sp)   ((unsigned long)(sp) < MAIN_STACK_TOP)
#define FILE_TO_MAP            -1
#define MMAP_MASK              (MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS)

#elif defined(OSF_SYS) && defined(ALPHA_ARCH)

#define ISOADDR_AREA_TOP       0x30000000000
#define MAIN_STACK_TOP         0x130000000
#define IS_ON_MAIN_STACK(sp)   ((unsigned long)(sp) < MAIN_STACK_TOP)
#define FILE_TO_MAP            -1
#define MMAP_MASK              (MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS)

#else

#error Sorry. This system/architecture is not yet supported.

#endif

#define SLOT_SIZE              THREAD_SLOT_SIZE

#if defined(SOLARIS_SYS) || defined(IRIX_SYS) || defined(FREEBSD_SYS)
extern int _zerofd;
#endif

#endif

