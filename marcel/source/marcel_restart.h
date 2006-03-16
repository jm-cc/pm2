
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

static inline marcel_t __thread_self() {
  return marcel_self();
}

static inline void restart(marcel_descr th)
{
  marcel_sem_V(&th->pthread_sync);
}

static inline void suspend(marcel_descr self)
{
  marcel_sem_P(&self->pthread_sync);  
}

#if 0
static inline int timedsuspend(marcel_descr self,
                const struct timespec *abstime)
{

}
#endif
