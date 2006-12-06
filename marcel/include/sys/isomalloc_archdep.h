
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

/* SLOT_AREA_TOP doit �tre un multiple de THREAD_SLOT_SIZE
 * et THREAD_SLOT_SIZE doit �tre une puissance de deux */

#define ISOADDR_PAGES                 (128*1024)
/* Attention : ASM_THREAD_SLOT_SIZE red�fini pour certaines archis */
/* il doit �tre une puissance de deux, et au moins deux fois plus grand que
 * PTHREAD_STACK_MIN */
/* Pas de typage pour ASM_THREAD_SLOT_SIZE car la constante est utilisée
   dans un source assembleur */
#ifdef MA__LIBPTHREAD
#define ASM_THREAD_SLOT_SIZE          (0x100000) /* 1 Mo */
#else
#define ASM_THREAD_SLOT_SIZE          (0x10000) /* 64 Ko */
#endif
#define THREAD_SLOT_SIZE              ((long)ASM_THREAD_SLOT_SIZE)
#define SLOT_AREA_TOP                 ((unsigned long) ISOADDR_AREA_TOP - DYN_DSM_AREA_SIZE)

#ifdef DSM
#  define DYN_DSM_AREA_SIZE             (ISOADDR_PAGES * 4096)
#else
#  define DYN_DSM_AREA_SIZE             0
#endif

#ifdef SOLARIS_SYS
extern int __zero_fd;
#  ifdef SPARC_ARCH
#    define ISOADDR_AREA_TOP       0xe0000000
#    define IS_ON_MAIN_STACK(sp)   ((sp) > ISOADDR_AREA_TOP)
#    define FILE_TO_MAP            __zero_fd
#    define MMAP_MASK              (MAP_PRIVATE | MAP_FIXED)
#  elif defined(X86_ARCH)
#    define ISOADDR_AREA_TOP       0xafff0000
#    define IS_ON_MAIN_STACK(sp)   ((sp) < (unsigned long)common_init)
#    define FILE_TO_MAP            __zero_fd
#    define MMAP_MASK              (MAP_PRIVATE | MAP_FIXED)
#  else
#    error Sorry. This architecture is not yet supported.
#  endif
#elif defined(LINUX_SYS)
#  ifdef X86_ARCH
#    define ISOADDR_AREA_TOP       0x40000000
#    define SLOT_AREA_BOTTOM       0x10000000
#    define MAIN_STACK_BOT         0xa8000000
#    define IS_ON_MAIN_STACK(sp)   ((sp) > MAIN_STACK_BOT)
#  elif defined(X86_64_ARCH)
#    ifdef PM2VALGRIND
#      define ISOADDR_AREA_TOP       0x20000000
#      define SLOT_AREA_BOTTOM       0x18000000
#      define MAIN_STACK_BOT         0xffffffff
#    elif defined(MA__PROVIDE_TLS)
#      define ISOADDR_AREA_TOP       0x100000000
#      define SLOT_AREA_BOTTOM       0x10000000
#      define MAIN_STACK_BOT         0xffffffff
#    else
#      define ISOADDR_AREA_TOP       0x2000000000
#      define SLOT_AREA_BOTTOM       0x1000000000
#      define MAIN_STACK_BOT         0x7000000000
#    endif
#    define IS_ON_MAIN_STACK(sp)   ((sp) > MAIN_STACK_BOT)
#  elif defined(IA64_ARCH)
#    undef ASM_THREAD_SLOT_SIZE
     /* 0x30000 nécessaire pour pthread_create */
#    define ASM_THREAD_SLOT_SIZE   (0x80000) /* 512 Ko */
#    define ISOADDR_AREA_TOP       0x10000000000
#    define MAIN_STACK_BOT         0x6000000000000000
#    define IS_ON_MAIN_STACK(sp)   ((sp) > MAIN_STACK_BOT)
#  elif defined(PPC_ARCH)
#    define ISOADDR_AREA_TOP       0x30000000
#    define MAIN_STACK_BOT         0x80000000
#    define IS_ON_MAIN_STACK(sp)   ((sp) > MAIN_STACK_BOT)
#  elif defined(ALPHA_ARCH)
#    define ISOADDR_AREA_TOP       0x30000000000
#    define MAIN_STACK_TOP         0x130000000
#    define IS_ON_MAIN_STACK(sp)   ((unsigned long)(sp) < MAIN_STACK_TOP)
#  elif defined(SPARC_ARCH)
#    define ISOADDR_AREA_TOP       0x70000000
#    define SLOT_AREA_BOTTOM       0x10000000
#    define MAIN_STACK_BOT         0xe8000000
#    define IS_ON_MAIN_STACK(sp)   ((sp) > MAIN_STACK_BOT)
#  else
#    error Sorry. This architecture is not yet supported.
#  endif
#  define FILE_TO_MAP            -1
#  define MMAP_MASK              (MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS)
#elif defined(GNU_SYS) && defined(X86_ARCH)
#    define ISOADDR_AREA_TOP       0xc0000000
#    define SLOT_AREA_BOTTOM       0x10000000
#    define MAIN_STACK_BOT         0x00000000
#    define MAIN_STACK_TOP         0x01020000
#    define IS_ON_MAIN_STACK(sp)   ((sp) < MAIN_STACK_TOP)
#    define FILE_TO_MAP            -1
#    define MMAP_MASK              (MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS)
#elif defined(OSF_SYS) && defined(ALPHA_ARCH)
#    define ISOADDR_AREA_TOP       0x30000000000
#    define MAIN_STACK_TOP         0x130000000
#    define IS_ON_MAIN_STACK(sp)   ((unsigned long)(sp) < MAIN_STACK_TOP)
#    define FILE_TO_MAP            -1
#    define MMAP_MASK              (MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS)
#elif defined(FREEBSD_SYS) && defined(X86_ARCH)
#    define ISOADDR_AREA_TOP       0x40000000
#    define MAIN_STACK_BOT         0xa0000000
#    define IS_ON_MAIN_STACK(sp)   ((sp) > MAIN_STACK_BOT)
#    define FILE_TO_MAP            __zero_fd
#    define MMAP_MASK              (MAP_PRIVATE | MAP_FIXED)
#elif defined(DARWIN_SYS)
#  if defined(PPC_ARCH)
#    define ISOADDR_AREA_TOP       0xb0000000
#    define MAIN_STACK_BOT         0x90000000
#  elif defined(X86_ARCH)
#    define ISOADDR_AREA_TOP       0x80000000
#    define MAIN_STACK_BOT         0xbf800000
#  else
#    error Sorry. This architecture is not yet supported.
#  endif
#    define IS_ON_MAIN_STACK(sp)   ((sp) > MAIN_STACK_BOT)
#    define FILE_TO_MAP            -1
#    define MMAP_MASK              (MAP_PRIVATE | MAP_FIXED | 0x1000)
#elif defined(AIX_SYS)
#    if defined(RS6K_ARCH) || defined(PPC_ARCH)
#      define ISOADDR_AREA_TOP       (0xd0000000 - THREAD_SLOT_SIZE)
#    endif
#    define FILE_TO_MAP            -1
#    define MMAP_MASK              (MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS)
#elif defined(IRIX_SYS) && defined(MIPS_ARCH)
extern int __zero_fd;
#    define ISOADDR_AREA_TOP       0x40000000
#    define FILE_TO_MAP            __zero_fd
#    define MMAP_MASK              (MAP_PRIVATE | MAP_FIXED)
#    define IS_ON_MAIN_STACK(sp)   ((sp) > SLOT_AREA_TOP)
#elif defined(WIN_SYS) && defined(X86_ARCH)
extern void *ISOADDR_AREA_TOP;
#    define MAIN_STACK_BOT         (ISOADDR_AREA_TOP)
#    define ISOADDR_AREA_SIZE      (512 * THREAD_SLOT_SIZE)
#    define FILE_TO_MAP            -1
#    define IS_ON_MAIN_STACK(sp)   ((sp) > MAIN_STACK_BOT)
#    ifdef __MINGW32__
#      define MMAP_MASK              0
#    else
#      define MMAP_MASK              (MAP_PRIVATE | MAP_ANONYMOUS)
#    endif
#else
#  error Sorry. This system/architecture is not yet supported.
#endif

#ifdef PM2VALGRIND
#    ifdef ENABLE_STACK_JUMPING
#	error "valgrind doesn't work with stack jumping enabled"
#    endif
#    ifdef SLOT_AREA_BOTTOM
#       undef IS_ON_MAIN_STACK
#	define IS_ON_MAIN_STACK(sp)   ((sp) > ISOADDR_AREA_TOP || (sp) < SLOT_AREA_BOTTOM )
#    else
#	error "IS_ON_MAIN_STACK needs to be fixed for valgrind support on this architecture"
#    endif
#endif

#if defined(SOLARIS_SYS) || defined(IRIX_SYS) || defined(FREEBSD_SYS)
extern int _zerofd;
#endif

#endif

