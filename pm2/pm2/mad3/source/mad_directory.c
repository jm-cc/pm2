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
 * Mad_directory.c
 * ===============
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#ifdef PM2
#include <sys/wait.h>
#endif /* PM2 */

#include "madeleine.h"

/*
 *  Functions
 * ____________
 */
static
void
mad_dir_sync(const char *s)
{
  char *command_string = NULL;

  LOG_IN();
  command_string = mad_leonie_receive_string();

  if (!tbx_streq(command_string, s))
    FAILURE("synchronisation error");

  TBX_FREE(command_string);
  command_string = NULL;
  LOG_OUT();
}

static
void
mad_dir_process_get_process(p_mad_madeleine_t madeleine)
{
  p_mad_directory_t    dir         = NULL;
  ntbx_process_grank_t global_rank =   -1;
  p_ntbx_process_t     process     = NULL;

  LOG_IN();
  dir = madeleine->dir;

  global_rank = mad_leonie_receive_int();
  TRACE_VAL("Received process", global_rank);

  process = ntbx_process_cons();
  process->global_rank = global_rank;

  tbx_darray_expand_and_set(dir->process_darray, global_rank, process);
  tbx_slist_append(dir->process_slist, process);
  LOG_OUT();
}

static
void
mad_dir_process_get(p_mad_madeleine_t madeleine)
{
  int number = 0;

  LOG_IN();
  number = mad_leonie_receive_int();
  if (number <= 0)
    FAILURE("invalid number of processes");

  TRACE_VAL("Number of processes", number);

  while (number--)
    {
      mad_dir_process_get_process(madeleine);
    }

  mad_dir_sync("end{processes}");
  LOG_OUT();
}

static
tbx_bool_t
mad_dir_node_get_process(p_mad_directory_t dir,
                         p_mad_dir_node_t  dir_node)
{
  p_ntbx_process_t     process          = NULL;
  ntbx_process_grank_t node_global_rank =   -1;
  ntbx_process_lrank_t node_local_rank  =   -1;

  LOG_IN();
  node_global_rank = mad_leonie_receive_int();
  if (node_global_rank == -1)
    {
      LOG_OUT();

      return tbx_false;
    }

  node_local_rank = mad_leonie_receive_int();
  process = tbx_darray_get(dir->process_darray, node_global_rank);

  ntbx_pc_add(dir_node->pc, process, node_local_rank,
              dir_node, "node", NULL);
  LOG_OUT();

  return tbx_true;
}

static
void
mad_dir_node_get_node(p_mad_madeleine_t madeleine)
{
  p_mad_directory_t dir      = NULL;
  p_mad_dir_node_t  dir_node = NULL;

  LOG_IN();
  dir = madeleine->dir;

  dir_node = mad_dir_node_cons();
  dir_node->name = mad_leonie_receive_string();
  TRACE_STR("Node name", dir_node->name);

#ifdef LEO_IP
  {
    char *ip_str, *dummy;

    ip_str = mad_ntbx_receive_string(client);
    dir_node->ip = strtoul(ip_str, &dummy, 16);
    TRACE_STR("Node IP", ip_str);
    TBX_FREE(ip_str);
  }
#endif // LEO_IP

  tbx_htable_add(dir->node_htable, dir_node->name, dir_node);
  dir_node->id = tbx_slist_get_length(dir->node_slist);
  tbx_slist_append(dir->node_slist, dir_node);

  while (mad_dir_node_get_process(dir, dir_node))
    ;

  LOG_OUT();
}

static
void
mad_dir_node_get(p_mad_madeleine_t madeleine)
{
  int number = 0;

  LOG_IN();
  number = mad_leonie_receive_int();

  if (number <= 0)
    FAILURE("invalid number of nodes");

  TRACE_VAL("Number of nodes", number);

  while (number--)
    {
      mad_dir_node_get_node(madeleine);
    }

  mad_dir_sync("end{nodes}");
  LOG_OUT();
}

static
char *
mad_dir_build_reference_name(const char *type, const char *name)
{
  char *reference_name = NULL;

  LOG_IN();
  reference_name = TBX_MALLOC(strlen(type) + strlen(name) + 2);
  sprintf(reference_name, "%s:%s", type, name);
  LOG_OUT();

  return reference_name;
}

static
p_mad_dir_driver_process_specific_t
mad_dir_adapter_get(void)
{
  p_mad_dir_driver_process_specific_t pi_specific        = NULL;
  p_tbx_slist_t                       adapter_slist      = NULL;
  p_tbx_htable_t                      adapter_htable     = NULL;
  int                                 adapter_number = 0;

  LOG_IN();
  pi_specific    = mad_dir_driver_process_specific_cons();
  adapter_slist  = pi_specific->adapter_slist;
  adapter_htable = pi_specific->adapter_htable;

  adapter_number = mad_leonie_receive_int();

  while (adapter_number--)
    {
      p_mad_dir_adapter_t adapter = NULL;

      adapter            = mad_dir_adapter_cons();
      adapter->name      = mad_leonie_receive_string();
      adapter->selector  = mad_leonie_receive_string();
      TRACE_STR("- name", adapter->name);

      adapter->id = tbx_slist_get_length(adapter_slist);
      tbx_slist_append(adapter_slist, adapter);
      tbx_htable_add(adapter_htable, adapter->name, adapter);
    }
  LOG_OUT();

  return pi_specific;
}

static
tbx_bool_t
mad_dir_driver_process_get(p_tbx_darray_t      process_darray,
                           p_mad_dir_driver_t  dir_driver,
                           char               *driver_reference_name)
{
  p_ntbx_process_t                    process            = NULL;
  p_mad_dir_driver_process_specific_t pi_specific        = NULL;
  ntbx_process_grank_t                driver_global_rank =   -1;
  ntbx_process_lrank_t                driver_local_rank  =   -1;

  LOG_IN();
  driver_global_rank = mad_leonie_receive_int();
  if (driver_global_rank == -1)
    return tbx_false;

  driver_local_rank = mad_leonie_receive_int();
  process     = tbx_darray_get(process_darray, driver_global_rank);
  pi_specific = mad_dir_adapter_get();

  ntbx_pc_add(dir_driver->pc, process, driver_local_rank,
              dir_driver, driver_reference_name, pi_specific);
  LOG_OUT();

  return tbx_true;
}

static
void
mad_dir_driver_get_driver(p_mad_madeleine_t madeleine)
{
  p_mad_directory_t   dir                   = NULL;
  p_mad_dir_driver_t  dir_driver            = NULL;
  char               *driver_reference_name = NULL;

  LOG_IN();
  dir = madeleine->dir;

  dir_driver = mad_dir_driver_cons();
  dir_driver->name = mad_leonie_receive_string();
  TRACE_STR("Driver name", dir_driver->name);

  driver_reference_name =
    mad_dir_build_reference_name("driver", dir_driver->name);

  tbx_htable_add(dir->driver_htable, dir_driver->name, dir_driver);
  dir_driver->id = tbx_slist_get_length(dir->driver_slist);
  tbx_slist_append(dir->driver_slist, dir_driver);

  while (mad_dir_driver_process_get(dir->process_darray,
                                    dir_driver,
                                    driver_reference_name))
    ;

  TBX_FREE(driver_reference_name);
  LOG_OUT();
}

static
void
mad_dir_driver_get(p_mad_madeleine_t madeleine)
{
  int number = 0;

  LOG_IN();
  number = mad_leonie_receive_int();

  if (number <= 0)
    FAILURE("invalid number of driver");

  TRACE_VAL("Number of driver", number);

  while (number--)
    {
      mad_dir_driver_get_driver(madeleine);
    }

  mad_dir_sync("end{drivers}");
  LOG_OUT();
}

static
tbx_bool_t
mad_dir_channel_get_adapters(p_mad_dir_channel_t  dir_channel,
                             p_tbx_darray_t       process_darray,
                             char                *channel_reference_name)
{
  p_ntbx_process_t                     process             = NULL;
  p_mad_dir_channel_process_specific_t cp_specific         = NULL;
  ntbx_process_grank_t                 channel_global_rank =   -1;
  ntbx_process_lrank_t                 channel_local_rank  =   -1;

  LOG_IN();
  channel_global_rank = mad_leonie_receive_int();
  if (channel_global_rank == -1)
    return tbx_false;

  channel_local_rank = mad_leonie_receive_int();
  process = tbx_darray_get(process_darray, channel_global_rank);

  cp_specific = mad_dir_channel_process_specific_cons();
  cp_specific->adapter_name = mad_leonie_receive_string();
  TRACE("local process %d: %s", channel_local_rank,
        cp_specific->adapter_name);

  ntbx_pc_add(dir_channel->pc, process, channel_local_rank,
              dir_channel, channel_reference_name, cp_specific);
  LOG_OUT();

  return tbx_true;
}

static
p_ntbx_topology_table_t
mad_dir_channel_ttable_build(p_ntbx_process_container_t pc)
{
  p_ntbx_topology_table_t ttable     = NULL;
  ntbx_process_lrank_t    l_rank_src =   -1;

  LOG_IN();
  ttable =
    ntbx_topology_table_init(pc, ntbx_topology_empty, NULL);

  if (ntbx_pc_first_local_rank(pc, &l_rank_src))
    {
      do
        {
          ntbx_process_lrank_t l_rank_dst = -1;

          ntbx_pc_first_local_rank(pc, &l_rank_dst);
          do
            {
              int value = 0;

              value = mad_leonie_receive_int();
              if (value)
                {
                  ntbx_topology_table_set(ttable,
                                          l_rank_src,
                                          l_rank_dst);
                  TRACE_CHAR('+');
                }
              else
                {
                  TRACE_CHAR(' ');
                }
            }
          while (ntbx_pc_next_local_rank(pc, &l_rank_dst));
          TRACE_CHAR('\n');
        }
      while (ntbx_pc_next_local_rank(pc, &l_rank_src));
    }
  LOG_OUT();

  return ttable;
}

static
void
mad_dir_channel_get_channel(p_mad_madeleine_t madeleine)
{
  p_mad_directory_t    dir                    = NULL;
  p_mad_dir_channel_t  dir_channel            = NULL;
  char                *channel_reference_name = NULL;
  char                *driver_name            = NULL;

  LOG_IN();
  dir = madeleine->dir;

  dir_channel       = mad_dir_channel_cons();
  dir_channel->name = mad_leonie_receive_string();
  TRACE_STR("Channel name", dir_channel->name);

  channel_reference_name =
    mad_dir_build_reference_name("channel", dir_channel->name);

  dir_channel->not_private = mad_leonie_receive_unsigned_int();
  if (dir_channel->not_private)
    {
      tbx_slist_append(madeleine->public_channel_slist, dir_channel->name);
    }

  driver_name = mad_leonie_receive_string();
  dir_channel->driver = tbx_htable_get(dir->driver_htable, driver_name);
  if (!dir_channel->driver)
    FAILURE("driver not found");

  TRACE_STR("Channel driver", dir_channel->driver->name);
  TBX_FREE(driver_name);

  tbx_htable_add(dir->channel_htable, dir_channel->name, dir_channel);
  dir_channel->id = tbx_slist_get_length(dir->channel_slist);
  tbx_slist_append(dir->channel_slist, dir_channel);

  TRACE("Adapters:");
  while (mad_dir_channel_get_adapters(dir_channel,
                                      dir->process_darray,
                                      channel_reference_name))
    ;

  dir_channel->ttable = mad_dir_channel_ttable_build(dir_channel->pc);
  TBX_FREE(channel_reference_name);
  LOG_OUT();
}

static
void
mad_dir_channel_get(p_mad_madeleine_t madeleine)
{
  p_mad_directory_t dir    = NULL;
  int               number =    0;

  LOG_IN();
  dir    = madeleine->dir;

  number = mad_leonie_receive_int();

  if (number <= 0)
    FAILURE("invalid number of channels");

  TRACE_VAL("Number of channels", number);

  while (number--)
    {
      mad_dir_channel_get_channel(madeleine);
    }

  mad_dir_sync("end{channels}");
  LOG_OUT();
}

static
void
mad_dir_fchannel_get(p_mad_madeleine_t madeleine)
{
  p_mad_directory_t dir             = NULL;
  int               number          =    0;

  LOG_IN();
  dir    = madeleine->dir;

  number = mad_leonie_receive_int();

  if (number < 0)
    FAILURE("invalid number of forwarding channels");

  TRACE_VAL("Number of forwarding channels", number);

  while (number--)
    {
      p_mad_dir_fchannel_t  dir_fchannel            = NULL;

      dir_fchannel               = mad_dir_fchannel_cons();
      dir_fchannel->name         = mad_leonie_receive_string();
      dir_fchannel->channel_name = mad_leonie_receive_string();
      TRACE_STR("Forwarding channel name", dir_fchannel->name);

      tbx_htable_add(dir->fchannel_htable, dir_fchannel->name, dir_fchannel);
      dir_fchannel->id = tbx_slist_get_length(dir->fchannel_slist);
      tbx_slist_append(dir->fchannel_slist, dir_fchannel);
    }

  mad_dir_sync("end{fchannels}");
  LOG_OUT();
}

static
void
mad_dir_vchannel_receive_channel_list(p_mad_dir_vchannel_t dir_vchannel,
                                      p_tbx_htable_t       channel_htable)
{
  p_tbx_slist_t dir_channel_slist     = NULL;
  int           dir_channel_slist_len =    0;

  LOG_IN();
  dir_channel_slist     = dir_vchannel->dir_channel_slist;
  dir_channel_slist_len = mad_leonie_receive_int();

  TRACE("Supporting channels:");
  do
    {
      p_mad_dir_channel_t  dir_channel      = NULL;
      char                *dir_channel_name = NULL;

      dir_channel_name = mad_leonie_receive_string();
      dir_channel = tbx_htable_get(channel_htable, dir_channel_name);
      if (!dir_channel)
        FAILURE("channel not found");

      TRACE_STR("- Channel", dir_channel->name);

      tbx_slist_append(dir_channel_slist, dir_channel);
      TBX_FREE(dir_channel_name);
    }
  while (--dir_channel_slist_len);
  LOG_OUT();
}

static
void
mad_dir_vchannel_receive_fchannel_list(p_mad_dir_vchannel_t dir_vchannel,
                                      p_tbx_htable_t       fchannel_htable)
{
  p_tbx_slist_t dir_fchannel_slist     = NULL;
  int           dir_fchannel_slist_len =    0;

  LOG_IN();
  dir_fchannel_slist     = dir_vchannel->dir_fchannel_slist;
  dir_fchannel_slist_len = mad_leonie_receive_int();

  TRACE("Supporting forwarding channels:");
  do
    {
      p_mad_dir_fchannel_t  dir_fchannel      = NULL;
      char                 *dir_fchannel_name = NULL;

      dir_fchannel_name = mad_leonie_receive_string();
      dir_fchannel = tbx_htable_get(fchannel_htable, dir_fchannel_name);
      if (!dir_fchannel)
        FAILURE("channel not found");

      TRACE_STR("- Forwarding channel", dir_fchannel->name);

      tbx_slist_append(dir_fchannel_slist, dir_fchannel);
      TBX_FREE(dir_fchannel_name);
    }
  while (--dir_fchannel_slist_len);
  LOG_OUT();
}

static
tbx_bool_t
mad_dir_vxchannel_get_rtable(p_ntbx_process_t            process,
                           p_tbx_darray_t              process_darray,
                           p_ntbx_process_container_t  ppc,
                           char                       *_ref_name)
{
  p_ntbx_process_t                           pprocess   = NULL;
  ntbx_process_grank_t                       g_rank_dst =   -1;
  p_mad_dir_vxchannel_process_routing_table_t  rtable     = NULL;
  char                                      *ref_name   = NULL;

  LOG_IN();
  g_rank_dst = mad_leonie_receive_int();
  if (g_rank_dst == -1)
    {
      LOG_OUT();

      return tbx_false;
    }

  pprocess = tbx_darray_get(process_darray, g_rank_dst);

  rtable = mad_dir_vxchannel_process_routing_table_cons();
  rtable->channel_name     = mad_leonie_receive_string();
  rtable->destination_rank = mad_leonie_receive_int();

  ref_name = tbx_strdup(_ref_name);
  ntbx_pc_add(ppc, pprocess, g_rank_dst, process, ref_name, rtable);
  LOG_OUT();

  return tbx_true;
}


static
tbx_bool_t
mad_dir_vchannel_get_routing_table(p_mad_directory_t     dir,
                                   p_mad_dir_vchannel_t  dir_vchannel,
                                   char                 *vchannel_reference_name)
{
  p_tbx_darray_t                          process_darray  = NULL;
  p_ntbx_process_t                        process         = NULL;
  ntbx_process_grank_t                    g_rank_src      =   -1;
  p_mad_dir_vxchannel_process_specific_t  pi_specific     = NULL;
  p_ntbx_process_container_t              ppc             = NULL;
  char                                   *ref_name        = NULL;

  LOG_IN();
  process_darray = dir->process_darray;

  g_rank_src = mad_leonie_receive_int();
  if (g_rank_src == -1)
    {
      LOG_OUT();

      return tbx_false;
    }

  process = tbx_darray_get(process_darray, g_rank_src);

  pi_specific = mad_dir_vxchannel_process_specific_cons();
  ppc = pi_specific->pc;

  {
    size_t len = 0;

    len = strlen(dir_vchannel->name) + 8;
    ref_name = TBX_MALLOC(len);

    sprintf(ref_name, "%s:%d", dir_vchannel->name,
            g_rank_src);
  }

  while (mad_dir_vxchannel_get_rtable(process, process_darray, ppc, ref_name))
    ;

  TBX_FREE(ref_name);
  ref_name = NULL;

  ntbx_pc_add(dir_vchannel->pc, process, g_rank_src,
              dir_vchannel, vchannel_reference_name, pi_specific);

  LOG_OUT();

  return tbx_true;
}

static
void
mad_dir_vchannel_get_data(p_mad_madeleine_t madeleine)
{
  p_mad_directory_t     dir                     = NULL;
  p_mad_dir_vchannel_t  dir_vchannel            = NULL;
  char                 *vchannel_reference_name = NULL;

  LOG_IN();
  dir = madeleine->dir;
  dir_vchannel = mad_dir_vchannel_cons();
  dir_vchannel->name = mad_leonie_receive_string();
  TRACE_STR("Virtual channel name", dir_vchannel->name);

  tbx_slist_append(madeleine->public_channel_slist, dir_vchannel->name);

  vchannel_reference_name =
    mad_dir_build_reference_name("vchannel", dir_vchannel->name);


  mad_dir_vchannel_receive_channel_list(dir_vchannel, dir->channel_htable);
  mad_dir_vchannel_receive_fchannel_list(dir_vchannel, dir->fchannel_htable);

  tbx_htable_add(dir->vchannel_htable, dir_vchannel->name, dir_vchannel);
  dir_vchannel->id = tbx_slist_get_length(dir->vchannel_slist);
  tbx_slist_append(dir->vchannel_slist, dir_vchannel);

  TRACE("Virtual channel routing table");
  while (mad_dir_vchannel_get_routing_table(dir, dir_vchannel,
                                            vchannel_reference_name))
    ;

  TBX_FREE(vchannel_reference_name);
  LOG_OUT();
}

static
void
mad_dir_vchannel_get(p_mad_madeleine_t madeleine)
{
  int number = 0;

  LOG_IN();
  number = mad_leonie_receive_int();

  if (number < 0)
    FAILURE("invalid number of virtual channels");

  TRACE_VAL("Number of virtual channels", number);

  while (number--)
    {
      mad_dir_vchannel_get_data(madeleine);
    }

  mad_dir_sync("end{vchannels}");
  LOG_OUT();
}

static
void
mad_dir_xchannel_receive_channel_list(p_mad_dir_xchannel_t dir_xchannel,
                                      p_tbx_htable_t       channel_htable)
{
  p_tbx_slist_t dir_channel_slist     = NULL;
  int           dir_channel_slist_len =    0;

  LOG_IN();
  dir_channel_slist     = dir_xchannel->dir_channel_slist;
  dir_channel_slist_len = mad_leonie_receive_int();

  TRACE("Supporting channels:");
  do
    {
      p_mad_dir_channel_t  dir_channel      = NULL;
      char                *dir_channel_name = NULL;

      dir_channel_name = mad_leonie_receive_string();
      dir_channel = tbx_htable_get(channel_htable,
                                   dir_channel_name);
      if (!dir_channel)
        FAILURE("channel not found");

      TRACE_STR("- Channel", dir_channel->name);

      tbx_slist_append(dir_channel_slist, dir_channel);
      TBX_FREE(dir_channel_name);
    }
  while (--dir_channel_slist_len);
  LOG_OUT();
}

static
tbx_bool_t
mad_dir_xchannel_get_routing_table(p_mad_dir_xchannel_t dir_xchannel,
                                   p_tbx_darray_t       process_darray)
{
  p_ntbx_process_t                        process     = NULL;
  ntbx_process_grank_t                    g_rank_src  =   -1;
  p_mad_dir_vxchannel_process_specific_t  pi_specific = NULL;
  p_ntbx_process_container_t              ppc         = NULL;
  char                                   *ref_name    = NULL;

  LOG_IN();
  g_rank_src = mad_leonie_receive_int();
  if (g_rank_src == -1){

    LOG_OUT();

    return tbx_false;
  }

  process = tbx_darray_get(process_darray, g_rank_src);

  pi_specific = mad_dir_vxchannel_process_specific_cons();
  ppc = pi_specific->pc;

  {
    size_t len = 0;

    len = strlen(dir_xchannel->name) + 8;
    ref_name = TBX_MALLOC(len);

    sprintf(ref_name, "%s:%d", dir_xchannel->name, g_rank_src);
  }

  while (mad_dir_vxchannel_get_rtable(process, process_darray, ppc, ref_name))
    ;

  TBX_FREE(ref_name);
  ref_name = NULL;

  ntbx_pc_add(dir_xchannel->pc, process, g_rank_src,
              dir_xchannel, dir_xchannel->name, pi_specific);
  LOG_OUT();

  return tbx_true;
}

static
void
mad_dir_xchannel_get_data(p_mad_madeleine_t madeleine)
{
  p_mad_directory_t     dir                        = NULL;
  p_mad_dir_xchannel_t  dir_xchannel               = NULL;
  char                 *xchannel_reference_name    = NULL;
  p_tbx_slist_t         sub_channel_name_slist     = NULL;
  int                   sub_channel_name_slist_len =    0;

  LOG_IN();
  dir = madeleine->dir;

  dir_xchannel = mad_dir_xchannel_cons();
  dir_xchannel->name = mad_leonie_receive_string();
  TRACE_STR("Virtual channel name", dir_xchannel->name);

  tbx_slist_append(madeleine->public_channel_slist, dir_xchannel->name);

  xchannel_reference_name =
    mad_dir_build_reference_name("xchannel", dir_xchannel->name);
  mad_dir_xchannel_receive_channel_list(dir_xchannel, dir->channel_htable);

  sub_channel_name_slist     = dir_xchannel->sub_channel_name_slist;
  sub_channel_name_slist_len = mad_leonie_receive_int();

  if (sub_channel_name_slist_len)
    {
      do
        {
          char *name = NULL;

          name = mad_leonie_receive_string();
          tbx_slist_append(sub_channel_name_slist, name);
        }
      while (--sub_channel_name_slist_len);
    }

  tbx_htable_add(dir->xchannel_htable, dir_xchannel->name, dir_xchannel);
  dir_xchannel->id = tbx_slist_get_length(dir->xchannel_slist);
  tbx_slist_append(dir->xchannel_slist, dir_xchannel);

  TRACE("Virtual channel routing table");
  while (mad_dir_xchannel_get_routing_table(dir_xchannel, dir->process_darray))
    ;

  TBX_FREE(xchannel_reference_name);
  LOG_OUT();
}

static
void
mad_dir_xchannel_get(p_mad_madeleine_t madeleine)
{
  int number =    0;

  LOG_IN();

  number = mad_leonie_receive_int();

  if (number < 0)
    FAILURE("invalid number of virtual channels");

  TRACE_VAL("Number of virtual channels", number);

  while (number--)
    {
      mad_dir_xchannel_get_data(madeleine);
    }

  mad_dir_sync("end{xchannels}");
  LOG_OUT();
}

void
mad_dir_directory_get(p_mad_madeleine_t madeleine)
{
  LOG_IN();
  TRACE("Receiving directory");
  mad_dir_process_get(madeleine);
  mad_dir_node_get(madeleine);
  mad_dir_driver_get(madeleine);
  mad_dir_channel_get(madeleine);
  mad_dir_fchannel_get(madeleine);
  mad_dir_vchannel_get(madeleine);
  mad_dir_xchannel_get(madeleine);
  mad_dir_sync("end{directory}");
  LOG_OUT();
}

static
void
channel_common_cleanup(p_mad_dir_channel_common_t cc)
{
  ntbx_process_grank_t g = 0;
  p_ntbx_process_container_t pc = NULL;

  LOG_IN();
  pc = cc->pc;
  if (ntbx_pc_first_global_rank(pc, &g))
    {
      do
        {
          p_mad_dir_channel_common_process_specific_t ccps = NULL;

          ccps = ntbx_pc_get_global_specific(pc, g);
          if (ccps)
            {
              int   i   =    0;
              char *ptr = NULL;

              if (ccps->parameter)
                {
                  TBX_FREE(ccps->parameter);
                  ccps->parameter = NULL;
                }

              if ((ptr = tbx_darray_first_idx(ccps->out_connection_parameter_darray, &i)))
                {
                  do
                    {
                      if (ptr)
                        {
                          char *ptr2 = NULL;

                          ptr2 = tbx_darray_expand_and_get(ccps->in_connection_parameter_darray, i);
                          if (ptr2 == ptr) 
                            {
                              tbx_darray_set(ccps->in_connection_parameter_darray, i, NULL);
                            }

                          TBX_FREE(ptr);
                          tbx_darray_set(ccps->out_connection_parameter_darray, i, NULL);
                        }
                    }
                  while ((ptr = tbx_darray_next_idx(ccps->out_connection_parameter_darray, &i)));
                }

              if ((ptr = tbx_darray_first_idx(ccps->in_connection_parameter_darray, &i)))
                {
                  do
                    {
                      if (ptr)
                        {
                          TBX_FREE(ptr);
                          tbx_darray_set(ccps->in_connection_parameter_darray, i, NULL);
                        }
                    }
                  while ((ptr = tbx_darray_next_idx(ccps->in_connection_parameter_darray, &i)));
                }

              tbx_darray_free(ccps->in_connection_parameter_darray);
              ccps->in_connection_parameter_darray = NULL;

              tbx_darray_free(ccps->out_connection_parameter_darray);
              ccps->out_connection_parameter_darray = NULL;
            }
        }
      while (ntbx_pc_next_global_rank(pc, &g));
    }

  ntbx_pc_dest(pc, tbx_default_specific_dest);
  cc->pc = NULL;
  TBX_FREE(cc);
  LOG_OUT();
}

static
void
mad_dir_vchannel_cleanup(p_mad_madeleine_t madeleine)
{
  p_mad_directory_t  dir             = NULL;
  p_tbx_htable_t     vchannel_htable = NULL;
  p_tbx_slist_t      vchannel_slist  = NULL;

  LOG_IN();
  dir             = madeleine->dir;
  vchannel_htable = dir->vchannel_htable;
  vchannel_slist  = dir->vchannel_slist;

  while (!tbx_slist_is_nil(vchannel_slist))
    {
      p_mad_dir_vchannel_t       dir_vchannel   = NULL;
      p_tbx_slist_t              channel_slist  = NULL;
      p_tbx_slist_t              fchannel_slist = NULL;
      p_ntbx_process_container_t pc             = NULL;
      ntbx_process_grank_t       g_rank_src     =   -1;

      dir_vchannel   = tbx_slist_extract(vchannel_slist);
      tbx_htable_extract(vchannel_htable, dir_vchannel->name);

      channel_slist  = dir_vchannel->dir_channel_slist;
      fchannel_slist = dir_vchannel->dir_fchannel_slist;

      while (!tbx_slist_is_nil(channel_slist))
	{
	  tbx_slist_extract(channel_slist);
	}

      tbx_slist_free(channel_slist);
      channel_slist                   = NULL;
      dir_vchannel->dir_channel_slist = NULL;

      while (!tbx_slist_is_nil(fchannel_slist))
	{
	  tbx_slist_extract(fchannel_slist);
	}

      tbx_slist_free(fchannel_slist);
      fchannel_slist                   = NULL;
      dir_vchannel->dir_fchannel_slist = NULL;

      pc = dir_vchannel->pc;

      if (ntbx_pc_first_global_rank(pc, &g_rank_src))
	{
	  do
	    {
	      p_mad_dir_vxchannel_process_specific_t vps        = NULL;
	      p_ntbx_process_container_t            ppc        = NULL;
	      ntbx_process_grank_t                  g_rank_dst =   -1;

	      vps = ntbx_pc_get_global_specific(pc, g_rank_src);
	      ppc = vps->pc;

	      ntbx_pc_first_global_rank(ppc, &g_rank_dst);
	      do
		{
		  p_mad_dir_vxchannel_process_routing_table_t rtable = NULL;

		  rtable = ntbx_pc_get_global_specific(ppc, g_rank_dst);
		  TBX_FREE(rtable->channel_name);
		  rtable->channel_name = NULL;
		}
	      while (ntbx_pc_next_global_rank(ppc, &g_rank_dst));

	      ntbx_pc_dest(ppc, tbx_default_specific_dest);
	    }
	  while (ntbx_pc_next_global_rank(pc, &g_rank_src));
	}

      ntbx_pc_dest(pc, tbx_default_specific_dest);
      dir_vchannel->pc   = NULL;

      TBX_FREE(dir_vchannel->name);
      dir_vchannel->name = NULL;

      dir_vchannel->id = 0;

      channel_common_cleanup(dir_vchannel->common);
      dir_vchannel->common = NULL;
      TBX_FREE(dir_vchannel);
    }

  tbx_slist_free(dir->vchannel_slist);
  dir->vchannel_slist = NULL;

  tbx_htable_free(dir->vchannel_htable);
  dir->vchannel_htable = NULL;
  LOG_OUT();
}


static
void
mad_dir_xchannel_cleanup(p_mad_madeleine_t madeleine)
{
  p_mad_directory_t  dir             = NULL;
  p_tbx_htable_t     xchannel_htable = NULL;
  p_tbx_slist_t      xchannel_slist  = NULL;

  LOG_IN();
  dir             = madeleine->dir;
  xchannel_htable = dir->xchannel_htable;
  xchannel_slist  = dir->xchannel_slist;

  while (!tbx_slist_is_nil(xchannel_slist))
    {
      p_mad_dir_xchannel_t       dir_xchannel   = NULL;
      p_tbx_slist_t              channel_slist  = NULL;
      p_ntbx_process_container_t pc             = NULL;
      ntbx_process_grank_t       g_rank_src     =   -1;

      dir_xchannel   = tbx_slist_extract(xchannel_slist);
      tbx_htable_extract(xchannel_htable, dir_xchannel->name);

      channel_slist  = dir_xchannel->dir_channel_slist;

      while (!tbx_slist_is_nil(channel_slist))
	{
	  tbx_slist_extract(channel_slist);
	}

      tbx_slist_free(channel_slist);
      channel_slist                   = NULL;
      dir_xchannel->dir_channel_slist = NULL;

      pc = dir_xchannel->pc;

      if (ntbx_pc_first_global_rank(pc, &g_rank_src))
	{
	  do
	    {
	      p_mad_dir_vxchannel_process_specific_t vps        = NULL;
	      p_ntbx_process_container_t             ppc        = NULL;
	      ntbx_process_grank_t                   g_rank_dst =   -1;

	      vps = ntbx_pc_get_global_specific(pc, g_rank_src);
	      ppc = vps->pc;

	      ntbx_pc_first_global_rank(ppc, &g_rank_dst);
	      do
		{
		  p_mad_dir_vxchannel_process_routing_table_t rtable = NULL;

		  rtable = ntbx_pc_get_global_specific(ppc, g_rank_dst);
		  TBX_FREE(rtable->channel_name);
		  rtable->channel_name = NULL;
		}
	      while (ntbx_pc_next_global_rank(ppc, &g_rank_dst));

	      ntbx_pc_dest(ppc, tbx_default_specific_dest);
	    }
	  while (ntbx_pc_next_global_rank(pc, &g_rank_src));
	}

      ntbx_pc_dest(pc, tbx_default_specific_dest);
      dir_xchannel->pc   = NULL;

      TBX_FREE(dir_xchannel->name);
      dir_xchannel->name = NULL;

      dir_xchannel->id = 0;

      channel_common_cleanup(dir_xchannel->common);
      dir_xchannel->common = NULL;
      TBX_FREE(dir_xchannel);
    }

  tbx_slist_free(dir->xchannel_slist);
  dir->xchannel_slist = NULL;

  tbx_htable_free(dir->xchannel_htable);
  dir->xchannel_htable = NULL;
  LOG_OUT();
}

static
void
mad_dir_fchannel_cleanup(p_mad_madeleine_t madeleine)
{
  p_mad_directory_t dir             = NULL;
  p_tbx_htable_t    fchannel_htable = NULL;
  p_tbx_slist_t     fchannel_slist  = NULL;

  LOG_IN();
  dir             = madeleine->dir;
  fchannel_htable = dir->fchannel_htable;
  fchannel_slist  = dir->fchannel_slist;

  while (!tbx_slist_is_nil(fchannel_slist))
    {
      p_mad_dir_fchannel_t dir_fchannel = NULL;

      dir_fchannel = tbx_slist_extract(fchannel_slist);
      tbx_htable_extract(fchannel_htable, dir_fchannel->name);

      TBX_FREE(dir_fchannel->name);
      dir_fchannel->name = NULL;

      TBX_FREE(dir_fchannel->channel_name);
      dir_fchannel->channel_name = NULL;

      dir_fchannel->id = 0;

      channel_common_cleanup(dir_fchannel->common);
      dir_fchannel->common = NULL;
      TBX_FREE(dir_fchannel);
    }

  tbx_slist_free(dir->fchannel_slist);
  dir->fchannel_slist = NULL;

  tbx_htable_free(dir->fchannel_htable);
  dir->fchannel_htable = NULL;
  LOG_OUT();
}

static
void
mad_dir_channel_cleanup(p_mad_madeleine_t madeleine)
{
  p_mad_directory_t dir            = NULL;
  p_tbx_htable_t    channel_htable = NULL;
  p_tbx_slist_t     channel_slist  = NULL;

  LOG_IN();
  dir            = madeleine->dir;
  channel_htable = dir->channel_htable;
  channel_slist  = dir->channel_slist;

  while (!tbx_slist_is_nil(channel_slist))
    {
      p_mad_dir_channel_t        dir_channel = NULL;
      p_ntbx_process_container_t pc          = NULL;
      ntbx_process_lrank_t       l_rank      =   -1;

      dir_channel = tbx_slist_extract(channel_slist);
      tbx_htable_extract(channel_htable, dir_channel->name);

      ntbx_topology_table_exit(dir_channel->ttable);
      dir_channel->ttable = NULL;

      pc = dir_channel->pc;

      if (ntbx_pc_first_local_rank(pc, &l_rank))
	{
	  do
	    {
	      p_mad_dir_channel_process_specific_t cps = NULL;

	      cps = ntbx_pc_get_local_specific(pc, l_rank);

	      if (cps && cps->adapter_name)
		{
		  TBX_FREE(cps->adapter_name);
		  cps->adapter_name = NULL;
		}
	    }
	  while (ntbx_pc_next_global_rank(pc, &l_rank));
	}

      ntbx_pc_dest(dir_channel->pc, tbx_default_specific_dest);
      dir_channel->pc = NULL;

      TBX_FREE(dir_channel->name);
      dir_channel->name = NULL;

      dir_channel->id          = 0;
      dir_channel->driver      = NULL;
      dir_channel->not_private = tbx_false;

      channel_common_cleanup(dir_channel->common);
      dir_channel->common = NULL;
      TBX_FREE(dir_channel);
    }

  tbx_slist_free(dir->channel_slist);
  dir->channel_slist = NULL;

  tbx_htable_free(dir->channel_htable);
  dir->channel_htable = NULL;
  LOG_OUT();
}

static
void
mad_dir_driver_cleanup(p_mad_madeleine_t madeleine)
{
  p_mad_directory_t dir           = NULL;
  p_tbx_htable_t    driver_htable = NULL;
  p_tbx_slist_t     driver_slist  = NULL;

  LOG_IN();
  dir           = madeleine->dir;
  driver_htable = dir->driver_htable;
  driver_slist  = dir->driver_slist;

  while (!tbx_slist_is_nil(driver_slist))
    {
      p_mad_dir_driver_t         dir_driver = NULL;
      p_ntbx_process_container_t pc         = NULL;
      ntbx_process_lrank_t       l_rank     =   -1;

      dir_driver = tbx_slist_extract(driver_slist);
      tbx_htable_extract(driver_htable, dir_driver->name);

      pc = dir_driver->pc;

      if (ntbx_pc_first_local_rank(pc, &l_rank))
	{
	  do
	    {
	      p_mad_dir_driver_process_specific_t dps = NULL;

	      dps = ntbx_pc_get_local_specific(pc, l_rank);

	      if (dps)
		{
		  p_tbx_htable_t adapter_htable = NULL;
		  p_tbx_slist_t  adapter_slist  = NULL;

		  adapter_htable = dps->adapter_htable;
		  adapter_slist  = dps->adapter_slist;

		  while (!tbx_slist_is_nil(adapter_slist))
		    {
		      p_mad_dir_adapter_t dir_adapter = NULL;

		      dir_adapter = tbx_slist_extract(adapter_slist);
		      tbx_htable_extract(adapter_htable, dir_adapter->name);

		      dir_adapter->id = 0;
		      TBX_FREE(dir_adapter->name);
		      dir_adapter->name = NULL;

		      if (dir_adapter->selector)
			{
			  TBX_FREE(dir_adapter->selector);
			  dir_adapter->selector = NULL;
			}

		      if (dir_adapter->parameter)
			{
			  TBX_FREE(dir_adapter->parameter);
			  dir_adapter->parameter = NULL;
			}

		      dir_adapter->mtu = 0;

		      TBX_FREE(dir_adapter);
		    }

		  tbx_slist_free(adapter_slist);
		  dps->adapter_slist = NULL;

		  tbx_htable_free(adapter_htable);
		  dps->adapter_htable = NULL;
		}
	    }
	  while (ntbx_pc_next_global_rank(pc, &l_rank));
	}

      ntbx_pc_dest(dir_driver->pc, tbx_default_specific_dest);
      dir_driver->pc = NULL;

      TBX_FREE(dir_driver->name);
      dir_driver->name = NULL;

      dir_driver->id = 0;

      TBX_FREE(dir_driver);
    }

  tbx_slist_free(dir->driver_slist);
  dir->driver_slist = NULL;

  tbx_htable_free(dir->driver_htable);
  dir->driver_htable = NULL;
  LOG_OUT();
}

static
void
mad_dir_node_cleanup(p_mad_madeleine_t madeleine)
{
  p_mad_directory_t dir         = NULL;
  p_tbx_htable_t    node_htable = NULL;
  p_tbx_slist_t     node_slist  = NULL;

  LOG_IN();
  dir            = madeleine->dir;
  node_htable = dir->node_htable;
  node_slist  = dir->node_slist;

  while (!tbx_slist_is_nil(node_slist))
    {
      p_mad_dir_node_t dir_node = NULL;

      dir_node = tbx_slist_extract(node_slist);
      tbx_htable_extract(node_htable, dir_node->name);

      ntbx_pc_dest(dir_node->pc, tbx_default_specific_dest);
      dir_node->pc   = NULL;

      TBX_FREE(dir_node->name);
      dir_node->name = NULL;
      dir_node->id   =    0;

      TBX_FREE(dir_node);
    }

  tbx_slist_free(dir->node_slist);
  dir->node_slist = NULL;

  tbx_htable_free(dir->node_htable);
  dir->node_htable = NULL;
  LOG_OUT();
}

static
void
mad_dir_process_cleanup(p_mad_madeleine_t madeleine)
{
  p_mad_directory_t dir            = NULL;
  p_tbx_darray_t    process_darray = NULL;
  p_tbx_slist_t     process_slist  = NULL;

  LOG_IN();
  dir            = madeleine->dir;
  process_darray = dir->process_darray;
  process_slist  = dir->process_slist;

  while (!tbx_slist_is_nil(process_slist))
    {
      p_ntbx_process_t process = NULL;
      p_tbx_htable_t   ref     = NULL;
      p_tbx_slist_t    keys    = NULL;

      process = tbx_slist_extract(process_slist);
      ref     = process->ref;
      keys    = tbx_htable_get_key_slist(ref);

      while (!tbx_slist_is_nil(keys))
	{
	  char *key        = NULL;

	  key = tbx_slist_extract(keys);
	  tbx_htable_extract(ref, key);
	  TBX_FREE(key);
	}

      tbx_slist_free(keys);
      tbx_htable_free(ref);

      TBX_FREE(process);
    }

  tbx_slist_free(dir->process_slist);
  dir->process_slist = NULL;

  tbx_darray_free(dir->process_darray);
  dir->process_darray = NULL;
  LOG_OUT();
}

void
mad_directory_exit(p_mad_madeleine_t madeleine)
{
  LOG_IN();
  TRACE("Cleaning directory");
  mad_dir_sync("clean{directory}");

  mad_dir_vchannel_cleanup(madeleine);
  mad_dir_xchannel_cleanup(madeleine);
  mad_dir_fchannel_cleanup(madeleine);
  mad_dir_channel_cleanup(madeleine);
  mad_dir_driver_cleanup(madeleine);
  mad_dir_node_cleanup(madeleine);
  mad_dir_process_cleanup(madeleine);
  LOG_OUT();
}


/*
 * Local variables:
 *  c-basic-offset: 2
 *  c-hanging-braces-alist: '((defun-open before after) (class-open before after) (inline-open before after) (block-open before after) (brace-list-open) (brace-entry-open) (substatement-open before after) (block-close . c-snug-do-while) (extern-lang-open before after) (inexpr-class-open before after) (inexpr-class-close before))
 * End:
 */
