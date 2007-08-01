/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
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

#include "tbx.h"

extern debug_type_t debug_nm_so_trace;
extern debug_type_t debug_nm_so_sr_trace;
extern debug_type_t debug_nm_so_sr_log;

#define NM_SO_TRACE(fmt, args...) \
    debug_printf(&debug_nm_so_trace, "[%s] " fmt ,__TBX_FUNCTION__ ,##args)

#define NM_SO_TRACE_LEVEL(level, fmt, args...) \
    debug_printfl(&debug_nm_so_trace, level, "[%s] " fmt ,__TBX_FUNCTION__  , ##args)

#define NM_SO_SR_TRACE(fmt, args...) \
    debug_printf(&debug_nm_so_sr_trace, "[%s] " fmt ,__TBX_FUNCTION__  , ##args)

#define NM_SO_SR_TRACE_LEVEL(level, fmt, args...) \
    debug_printfl(&debug_nm_so_sr_trace, level, "[%s] " fmt ,__TBX_FUNCTION__  , ##args)

#define NM_SO_SR_LOG_IN() \
    debug_printf(&debug_nm_so_sr_log, "%s, : -->\n", __TBX_FUNCTION__)

#define NM_SO_SR_LOG_OUT() \
    debug_printf(&debug_nm_so_sr_log, "%s, : <--\n", __TBX_FUNCTION__)

void nm_so_debug_init(int* argc, char** argv, int debug_flags);
void nm_so_sr_debug_init(int* argc, char** argv, int debug_flags);
