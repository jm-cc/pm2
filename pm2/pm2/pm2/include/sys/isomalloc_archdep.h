
#ifndef ISOMALLOC_ARCHDEP_IS_DEF
#define ISOMALLOC_ARCHDEP_IS_DEF

#define ISOADDR_PAGES (128*1024)
#define DYN_DSM_AREA_SIZE (ISOADDR_PAGES * 4096)
#define THREAD_SLOT_SIZE              0x10000 /* 64 Ko */

#if defined(SOLARIS_SYS) && defined(SPARC_ARCH)
extern int __zero_fd;
#define ISOADDR_AREA_TOP          0xe0000000
#define SLOT_AREA_TOP  (ISOADDR_AREA_TOP - DYN_DSM_AREA_SIZE)
#define IS_ON_MAIN_STACK(sp)   ((sp) > ISOADDR_AREA_TOP)
#define FILE_TO_MAP            __zero_fd
#define MMAP_MASK              (MAP_PRIVATE | MAP_FIXED)

#elif defined(LINUX_SYS) && defined(X86_ARCH)

#define ISOADDR_AREA_TOP          0x40000000
#define SLOT_AREA_TOP  (ISOADDR_AREA_TOP - DYN_DSM_AREA_SIZE)
#define MAIN_STACK_BOT         0xa0000000
#define IS_ON_MAIN_STACK(sp)   ((sp) > MAIN_STACK_BOT)
#define FILE_TO_MAP            -1
#define MMAP_MASK              (MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS)

#elif defined(LINUX_SYS) && defined(PPC_ARCH)

#define ISOADDR_AREA_TOP          0x30000000
#define SLOT_AREA_TOP  (ISOADDR_AREA_TOP - DYN_DSM_AREA_SIZE)
#define FILE_TO_MAP            -1
#define MMAP_MASK              (MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS)

#elif defined(SOLARIS_SYS) && defined(X86_ARCH)
extern int __zero_fd;
extern void common_init(int *argc, char **argv);
#define ISOADDR_AREA_TOP          0xafff0000
#define SLOT_AREA_TOP  (ISOADDR_AREA_TOP - DYN_DSM_AREA_SIZE)
#define IS_ON_MAIN_STACK(sp)   ((sp) < (unsigned long)common_init)
#define FILE_TO_MAP            __zero_fd
#define MMAP_MASK              (MAP_PRIVATE | MAP_FIXED)

#elif defined(FREEBSD_SYS) && defined(X86_ARCH)

#define ISOADDR_AREA_TOP          0x40000000
#define SLOT_AREA_TOP  (ISOADDR_AREA_TOP - DYN_DSM_AREA_SIZE)
#define MAIN_STACK_BOT         0xa0000000
#define IS_ON_MAIN_STACK(sp)   ((sp) > MAIN_STACK_BOT)
#define FILE_TO_MAP            __zero_fd
#define MMAP_MASK              (MAP_PRIVATE | MAP_FIXED)

#elif defined(AIX_SYS) && defined(RS6K_ARCH)

#define ISOADDR_AREA_TOP          0xcfff0000
#define SLOT_AREA_TOP  (ISOADDR_AREA_TOP - DYN_DSM_AREA_SIZE)
#define FILE_TO_MAP            -1
#define MMAP_MASK              (MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS)

#elif defined(IRIX_SYS) && defined(MIPS_ARCH)
extern int __zero_fd;
#define ISOADDR_AREA_TOP          0x40000000
#define SLOT_AREA_TOP  (ISOADDR_AREA_TOP - DYN_DSM_AREA_SIZE)
#define FILE_TO_MAP            __zero_fd
#define MMAP_MASK              (MAP_PRIVATE | MAP_FIXED)
#define IS_ON_MAIN_STACK(sp)   ((sp) > SLOT_AREA_TOP)

#else

#error Sorry. This system/architecture is not yet supported.

#endif

#define SLOT_SIZE              0x10000 /* 64 Ko */

#if defined(SOLARIS_SYS) || defined(IRIX_SYS) || defined(FREEBSD_SYS)
extern int _zerofd;
#endif

#endif

