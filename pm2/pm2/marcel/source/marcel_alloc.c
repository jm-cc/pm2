
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "marcel.h"
#include "marcel_alloc.h"

static void *next_slot;

static marcel_lock_t alloc_lock = MARCEL_LOCK_INIT;

#if defined(SOLARIS_SYS) || defined(IRIX_SYS) || defined(FREEBSD_SYS)
static int __zero_fd;
#endif

static struct {
   unsigned last;
   void *stacks[MAX_STACK_CACHE];
} stack_cache = { 0, };

void marcel_slot_init(void)
{
#if defined(SOLARIS_SYS) || defined(IRIX_SYS) || defined(FREEBSD_SYS)
  __zero_fd = open("/dev/zero", O_RDWR);
#endif
  stack_cache.last = 0;

  next_slot = (void *)SLOT_AREA_TOP;
}

extern volatile unsigned long threads_created_in_cache;

void *marcel_slot_alloc(void)
{
  register void *ptr;

  marcel_lock_acquire(&alloc_lock);

  if(stack_cache.last) {

    ptr = stack_cache.stacks[--stack_cache.last];

    threads_created_in_cache++;

  } else {

    next_slot -= SLOT_SIZE;

    ptr = mmap(next_slot,
	       SLOT_SIZE,
	       PROT_READ | PROT_WRITE,
	       MMAP_MASK,
	       FILE_TO_MAP, 0);

    if(ptr == MAP_FAILED) {
      perror("mmap");
      RAISE(CONSTRAINT_ERROR);
    }
  }

  marcel_lock_release(&alloc_lock);

  return ptr;
}

void marcel_slot_free(void *addr)
{
  marcel_lock_acquire(&alloc_lock);

  if(stack_cache.last < MAX_STACK_CACHE) /* Si le cache n'est pas plein */
    stack_cache.stacks[stack_cache.last++] = addr;
  else
    if(munmap(addr, SLOT_SIZE) == -1)
      RAISE(CONSTRAINT_ERROR);

  marcel_lock_release(&alloc_lock);
}

void marcel_slot_exit(void)
{
  while(stack_cache.last) {
    stack_cache.last--;
    if(munmap(stack_cache.stacks[stack_cache.last], SLOT_SIZE) == -1)
      RAISE(CONSTRAINT_ERROR);
  }
}
