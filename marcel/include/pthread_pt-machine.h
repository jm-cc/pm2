
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

#section marcel_macros

#depend "asm/linux_system.h[]"
#ifndef MEMORY_BARRIER
#  define MEMORY_BARRIER ma_smp_mb()
#endif
#ifndef READ_MEMORY_BARRIER
#  define READ_MEMORY_BARRIER() ma_smp_rmb()
#endif
#ifndef WRITE_MEMORY_BARRIER
#  define WRITE_MEMORY_BARRIER() ma_smp_wmb()
#endif

