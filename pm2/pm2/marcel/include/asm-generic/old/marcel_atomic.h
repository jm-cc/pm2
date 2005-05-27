#section common
#include "tbx_compiler.h"
#section marcel_macros

#warning using generic atomic.h. Please provide one for this architecture.
#warning Look at the one in the kernel sources for example.

typedef unsigned atomic_t;

#define ATOMIC_INIT(i)	        (i)
#define atomic_read(v)		(*(v))
#define atomic_set(v, i)	(*(v) = (i))

static __tbx_inline__ 
void atomic_inc(volatile atomic_t *v) __attribute__ ((unused));
static __tbx_inline__ void atomic_inc(volatile atomic_t *v)
{
  (*v)++;
}

static __tbx_inline__ 
void atomic_dec(volatile atomic_t *v) __attribute__ ((unused));
static __tbx_inline__ void atomic_dec(volatile atomic_t *v)
{
  (*v)--;
}

