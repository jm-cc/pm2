

/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2004 "the PM2 team" (see AUTHORS file)
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

#section marcel_types
#ifdef MA__LWPS
typedef unsigned long ma_cpu_set_t;
#else
typedef unsigned ma_cpu_set_t;
#endif

#section marcel_macros
#depend "linux_bitops.h[]"
#define MA_CPU_EMPTY			(0UL)
#define MA_CPU_FULL			(~0UL)
#define MA_CPU_ZERO(cpusetp)		(*(cpusetp)=MA_CPU_EMPTY)
#ifdef MA__LWPS
#define MA_CPU_FILL(cpusetp)		(*(cpusetp)=MA_CPU_FULL)
#define MA_CPU_SET(cpu, cpusetp)	(*(cpusetp)|=1UL<<(cpu))
#define MA_CPU_CLR(cpu, cpusetp)	(*(cpusetp)&=~(1UL<<(cpu)))
#define MA_CPU_ISSET(cpu, cpusetp)	((*(cpusetp)&(1UL<<(cpu))) != 0)
#define MA_CPU_WEIGHT(cpusetp)		ma_hweight_long(*(cpusetp))
#else
#define MA_CPU_FILL(cpusetp)		(*(cpusetp)=1)
#define MA_CPU_SET(cpu, cpusetp)	MA_CPU_FILL(cpusetp)
#define MA_CPU_CLR(cpu, cpusetp)	MA_CPU_ZERO(cpusetp)
#define MA_CPU_ISSET(cpu, cpusetp)	(*(cpusetp))
#define MA_CPU_WEIGHT(cpusetp)		(*(cpusetp))
#endif
