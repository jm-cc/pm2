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
 * leo_objects.c
 * =============
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "leonie.h"

p_leo_settings_t
leo_settings_init(void)
{
  p_leo_settings_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(leo_settings_t));

  object->gdb_mode    = tbx_false;
  object->xterm_mode  = tbx_true;
  object->log_mode    = tbx_false;
  object->pause_mode  = tbx_true;
  object->smp_mode    = tbx_false;

#ifdef LEO_EXPORT_MODE
  object->export_mode = tbx_false;
#endif // LEO_EXPORT_MODE
  LOG_OUT();

  return object;
}

p_leo_process_specific_t
leo_process_specific_init(void)
{
  p_leo_process_specific_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(leo_process_specific_t));

  object->client                   = ntbx_client_cons();
  object->current_loader_priority  = leo_loader_priority_undefined;
  LOG_OUT();

  return object;
}

p_ntbx_server_t
leo_net_server_init(void)
{
  p_ntbx_server_t object = NULL;

  LOG_IN();
  object = ntbx_server_cons();
  ntbx_tcp_server_init(object);
  LOG_OUT();

  return object;
}

p_tbx_htable_t
leo_htable_init(void)
{
  p_tbx_htable_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(tbx_htable_t));
  tbx_htable_init(object, 0);
  LOG_OUT();

  return object;
}

p_leo_directory_t
leo_directory_init(void)
{
  p_leo_directory_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(leo_directory_t));

  object->process_slist   = tbx_slist_nil();
  object->node_htable     = leo_htable_init();
  object->node_slist      = tbx_slist_nil();
  object->driver_htable   = leo_htable_init();
  object->driver_slist    = tbx_slist_nil();
  object->channel_htable  = leo_htable_init();
  object->channel_slist   = tbx_slist_nil();
  object->fchannel_htable = leo_htable_init();
  object->fchannel_slist  = tbx_slist_nil();
  object->vchannel_htable = leo_htable_init();
  object->vchannel_slist  = tbx_slist_nil();
  object->xchannel_htable = leo_htable_init();
  object->xchannel_slist  = tbx_slist_nil();
  LOG_OUT();

  return object;
}

p_leo_dir_adapter_t
leo_dir_adapter_init(void)
{
  p_leo_dir_adapter_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(leo_dir_adapter_t));
  LOG_OUT();

  return object;
}

p_leo_dir_driver_process_specific_t
leo_dir_driver_process_specific_init(void)
{
  p_leo_dir_driver_process_specific_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(leo_dir_driver_process_specific_t));

  object->adapter_htable = leo_htable_init();
  object->adapter_slist  = tbx_slist_nil();
  LOG_OUT();

  return object;
}

p_leo_dir_node_t
leo_dir_node_init(void)
{
  p_leo_dir_node_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(leo_dir_node_t));

  object->pc = ntbx_pc_cons();
  LOG_OUT();

  return object;
}

p_leo_dir_driver_t
leo_dir_driver_init(void)
{
  p_leo_dir_driver_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(leo_dir_driver_t));

  object->pc = ntbx_pc_cons();
  LOG_OUT();

  return object;
}

p_leo_dir_channel_process_specific_t
leo_dir_channel_process_specific_init(void)
{
  p_leo_dir_channel_process_specific_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(leo_dir_channel_process_specific_t));
  LOG_OUT();

  return object;
}

p_leo_dir_channel_common_process_specific_t
leo_dir_channel_common_process_specific_init(void)
{
  p_leo_dir_channel_common_process_specific_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(leo_dir_channel_common_process_specific_t));

  object->in_connection_parameter_darray  = tbx_darray_init();
  object->out_connection_parameter_darray = tbx_darray_init();
  LOG_OUT();

  return object;
}

p_leo_dir_channel_common_t
leo_dir_channel_common_init(void)
{
  p_leo_dir_channel_common_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(leo_dir_channel_common_t));

  object->pc = ntbx_pc_cons();
  LOG_OUT();

  return object;
}

p_leo_dir_channel_t
leo_dir_channel_init(void)
{
  p_leo_dir_channel_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(leo_dir_channel_t));

  object->pc     = ntbx_pc_cons();
  object->public = tbx_true;
  object->common = leo_dir_channel_common_init();
  LOG_OUT();

  return object;
}

p_leo_dir_fchannel_t
leo_dir_fchannel_init(void)
{
  p_leo_dir_fchannel_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(leo_dir_fchannel_t));

  object->common = leo_dir_channel_common_init();
  LOG_OUT();

  return object;
}

p_leo_dir_vchannel_process_routing_table_t
leo_dir_vchannel_process_routing_table_init(void)
{
  p_leo_dir_vchannel_process_routing_table_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(leo_dir_vchannel_process_routing_table_t));

  object->destination_rank = -1;
  LOG_OUT();

  return object;
}

p_leo_dir_vchannel_process_specific_t
leo_dir_vchannel_process_specific_init(void)
{
  p_leo_dir_vchannel_process_specific_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(leo_dir_vchannel_process_specific_t));

  object->pc = ntbx_pc_cons();
  LOG_OUT();

  return object;
}

p_leo_dir_vchannel_t
leo_dir_vchannel_init(void)
{
  p_leo_dir_vchannel_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(leo_dir_vchannel_t));

  object->dir_channel_slist  = tbx_slist_nil();
  object->dir_fchannel_slist = tbx_slist_nil();
  object->pc                 = ntbx_pc_cons();
  LOG_OUT();

  return object;
}

p_leo_dir_xchannel_process_routing_table_t
leo_dir_xchannel_process_routing_table_init(void)
{
  p_leo_dir_xchannel_process_routing_table_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(leo_dir_xchannel_process_routing_table_t));

  object->destination_rank = -1;
  LOG_OUT();

  return object;
}

p_leo_dir_xchannel_process_specific_t
leo_dir_xchannel_process_specific_init(void)
{
  p_leo_dir_xchannel_process_specific_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(leo_dir_xchannel_process_specific_t));

  object->pc = ntbx_pc_cons();
  LOG_OUT();

  return object;
}

p_leo_dir_xchannel_t
leo_dir_xchannel_init(void)
{
  p_leo_dir_xchannel_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(leo_dir_xchannel_t));

  object->dir_channel_slist = tbx_slist_nil();
  object->pc                = ntbx_pc_cons();
  LOG_OUT();

  return object;
}

p_leo_networks_t
leo_networks_init(void)
{
  p_leo_networks_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(leo_networks_t));

  object->htable = leo_htable_init();
  object->slist  = tbx_slist_nil();
  LOG_OUT();

  return object;
}

p_leo_spawn_group_t
leo_spawn_group_init(void)
{
  p_leo_spawn_group_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(leo_spawn_group_t));
  object->process_slist = tbx_slist_nil();
  LOG_OUT();

  return object;
}

p_leo_spawn_groups_t
leo_spawn_groups_init(void)
{
  p_leo_spawn_groups_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(leo_spawn_groups_t));

  object->htable = leo_htable_init();
  object->slist  = tbx_slist_nil();
  LOG_OUT();

  return object;
}

p_leo_loader_t
leo_loader_init(void)
{
  p_leo_loader_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(leo_loader_t));
  LOG_OUT();

  return object;
}

p_leonie_t
leonie_init(void)
{
  p_leonie_t object = NULL;

  LOG_IN();
  object = TBX_CALLOC(1, sizeof(leonie_t));
  LOG_OUT();

  return object;
}
