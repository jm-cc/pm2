
#ifdef SMP
#include <sched.h>
#endif

#include "marcel.h"

void marcel_lock_init(marcel_lock_t *lock)
{
  *lock = MARCEL_LOCK_INIT;
}


