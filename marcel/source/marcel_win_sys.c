
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

#ifdef WIN_SYS

#define __WIN_MMAP_KERNEL__

#include <stdio.h>
#include <stdlib.h>

#include "marcel.h"

#include <windows.h>

void *ISOADDR_AREA_TOP;
static void *ISOADDR_AREA_BOTTOM;

void *win_mmap(void *__addr, size_t __len)
{
	return __addr;
}

int win_munmap(void *__addr, size_t __len)
{
	return 0;
}

#else

#include <marcel.h>

#endif				// WIN_SYS

void marcel_win_sys_init(int *argc, char *argv[])
{
#ifdef WIN_SYS
#ifdef __CYGWIN__
	ISOADDR_AREA_BOTTOM = mmap(NULL,
	    ISOADDR_AREA_SIZE,
	    PROT_READ | PROT_WRITE, MMAP_MASK, FILE_TO_MAP, 0);

	if (ISOADDR_AREA_BOTTOM == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}
#else
	ISOADDR_AREA_BOTTOM = HeapAlloc(GetProcessHeap(), 0, ISOADDR_AREA_SIZE);
	if (!ISOADDR_AREA_BOTTOM) {
		fprintf(stderr, "HeapAlloc() failed (%ld)", GetLastError());
		exit(1);
	}
#endif

	ISOADDR_AREA_TOP = ISOADDR_AREA_BOTTOM + ISOADDR_AREA_SIZE;

	// Ajustements pour aligner sur une frontière de THREAD_SLOT_SIZE
	ISOADDR_AREA_BOTTOM = (void *) (((unsigned long) ISOADDR_AREA_BOTTOM +
		THREAD_SLOT_SIZE - 1) & ~(THREAD_SLOT_SIZE - 1));
	ISOADDR_AREA_TOP = (void *) ((unsigned long) ISOADDR_AREA_TOP &
	    ~(THREAD_SLOT_SIZE - 1));
	mdebug("isoaddr from %p to %p\n", ISOADDR_AREA_BOTTOM,
	    ISOADDR_AREA_TOP);
#endif
}
