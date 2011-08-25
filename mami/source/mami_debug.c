/* MaMI --- NUMA Memory Interface
 *
 * Copyright (C) 2008, 2009, 2010, 2011  Centre National de la Recherche Scientifique
 *
 * MaMI is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * MaMI is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU Lesser General Public License in COPYING.LGPL for more details.
 */
#if defined(MAMI_ENABLED)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mami_debug.h"

unsigned long _mami_debug_mask;

#ifdef MEMORY_DEBUG
static void _mami_process_string(char *option)
{
        if (strstr(option, "--help")) {
                fprintf(stderr, "Available options:\n");
                fprintf(stderr, "         --memory-debug\n");
                fprintf(stderr, "         --memory-log\n");
                fprintf(stderr, "         --memory-ilog\n");
                exit(1);
        }
        if (strstr(option, "--memory-debug")) {
                _mami_debug_mask |= MDEBUG_MEMORY_DEBUG_SET;
        }
        if (strstr(option, "--memory-log")) {
                _mami_debug_mask |= MDEBUG_MEMORY_LOG_SET;
        }
        if (strstr(option, "--memory-ilog")) {
                _mami_debug_mask |= MDEBUG_MEMORY_ILOG_SET;
        }
}
#endif

void mami_debug_init(int argc, char **argv)
{
#ifdef MEMORY_DEBUG
        int i;
        char *env;

        _mami_debug_mask = 0;

        for(i=0 ; i<argc ; i++) {
                _mami_process_string(argv[i]);
        }

        env = getenv("MAMI_DEBUG");
        if (env) _mami_process_string(env);
#endif
}

#endif /* MAMI_ENABLED */
