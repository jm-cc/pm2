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

#include "pm2_common.h"
#include "nm_so_debug.h"

debug_type_t debug_nm_so_trace=NEW_DEBUG_TYPE("NM_SO: ", "nm_so_trace");
debug_type_t debug_nm_so_sr_trace=NEW_DEBUG_TYPE("NM_SO_SR: ", "nm_so_sr_trace");
debug_type_t debug_nm_so_sr_log=NEW_DEBUG_TYPE("NM_SO_SR_LOG: ", "nm_so_sr_log");

void nm_so_debug_init(int* argc TBX_UNUSED, char** argv TBX_UNUSED, int debug_flags TBX_UNUSED)
{
  pm2debug_register(&debug_nm_so_trace);
  pm2debug_init_ext(argc, argv, debug_flags);
}

void nm_so_sr_debug_init(int* argc TBX_UNUSED, char** argv TBX_UNUSED, int debug_flags TBX_UNUSED)
{
  pm2debug_register(&debug_nm_so_sr_trace);
  pm2debug_register(&debug_nm_so_sr_log);
  pm2debug_init_ext(argc, argv, debug_flags);
}
