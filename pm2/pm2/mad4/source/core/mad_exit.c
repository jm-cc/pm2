
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
 * Mad_exit.c
 * ==========
 */
/* #define TIMING */
#include "madeleine.h"

void
mad_leonie_sync(p_mad_madeleine_t madeleine)
{
  p_mad_session_t session = NULL;
  p_ntbx_client_t client  = NULL;
  int             data    =    0;

  LOG_IN();
  TRACE("Termination sync");  
  session = madeleine->session;
  client  = session->leonie_link;

  mad_ntbx_send_int(client, mad_leo_command_end);
  data = mad_ntbx_receive_int(client);
  if (data != 1)
    FAILURE("synchronization error");

  LOG_OUT();
}

void
mad_leonie_link_exit(p_mad_madeleine_t madeleine)
{
  p_mad_session_t session = NULL;
  p_ntbx_client_t client  = NULL;

  LOG_IN();
  session        = madeleine->session;
  client         = session->leonie_link;
  
  ntbx_tcp_client_disconnect(client);
  ntbx_client_dest(client);
  session->leonie_link = NULL;
  LOG_OUT();
}

void
mad_object_exit(p_mad_madeleine_t madeleine TBX_UNUSED)
{
  LOG_IN();
#warning unimplemented
  LOG_OUT();
}
