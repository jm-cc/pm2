/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2008 "the PM2 team" (see AUTHORS file)
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

#if defined(MM_MAMI_ENABLED)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mm_debug.h"

unsigned long _mami_debug_mask;

void mm_debug_init(int argc, char **argv)
{
#ifdef MEMORY_DEBUG
        int i;
        _mami_debug_mask = 0;

        for(i=0 ; i<argc ; i++) {
                if (!strcmp(argv[i], "--help")) {
                        fprintf(stderr, "Available options:\n");
                        fprintf(stderr, "         --memory-debug\n");
                        fprintf(stderr, "         --memory-log\n");
                        fprintf(stderr, "         --memory-ilog\n");
                        exit(1);
                }
                else if (!strcmp(argv[i], "--memory-debug")) {
                        _mami_debug_mask |= MDEBUG_MEMORY_DEBUG_SET;
                }
                else if (!strcmp(argv[i], "--memory-log")) {
                        _mami_debug_mask |= MDEBUG_MEMORY_LOG_SET;
                }
                else if (!strcmp(argv[i], "--memory-ilog")) {
                        _mami_debug_mask |= MDEBUG_MEMORY_ILOG_SET;
                }
        }
#endif
}

#endif /* MM_MAMI_ENABLED */
