/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.
*/

#ifndef ISOMALLOC_ARCHDEP_IS_DEF
#define ISOMALLOC_ARCHDEP_IS_DEF

#if defined(SOLARIS_SYS) && defined(SPARC_ARCH)

#define SLOT_AREA_TOP          0xe0000000
#define IS_ON_MAIN_STACK(sp)   ((sp) > SLOT_AREA_TOP)
#define FILE_TO_MAP            __zero_fd
#define MMAP_MASK              (MAP_PRIVATE | MAP_FIXED)

#elif defined(LINUX_SYS) && defined(X86_ARCH)

#define SLOT_AREA_TOP          0x40000000
#define MAIN_STACK_BOT         0xa0000000
#define IS_ON_MAIN_STACK(sp)   ((sp) > MAIN_STACK_BOT)
#define FILE_TO_MAP            -1
#define MMAP_MASK              (MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS)

#elif defined(LINUX_SYS) && defined(PPC_ARCH)

#define SLOT_AREA_TOP          0x30000000
#define FILE_TO_MAP            -1
#define MMAP_MASK              (MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS)

#elif defined(SOLARIS_SYS) && defined(X86_ARCH)

extern void marcel_init(int *argc, char **argv);
#define SLOT_AREA_TOP          0xafff0000
#define IS_ON_MAIN_STACK(sp)   ((sp) < (unsigned long)marcel_init)
#define FILE_TO_MAP            __zero_fd
#define MMAP_MASK              (MAP_PRIVATE | MAP_FIXED)

#elif defined(FREEBSD_SYS) && defined(X86_ARCH)

#define SLOT_AREA_TOP          0x40000000
#define MAIN_STACK_BOT         0xa0000000
#define IS_ON_MAIN_STACK(sp)   ((sp) > MAIN_STACK_BOT)
#define FILE_TO_MAP            __zero_fd
#define MMAP_MASK              (MAP_PRIVATE | MAP_FIXED)

#elif defined(AIX_SYS) && defined(RS6K_ARCH)

#define SLOT_AREA_TOP          0xcfff0000
#define FILE_TO_MAP            -1
#define MMAP_MASK              (MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS)

#elif defined(IRIX_SYS) && defined(MIPS_ARCH)

#define SLOT_AREA_TOP          0x40000000
#define FILE_TO_MAP            __zero_fd
#define MMAP_MASK              (MAP_PRIVATE | MAP_FIXED)

#else

#error Sorry. This system/architecture is not yet supported.

#endif

#define SLOT_SIZE              0x10000 /* 64 Ko */

#if defined(SOLARIS_SYS) || defined(IRIX_SYS) || defined(FREEBSD_SYS)
extern int _zerofd;
#endif

#endif
