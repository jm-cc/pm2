
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

#if defined(SOLARIS_SYS)
#include <sys/trap.h>
#elif defined(LINUX_SYS)
#include <asm/traps.h>
#else
#error Please tell where ST_FLUSH_WINDOWS is defined on your system
#endif

	.seg	"text"

	.global	call_ST_FLUSH_WINDOWS
call_ST_FLUSH_WINDOWS:
	retl
	ta	ST_FLUSH_WINDOWS

