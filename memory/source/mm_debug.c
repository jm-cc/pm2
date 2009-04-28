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

#include "mm_debug.h"

debug_type_t debug_memory = NEW_DEBUG_TYPE("MEMORY: ", "memory-debug");
debug_type_t debug_memory_log = NEW_DEBUG_TYPE("MEMORY: ", "memory-log");
debug_type_t debug_memory_ilog = NEW_DEBUG_TYPE("MEMORY: ", "memory-ilog");
debug_type_t debug_memory_warn = NEW_DEBUG_TYPE("MEMORY: ", "memory-warn");

#endif /* MM_MAMI_ENABLED || MM_HEAP_ENABLED */
