
#include <stdio.h>
#include <stdlib.h>
#include <Padico/Puk-ABI.h>

#define SIZE 10

static void lock_acquire(void)
{
  fprintf(stderr, "    acquire lock\n");
}
static void lock_release(void)
{
  fprintf(stderr, "    release lock\n");
}
static int lock_trylock(void)
{
  lock_acquire();
  return 0; /* success */
}

static void default_spinlock_acquire(void) { /* do nothing */ }
static void default_spinlock_release(void) { /* do nothing */ }
static int  default_spinlock_trylock(void) { return 0; }

void*my_real_malloc(size_t s)
{
  fprintf(stderr, "  inside virtual malloc. Calling real malloc...\n");
  void*_ptr = PUK_ABI_REAL(malloc)(s);
  fprintf(stderr, "  inside virtual malloc. Real malloc returned.\n");
  return _ptr;
}
void my_real_free(void*_ptr)
{
  fprintf(stderr, "  inside virtual free. Calling real free...\n");
  PUK_ABI_REAL(free)(_ptr);
  fprintf(stderr, "  inside virtual free. Real free returned.\n");
}

void*my_wrap_malloc(size_t s)
{
  fprintf(stderr, "  inside virtual malloc. Calling wrap malloc...\n");
  void*_ptr = PUK_ABI_WRAP(malloc)(s);
  fprintf(stderr, "  inside virtual malloc. Wrap malloc returned.\n");
  return _ptr;
}
void my_wrap_free(void*_ptr)
{
  fprintf(stderr, "  inside virtual free. Calling wrap free...\n");
  PUK_ABI_WRAP(free)(_ptr);
  fprintf(stderr, "  inside virtual free. Wrap free returned.\n");
}

int main(int argc, char**argv)
{
  void*ptr;

  fprintf(stderr, "* Step 1: no lock registered\n");
  ptr = malloc(SIZE);
  free(ptr);

  fprintf(stderr, "* Step 2: trace hooks registered as locks\n");
  puk_abi_set_spinlock_handlers(&lock_acquire, &lock_release, &lock_trylock);
  ptr = malloc(SIZE);
  free(ptr);

  fprintf(stderr, "* Step 3: malloc/free fully virtualized, calling real malloc\n");
  puk_abi_set_virtual(malloc, &my_real_malloc);
  puk_abi_set_virtual(free,   &my_real_free);
  ptr = malloc(SIZE);
  free(ptr);

  fprintf(stderr, "* Step 4: malloc/free fully virtualized, calling wrapped malloc\n");
  puk_abi_set_virtual(malloc, &my_wrap_malloc);
  puk_abi_set_virtual(free,   &my_wrap_free);
  ptr = malloc(SIZE);
  free(ptr);

  fprintf(stderr, "* Step 5: reverting virtual functions.\n");
  puk_abi_set_virtual(malloc, NULL);
  puk_abi_set_virtual(free,   NULL);
  ptr = malloc(SIZE);
  free(ptr);

  fprintf(stderr, "* Step 6: reverting lock hooks\n");
  puk_abi_set_spinlock_handlers(&default_spinlock_acquire, &default_spinlock_release, &default_spinlock_trylock);
  ptr = malloc(SIZE);
  free(ptr);

  fprintf(stderr, "* Ok! We are done.\n");

  return 0;
}
