
#ifndef MAR_FOR_WIN_EST_DEF
#define MAR_FOR_WIN_EST_DEF

#ifdef WIN_SYS

void marcel_for_win_init(int *argc, char *argv[]);

#include <sys/time.h>

#define	timerclear(tvp)		((tvp)->tv_sec = (tvp)->tv_usec = 0)
#define	timercmp(a, b, CMP) 						      \
  (((a)->tv_sec == (b)->tv_sec) ? 					      \
   ((a)->tv_usec CMP (b)->tv_usec) : 					      \
   ((a)->tv_sec CMP (b)->tv_sec))

#include <sys/mman.h>

#ifndef __WIN_MMAP_KERNEL__
#define mmap(a, l, p, f, fd, o)    win_mmap(a, l, p, f, fd, o)
#define munmap(a, l)               win_munmap(a, l)
#endif

extern caddr_t win_mmap(caddr_t __addr, size_t __len, int __prot, int __flags,
		    int __fd, off_t __off);

extern int win_munmap(caddr_t __addr, size_t __len);

#endif

#endif
