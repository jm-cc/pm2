
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

#section common
#depend "asm/pthread_pt-machine.h[]"

#section marcel_macros

/* If MEMORY_BARRIER isn't defined in pt-machine.h, assume the
   architecture doesn't need a memory barrier instruction (e.g. Intel
   x86).  Still we need the compiler to respect the barrier and emit
   all outstanding operations which modify memory.  Some architectures
   distinguish between full, read and write barriers.  */

#ifndef MEMORY_BARRIER
#  define MEMORY_BARRIER() asm ("" : : : "memory")
#endif
#ifndef READ_MEMORY_BARRIER
#  define READ_MEMORY_BARRIER() MEMORY_BARRIER()
#endif
#ifndef WRITE_MEMORY_BARRIER
#  define WRITE_MEMORY_BARRIER() MEMORY_BARRIER()
#endif

