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

#if defined(MM_MAMI_ENABLED) || defined(MM_HEAP_ENABLED)

#ifndef MM_DEBUG_H
#define MM_DEBUG_H

#include "pm2_common.h"

extern debug_type_t debug_memory;
extern debug_type_t debug_memory_log;
extern debug_type_t debug_memory_ilog;
extern debug_type_t debug_memory_warn;

#define mdebug_memory(fmt, args...) \
    debug_printf(&debug_memory, "[%s] " fmt , __TBX_FUNCTION__, ##args)

#if defined(PM2DEBUG)
#  define MEMORY_LOG_IN()        debug_printf(&debug_memory_log, "%s: -->\n", __TBX_FUNCTION__)
#  define MEMORY_LOG_OUT()       debug_printf(&debug_memory_log, "%s: <--\n", __TBX_FUNCTION__)
#  define MEMORY_ILOG_IN()       debug_printf(&debug_memory_ilog, "%s: -->\n", __TBX_FUNCTION__)
#  define MEMORY_ILOG_OUT()      debug_printf(&debug_memory_ilog, "%s: <--\n", __TBX_FUNCTION__)
#else
#  define MEMORY_LOG_IN()
#  define MEMORY_LOG_OUT()
#  define MEMORY_ILOG_IN()
#  define MEMORY_ILOG_OUT()
#endif

/* --- debug functions --- */

#ifdef PM2DEBUG
#define mdebug_memory_list(str,root) heap_print_list(str,root)
#else
#define	mdebug_memory_list(...)
#endif


#endif /* MM_DEBUG_H */
#endif /* MM_MAMI_ENABLED || MM_HEAP_ENABLED */
