
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

/*
 * swann_types.h
 * -------------
 */

#ifndef SWANN_TYPES_H
#define SWANN_TYPES_H

typedef struct s_swann_settings
{
  char       *leonie_server_host_name;
  char       *leonie_server_port;
  tbx_bool_t  gdb_mode;
  tbx_bool_t  xterm_mode;
  tbx_bool_t  log_mode;
  tbx_bool_t  pause_mode;
  tbx_bool_t  smp_mode;
} swann_settings_t;

typedef struct s_swann_session
{
  p_ntbx_client_t      leonie_link;
  ntbx_process_grank_t process_rank;
} swann_session_t;

typedef struct s_swann
{
  p_swann_settings_t settings;
  p_swann_session_t  session;
} swann_t;

#endif /* SWANN_TYPES_H */
