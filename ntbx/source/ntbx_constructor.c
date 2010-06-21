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
 * Ntbx_constructor.c
 * ==================
 */

#include "tbx.h"
#include "ntbx.h"

p_ntbx_client_t
ntbx_client_cons(void)
{
  p_ntbx_client_t object = NULL;

  PM2_LOG_IN();
  object = TBX_CALLOC(1, sizeof(ntbx_client_t));

  object->local_host	= NULL;
  object->local_alias	= tbx_slist_nil();
  object->remote_host	= NULL;
  object->remote_alias	= tbx_slist_nil();
  object->specific	= NULL;
  object->read_ok	= tbx_false;
  object->write_ok	= tbx_false;
  object->read_rq	= 0;
  object->read_rq_flag	= tbx_false;
  object->write_rq	= 0;
  object->write_rq_flag	= tbx_false;
  PM2_LOG_OUT();

  return object;
}

p_ntbx_server_t
ntbx_server_cons(void)
{
  p_ntbx_server_t object = NULL;

  PM2_LOG_IN();
  object = TBX_CALLOC(1, sizeof(ntbx_server_t));

  object->local_host              = NULL;
  object->local_alias             = tbx_slist_nil();
  object->connection_data.data[0] = '\0';
  object->specific                = NULL;
  PM2_LOG_OUT();

  return object;
}

p_ntbx_process_info_t
ntbx_process_info_cons(void)
{
  p_ntbx_process_info_t object = NULL;

  PM2_LOG_IN();
  object = TBX_CALLOC(1, sizeof(ntbx_process_info_t));

  object->local_rank =   -1;
  object->process    = NULL;
  object->specific   = NULL;
  PM2_LOG_OUT();

  return object;
}

p_ntbx_process_container_t
ntbx_pc_cons(void)
{
  p_ntbx_process_container_t object = NULL;

  PM2_LOG_IN();
  object = TBX_CALLOC(1, sizeof(ntbx_process_container_t));

  object->local_array_size  = 0;
  object->local_index       = NULL;
  object->global_array_size = 0;
  object->global_index      = NULL;
  object->count             = 0;
  PM2_LOG_OUT();

  return object;
}

p_ntbx_process_t
ntbx_process_cons(void)
{
  p_ntbx_process_t object = NULL;

  PM2_LOG_IN();
  object = TBX_CALLOC(1, sizeof(ntbx_process_t));

  object->global_rank =   -1;
  object->pid         =   -1;
  object->ref         = tbx_htable_empty_table();
  object->specific    = NULL;
  PM2_LOG_OUT();

  return object;
}

p_ntbx_topology_element_t
ntbx_topology_element_cons(void)
{
  p_ntbx_topology_element_t object = NULL;

  PM2_LOG_IN();
  object = TBX_CALLOC(1, sizeof(ntbx_topology_element_t));

  object->specific = NULL;
  PM2_LOG_OUT();

  return object;
}

p_ntbx_topology_table_t
ntbx_topology_table_cons(void)
{
  p_ntbx_topology_table_t object = NULL;

  PM2_LOG_IN();
  object = TBX_CALLOC(1, sizeof(ntbx_topology_table_t));

  object->table = NULL;
  object->size =    0;
  PM2_LOG_OUT();

  return object;
}
