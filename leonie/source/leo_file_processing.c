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
 * leo_file_processing.c
 * =====================
 *
 * - configuration file processing
 * - Madeleine's "directory" structure construction
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#define NEED_LEO_HELPERS
#include "leonie.h"

// build_network_host_table: builds a table of "normalized" host names from
// a list of 'unnormalized' host names. The normalization is performed using
// the 'ntbx_true_name' function.
#if 0
p_tbx_htable_t
build_network_host_htable(p_tbx_slist_t network_host_slist)
{
  p_tbx_htable_t host_htable = NULL;

  LOG_IN();
  host_htable = leo_htable_init();

  tbx_slist_ref_to_head(network_host_slist);
  do
    {
      p_leoparse_object_t  object    = NULL;
      char                *host_name = NULL;

      object = tbx_slist_ref_get(network_host_slist);

      {
	char *temp_name = NULL;

	temp_name = leoparse_get_id(object);
	TRACE_STR("expected name", temp_name);

	host_name = ntbx_true_name(temp_name);
	TRACE_STR("true name", host_name);
      }

      if (!tbx_htable_get(host_htable, host_name))
	{
	  tbx_htable_add(host_htable, host_name, host_name);
	}
    }
  while (tbx_slist_ref_forward(network_host_slist));

  LOG_OUT();

  return host_htable;
}
#endif

static
void
build_network_host_htable(char           *network_device_name,
                          p_tbx_slist_t   hosts_slist,
                          p_tbx_htable_t *p_hosts_htable,
                          p_tbx_htable_t *p_device_hosts_htable)
{
  p_tbx_htable_t hosts_htable        = NULL;
  p_tbx_htable_t device_hosts_htable = NULL;

  void _f(void *_object) {
    p_leoparse_object_t  object             = _object;
    char                *host_name          = NULL;
    char                *device_host_name   = NULL;
    p_tbx_htable_t	 network_host_htable = NULL;
    p_tbx_htable_t       device_host_htable = NULL;

    device_host_name = leoparse_try_get_id(object);

    if (device_host_name) {
      network_host_htable = leo_htable_init();
      leoparse_write_id(network_host_htable, "name", device_host_name);
    } else {
      network_host_htable = leoparse_get_htable(object);
      device_host_name = leoparse_read_id(network_host_htable, "name");
    }

    TRACE_STR("expected name", device_host_name);

    host_name = ntbx_true_name(device_host_name);
    TRACE_STR("true name", host_name);

    tbx_htable_add(network_host_htable, "host_name", host_name);

    if (!tbx_htable_get(hosts_htable, host_name)) {
        tbx_htable_add(hosts_htable, host_name, network_host_htable);
        device_host_htable = leo_htable_init();
        tbx_htable_add(device_hosts_htable, host_name, device_host_htable);
    } else {
      device_host_htable = tbx_htable_get(device_hosts_htable, host_name);
    }

    if (!tbx_htable_get(device_host_htable, network_device_name)) {
      tbx_htable_add(device_host_htable, network_device_name, device_host_name);
    }

    if (!tbx_htable_get(device_host_htable, "_default_")) {
      tbx_htable_add(device_host_htable, "_default_", device_host_name);
    }
  }

  LOG_IN();
  hosts_htable        = leo_htable_init();
  device_hosts_htable = leo_htable_init();

  do_slist(hosts_slist, _f);

  *p_hosts_htable        = hosts_htable;
  *p_device_hosts_htable = device_hosts_htable;
  LOG_OUT();
}

// make_fchannel: generates a 'dir_fchannel' forwarding channel data structure
// from a dir_channel data structure. The forwarding channel name is derived
// from the channel name by prepending an 'f' to it.
p_leo_dir_fchannel_t
make_fchannel(p_leo_directory_t   dir,
	      p_leo_dir_channel_t dir_channel)
{
  p_leo_dir_fchannel_t  dir_fchannel = NULL;
  char                 *ref_name     = NULL;

  void _cnx_init(ntbx_process_lrank_t _l, p_ntbx_process_t _process) {
    p_leo_dir_connection_t cnx = NULL;

    cnx = leo_dir_connection_init();
    ntbx_pc_add(dir_fchannel->pc, _process, _l, dir_fchannel, ref_name, cnx);
  }

  p_tbx_string_t s = NULL;

  LOG_IN();
  dir_fchannel = leo_dir_fchannel_init();
  s = tbx_string_init_to_char('f');
  tbx_string_append_cstring(s, dir_channel->name);
  dir_fchannel->name         = tbx_string_to_cstring(s);
  dir_fchannel->channel_name = strdup(dir_channel->name);
  tbx_string_append_cstring(s, "_common");

  ref_name = tbx_string_to_cstring_and_free(s);
  do_pc_local_p(dir_channel->pc, _cnx_init);

  tbx_htable_add(dir->fchannel_htable, dir_fchannel->name, dir_fchannel);
  tbx_slist_append(dir->fchannel_slist, dir_fchannel);
  LOG_OUT();

  return dir_fchannel;
}

static
void
list_append_int(p_tbx_slist_t slist, int val) {
  int *p_val = TBX_MALLOC(sizeof(int));
  *p_val = val;
  tbx_slist_append(slist, p_val);
}


// expand_sbracket_modifier: the Leoparse 'square-bracket' identifier
// modifier defines a series of values and ranges of values. The purpose
// of 'expand_sbracket_modifier' is to generate a list made of these values.
// if there is no modifiers or the modifier is not a 'square-bracket' modifier
// the 'default_value' argument is returned into the list.
p_tbx_slist_t
expand_sbracket_modifier(p_leoparse_modifier_t modifier,
			 int                   default_value)
{
  p_tbx_slist_t slist = NULL;

  LOG_IN();
  slist = tbx_slist_nil();

  if (modifier && modifier->type == leoparse_m_sbracket) {
    p_tbx_slist_t ranges = modifier->sbracket;

    if (!ranges || tbx_slist_is_nil(ranges))
      TBX_FAILURE("parse error : no range list");

    tbx_slist_ref_to_head(ranges);

    do {
      p_leoparse_object_t o = tbx_slist_ref_get(ranges);

      if (o->type == leoparse_o_integer) {
        int val = leoparse_get_val(o);
        list_append_int(slist, val);
      } else if (o->type == leoparse_o_range) {
        p_leoparse_range_t range = NULL;
        int                val   =    0;

        range = leoparse_get_range(o);

        for (val = range->begin; val < range->end; val++) {
          list_append_int(slist, val);
        }
      } else
        TBX_FAILURE("parse error : invalid object type");
    } while (tbx_slist_ref_forward(ranges));
  } else {
    list_append_int(slist, default_value);
  }
  LOG_OUT();

  return slist;
}

static
void
fill_dps(p_leo_dir_driver_process_specific_t  dps,
         p_tbx_htable_t                       network_host_htable,
         const char                          *adapter_name,
         const char                          *adapter_selector)
{
  p_leo_dir_adapter_t dir_adapter = NULL;

  dir_adapter = tbx_htable_get(dps->adapter_htable, adapter_name);

  if (!dir_adapter) {
    dir_adapter = leo_dir_adapter_init();

    dir_adapter->name      = strdup(adapter_name);
    dir_adapter->selector  = strdup(adapter_selector);
    dir_adapter->parameter = NULL;

    tbx_htable_add(dps->adapter_htable, dir_adapter->name, dir_adapter);
    tbx_slist_append(dps->adapter_slist, dir_adapter);
  }

  dps->parameter = leoparse_try_read_string(network_host_htable, "parameter");
  if (!dps->parameter) {
    dps->parameter = tbx_strdup("-");
  }

  TRACE_STR("driver parameter", dps->parameter);
}

static
char *
build_ref_name(const char *name, const char *type)
{
  p_tbx_string_t  ref_str  = NULL;
  char           *ref_name = NULL;

  LOG_IN();
  ref_str = tbx_string_init_to_cstring(name);
  tbx_string_append_char(ref_str, ':');
  tbx_string_append_cstring(ref_str, type);
  ref_name = tbx_string_to_cstring_and_free(ref_str);
  LOG_OUT();

  return ref_name;
}

static
void
set_driver_process_info(p_leo_dir_driver_t  dir_driver,
                        p_ntbx_process_t    process,
                        p_tbx_htable_t      network_host_htable,
                        const char         *adapter_name,
                        const char         *adapter_selector)
{
  p_ntbx_process_info_t                dpi = NULL;
  p_leo_dir_driver_process_specific_t  dps = NULL;

  LOG_IN();
  dpi = ntbx_pc_get_global(dir_driver->pc, process->global_rank);

  if (!dpi) {
    char *ref_name = NULL;

    dps = leo_dir_driver_process_specific_init();
    ref_name = build_ref_name(dir_driver->network_name, "driver");
    ntbx_pc_add(dir_driver->pc, process, -1, dir_driver, ref_name, dps);

    TBX_FREE(ref_name);
  } else {
    dps = dpi->specific;
  }

  fill_dps(dps, network_host_htable, adapter_name, adapter_selector);
  LOG_OUT();
}

static
void
set_channel_process_info(p_leo_dir_channel_t  dir_channel,
                         p_ntbx_process_t     process,
                         const char          *adapter_name)
{
  p_leo_dir_connection_t  cnx         = NULL;
  char                   *c_ref_name  = NULL;

  LOG_IN();
  if (ntbx_pc_get_global(dir_channel->pc, process->global_rank))
    {
      LOG_OUT();
      leo_terminate("duplicate process in channel process list", NULL);
    }
  cnx = leo_dir_connection_init();
  cnx->adapter_name = strdup(adapter_name);
  c_ref_name = build_ref_name(dir_channel->name, "channel");
  ntbx_pc_add(dir_channel->pc, process, -1, dir_channel, c_ref_name, cnx);
  TBX_FREE(c_ref_name);
  LOG_OUT();
}

// process_dummy_channel: analyses a channel description returned by
// Leoparse from the application configuration file parsing and builds
// the corresponding directory data structures.
void
process_dummy_channel(p_leonie_t    leonie,
                      p_tbx_slist_t channel_host_slist)
{
  p_leo_spawn_groups_t   spawn_groups            = NULL;
  p_leo_spawn_group_t    spawn_group             = NULL;
  p_leo_directory_t      dir                     = NULL;

  LOG_IN();
  spawn_groups = leonie->spawn_groups;
  dir          = leonie->directory;

  spawn_group = tbx_htable_get(spawn_groups->htable, "dummy");

  if (!spawn_group) {
    spawn_group = leo_spawn_group_init();
    spawn_group->loader_name = strdup("default");
    tbx_htable_add(spawn_groups->htable, "dummy", spawn_group);
    tbx_slist_append(spawn_groups->slist, spawn_group);
  }

  tbx_slist_ref_to_head(channel_host_slist);
  do {
      p_leoparse_object_t  object              = NULL;
      char                *host_name           = NULL;
      char                *channel_host_name   = NULL;
      p_leo_dir_node_t     dir_node            = NULL;
      p_tbx_slist_t        process_slist       = NULL;

      object = tbx_slist_ref_get(channel_host_slist);

      channel_host_name = tbx_strdup(leoparse_get_id(object));
      TRACE_STR("expected name", channel_host_name);

      host_name = ntbx_true_name(channel_host_name);
      TRACE_STR("true name", host_name);

      TRACE_STR("====== node hostname", host_name);

      dir_node = tbx_htable_get(dir->node_htable, host_name);

      if (!dir_node) {
        dir_node       = leo_dir_node_init();
        dir_node->name = strdup(host_name);

        dir_node->device_host_names = NULL;
        dir_node->channel_host_names = leo_htable_init();
        tbx_htable_add(dir_node->channel_host_names,
                       "dummy", channel_host_name);
        tbx_htable_add(dir->node_htable, host_name, dir_node);
        tbx_slist_append(dir->node_slist, dir_node);
      }

      process_slist = expand_sbracket_modifier(object->modifier, 0);

      tbx_slist_ref_to_head(process_slist);
      do {
        p_ntbx_process_t     process         = NULL;
        ntbx_process_lrank_t node_local_rank =   -1;

        {
          int *p_val = NULL;

          p_val = tbx_slist_ref_get(process_slist);
          node_local_rank = *p_val;

          TRACE_VAL("======== Process number", node_local_rank);
        }

        {
          p_ntbx_process_info_t node_process_info = NULL;

          node_process_info = ntbx_pc_get_local(dir_node->pc,
                                                node_local_rank);

          if (!node_process_info) {
            p_leo_process_specific_t nps = NULL;

            nps = leo_process_specific_init();
            ntbx_tcp_client_init(nps->client);
            nps->current_loader_name      = strdup("default");
            nps->current_spawn_group_name = strdup("dummy");
            nps->current_loader_priority  = leo_loader_priority_low;

            process              = ntbx_process_cons();
            process->ref         = leo_htable_init();
            process->specific    = nps;
            process->global_rank = tbx_slist_get_length(dir->process_slist);

            tbx_slist_append(dir->process_slist, process);

            ntbx_pc_add(dir_node->pc, process, node_local_rank,
                        dir_node, "node", NULL);

            tbx_slist_append(spawn_group->process_slist, process);
          }
        }

      } while (tbx_slist_ref_forward(process_slist));

      TBX_FREE(host_name);
    } while (tbx_slist_ref_forward(channel_host_slist));

  LOG_OUT();
}

// process_channel: analyses a channel description returned by Leoparse from
// the application configuration file parsing and builds the corresponding
// directory data structures.
void
process_channel(p_leonie_t     leonie,
		p_tbx_htable_t channel)
{
  char                  *channel_name            = NULL;
  char                  *channel_network         = NULL;
  p_tbx_slist_t          channel_host_slist      = NULL;
  p_tbx_htable_t         network_htable          = NULL;
  char                  *network_name            = NULL;
  p_tbx_htable_t         hosts_htable            = NULL;
  p_tbx_htable_t         device_hosts_htable     = NULL;
  char                  *network_device_name     = NULL;
  char                  *network_loader          = NULL;
  leo_loader_priority_t  network_loader_priority =
    leo_loader_priority_undefined;
  p_leo_dir_channel_t    dir_channel             = NULL;
  char                  *channel_reference_name  = NULL;
  char                  *driver_reference_name   = NULL;
  p_leo_dir_driver_t     dir_driver              = NULL;
  p_leo_spawn_groups_t   spawn_groups            = NULL;
  p_leo_spawn_group_t    spawn_group             = NULL;
  p_leo_networks_t       networks                = NULL;
  p_leo_directory_t      dir                     = NULL;
  p_tbx_slist_t		 network_hosts_slist	 = NULL;
  tbx_bool_t		 dynamic_host_list	 = tbx_false;

  LOG_IN();
  spawn_groups = leonie->spawn_groups;
  networks     = leonie->networks;
  dir          = leonie->directory;

  channel_name = leoparse_read_id(channel, "name");
  TRACE_STR("====== Processing channel", channel_name);

  channel_network  = leoparse_read_id(channel, "net");
  TRACE_STR("====== Channel network is", channel_network);

  channel_host_slist = leoparse_read_as_slist(channel, "hosts");

  network_htable = tbx_htable_get(networks->htable, channel_network);
  if (!network_htable)
    TBX_FAILURE("unknown network");

  network_name        = leoparse_read_id(network_htable, "name");
  network_device_name = leoparse_read_id(network_htable, "dev");

  if ((network_loader = leoparse_read_id(network_htable, "mandatory_loader")))
    {
      network_loader_priority = leo_loader_priority_high;
    }
  else if ((network_loader =
	    leoparse_read_id(network_htable, "optional_loader")))
    {
      network_loader_priority = leo_loader_priority_medium;
    }
  else
    {
      network_loader_priority = leo_loader_priority_low;
      network_loader = strdup("default");
    }

  network_hosts_slist = leoparse_read_as_slist(network_htable, "hosts");
  dynamic_host_list	= !network_hosts_slist;

  hosts_htable        = tbx_htable_get(network_htable, "hosts_htable");
  device_hosts_htable = tbx_htable_get(network_htable, "device_hosts_htable");

  if (!hosts_htable) {
    if (network_hosts_slist) {
      build_network_host_htable(network_device_name,
                                network_hosts_slist,
                                &hosts_htable,
                                &device_hosts_htable);
    } else {
      hosts_htable		= leo_htable_init();
      device_hosts_htable	= leo_htable_init();
    }

    tbx_htable_add(network_htable, "hosts_htable", hosts_htable);
    tbx_htable_add(network_htable, "device_hosts_htable", device_hosts_htable);
  }

  TRACE_STR("====== effective network name", network_name);
  TRACE_STR("====== network device", network_device_name);
  TRACE_STR("====== network loader", network_loader);
  TRACE_VAL("====== network loader priority", (int)network_loader_priority);

  dir_channel            = leo_dir_channel_init();
  dir_channel->name      = strdup(channel_name);
  channel_reference_name = build_ref_name(channel_name, "channel");

  dir_driver = tbx_htable_get(dir->driver_htable, network_name);

  if (!dir_driver) {
    dir_driver = leo_dir_driver_init();
    dir_driver->network_name = strdup(network_name);
    dir_driver->device_name = strdup(network_device_name);
    tbx_htable_add(dir->driver_htable, dir_driver->network_name, dir_driver);
    tbx_slist_append(dir->driver_slist, dir_driver);
  }

  dir_channel->driver	= dir_driver;
  driver_reference_name	= build_ref_name(dir_driver->network_name, "driver");

  spawn_group = tbx_htable_get(spawn_groups->htable, network_name);

  if (!spawn_group) {
    spawn_group = leo_spawn_group_init();
    spawn_group->loader_name = strdup(network_loader);
    tbx_htable_add(spawn_groups->htable, network_name, spawn_group);
    tbx_slist_append(spawn_groups->slist, spawn_group);
  }

  tbx_slist_ref_to_head(channel_host_slist);
  do {
      p_leoparse_object_t  object              = NULL;
      char                *host_name           = NULL;
      char                *channel_host_name   = NULL;
      p_leo_dir_node_t     dir_node            = NULL;
      p_tbx_slist_t        process_slist       = NULL;
      p_tbx_htable_t       network_host_htable = NULL;
      const char          *adapter_name        = "default";
      const char          *adapter_selector    = "-";

      object = tbx_slist_ref_get(channel_host_slist);

      channel_host_name = tbx_strdup(leoparse_get_id(object));
      TRACE_STR("expected name", channel_host_name);

      host_name = ntbx_true_name(channel_host_name);
      TRACE_STR("true name", host_name);

      network_host_htable = tbx_htable_get(hosts_htable, host_name);
      if (!network_host_htable) {
        if (dynamic_host_list) {
          network_host_htable = leo_htable_init();
          leoparse_write_id(network_host_htable, "name", host_name);
          tbx_htable_add(network_host_htable, "host_name", host_name);
          tbx_htable_add(hosts_htable, host_name, network_host_htable);
        } else
          TBX_FAILURE("unknown hostname");
      }

      if (dynamic_host_list) {
        p_tbx_htable_t       device_host_htable = NULL;

        if (!tbx_htable_get(device_hosts_htable, host_name)) {
          device_host_htable = leo_htable_init();
          tbx_htable_add(device_hosts_htable, host_name, device_host_htable);
        } else {
          device_host_htable = tbx_htable_get(device_hosts_htable, host_name);
        }

        if (!tbx_htable_get(device_host_htable, network_device_name)) {
          tbx_htable_add(device_host_htable, network_device_name, host_name);
        }

        if (!tbx_htable_get(device_host_htable, "_default_")) {
          tbx_htable_add(device_host_htable, "_default_", host_name);
        }
      }

      TRACE_STR("====== node hostname", host_name);

      dir_node = tbx_htable_get(dir->node_htable, host_name);

      if (!dir_node) {
        dir_node       = leo_dir_node_init();
        dir_node->name = strdup(host_name);

        if (!(dir_node->device_host_names =
              tbx_htable_get(device_hosts_htable, host_name)))
          TBX_FAILURE("invalid state");

        dir_node->channel_host_names = leo_htable_init();
        tbx_htable_add(dir_node->channel_host_names,
                       channel_name, channel_host_name);
        tbx_htable_add(dir->node_htable, host_name, dir_node);
        tbx_slist_append(dir->node_slist, dir_node);
      }

      process_slist = expand_sbracket_modifier(object->modifier, 0);

      tbx_slist_ref_to_head(process_slist);
      do {
        p_ntbx_process_t     process         = NULL;
        ntbx_process_lrank_t node_local_rank =   -1;

        {
          int *p_val = NULL;

          p_val = tbx_slist_ref_get(process_slist);
          node_local_rank = *p_val;

          TRACE_VAL("======== Process number", node_local_rank);
        }

        {
          p_ntbx_process_info_t node_process_info = NULL;

          node_process_info = ntbx_pc_get_local(dir_node->pc,
                                                node_local_rank);

          if (!node_process_info) {
            p_leo_process_specific_t nps = NULL;

            nps = leo_process_specific_init();
            ntbx_tcp_client_init(nps->client);
            nps->current_loader_name      = network_loader;
            nps->current_spawn_group_name = network_name;
            nps->current_loader_priority  = network_loader_priority;

            process              = ntbx_process_cons();
            process->ref         = leo_htable_init();
            process->specific    = nps;
            process->global_rank = tbx_slist_get_length(dir->process_slist);

            tbx_slist_append(dir->process_slist, process);

            ntbx_pc_add(dir_node->pc, process, node_local_rank,
                        dir_node, "node", NULL);

            tbx_slist_append(spawn_group->process_slist, process);
          } else {
            p_leo_process_specific_t nps = NULL;

            process = node_process_info->process;
            nps     = process->specific;

            if (nps->current_loader_priority < network_loader_priority) {
              p_leo_spawn_group_t previous_spawn_group = NULL;

              previous_spawn_group =
                tbx_htable_get(spawn_groups->htable,
                               nps->current_spawn_group_name);

              tbx_slist_search_and_extract(previous_spawn_group->process_slist,
                                           NULL, process);

              nps->current_loader_name      = network_loader;
              nps->current_spawn_group_name = network_name;
              nps->current_loader_priority  = network_loader_priority;

              tbx_slist_append(spawn_group->process_slist, process);

            } else if (network_loader_priority == leo_loader_priority_high
                       &&
                       strcmp(nps->current_loader_name, network_loader))
              TBX_FAILURE("conflicting loaders");
          }
        }

        set_channel_process_info(dir_channel, process, adapter_name);

        set_driver_process_info(dir_driver, process, network_host_htable,
                                adapter_name, adapter_selector);

      } while (tbx_slist_ref_forward(process_slist));

      TBX_FREE(host_name);
    } while (tbx_slist_ref_forward(channel_host_slist));

    {
      p_tbx_htable_t	 topo_htable	= NULL;
      char		*type		= NULL;

      topo_htable = leoparse_try_read_htable(network_htable, "topology");
      if (topo_htable) {
        type	= leoparse_read_id(topo_htable, "type");
      }

      if (!type || tbx_streq(type, "default")) {
        dir_channel->ttable = ntbx_topology_table_init(dir_channel->pc,
                                                       ntbx_topology_regular,
                                                       NULL);
      } else if (tbx_streq(type, "empty")) {
        dir_channel->ttable = ntbx_topology_table_init(dir_channel->pc,
                                                       ntbx_topology_empty,
                                                       NULL);
      } else if (tbx_streq(type, "full")) {
        dir_channel->ttable = ntbx_topology_table_init(dir_channel->pc,
                                                       ntbx_topology_full,
                                                       NULL);
      } else if (tbx_streq(type, "star")) {
        TBX_FAILUREF("unimplemented topology type: '%s'", type);
      } else if (tbx_streq(type, "ring")) {
        TBX_FAILUREF("unimplemented topology type: '%s'", type);
      } else if (tbx_streq(type, "grid")) {
        TBX_FAILUREF("unimplemented topology type: '%s'", type);
      } else if (tbx_streq(type, "torus")) {
        TBX_FAILUREF("unimplemented topology type: '%s'", type);
      } else if (tbx_streq(type, "hypercube")) {
        TBX_FAILUREF("unimplemented topology type: '%s'", type);
      } else if (tbx_streq(type, "node")) {
        p_ntbx_process_container_t pc = dir_channel->pc;

        int _f_node(ntbx_process_lrank_t src,  ntbx_process_lrank_t dst) {
          p_ntbx_process_t	psrc;
          p_leo_dir_node_t	nsrc;
          p_ntbx_process_t	pdst;
          p_leo_dir_node_t	ndst;

          if (src == dst)
            return 0;

          psrc	= ntbx_pc_get_local_process(pc, src);
          if (!psrc)
            return 0;

          pdst	= ntbx_pc_get_local_process(pc, dst);
          if (!pdst)
            return 0;

          nsrc	= tbx_htable_get(psrc->ref, "node");
          if (!nsrc)
            TBX_FAILURE("invalid state");

          ndst	= tbx_htable_get(pdst->ref, "node");
          if (!ndst)
            TBX_FAILURE("invalid state");

          return nsrc == ndst;
        }

        dir_channel->ttable = ntbx_topology_table_init(pc,
                                                       ntbx_topology_function,
                                                       _f_node);
      } else
        TBX_FAILUREF("unknown topology type: '%s'", type);
    }

  tbx_htable_add(dir->channel_htable, dir_channel->name, dir_channel);
  tbx_slist_append(dir->channel_slist, dir_channel);
  LOG_OUT();
}

// process_vchannel: analyses a virtual channel description returned by
// Leoparse from the application configuration file parsing and builds
// the corresponding directory data structures.
void
process_vchannel(p_leonie_t     leonie,
		 p_tbx_htable_t vchannel)
{
  char                       *vchannel_name           = NULL;
  p_tbx_slist_t               vchannel_channel_slist  = NULL;
  p_leo_dir_vxchannel_t       dir_vchannel            = NULL;
  p_ntbx_process_container_t  dir_vchannel_pc         = NULL;
  p_tbx_slist_t               dir_channel_slist       = NULL;
  p_tbx_slist_t               dir_fchannel_slist      = NULL;
  char                       *vchannel_reference_name = NULL;
  p_leo_directory_t           dir                     = NULL;

  LOG_IN();
  dir = leonie->directory;

  vchannel_name = leoparse_read_id(vchannel, "name");
  TRACE_STR("====== Processing vchannel", vchannel_name);

  vchannel_channel_slist = leoparse_read_as_slist(vchannel, "channels");
  dir_vchannel           = leo_dir_vxchannel_init();
  dir_vchannel->name     = tbx_strdup(vchannel_name);
  dir_vchannel_pc        = dir_vchannel->pc;
  dir_channel_slist      = dir_vchannel->dir_channel_slist;
  dir_fchannel_slist     = dir_vchannel->dir_fchannel_slist;

  vchannel_reference_name =
    TBX_MALLOC(strlen("vchannel") + strlen(vchannel_name) + 2);
  sprintf(vchannel_reference_name, "%s:%s", "vchannel", vchannel_name);

  if (tbx_slist_is_nil(vchannel_channel_slist))
    {
      leo_terminate("vchannel has empty real channel list", leonie->settings);
    }

  TRACE("====== Process list construction");
  tbx_slist_ref_to_head(vchannel_channel_slist);
  do
    {
      p_leoparse_object_t        object         = NULL;
      char                      *channel_name   = NULL;
      p_leo_dir_channel_t        dir_channel    = NULL;
      p_leo_dir_fchannel_t       dir_fchannel   = NULL;
      p_ntbx_process_container_t dir_channel_pc = NULL;
      ntbx_process_lrank_t       rank           =    0;

      object = tbx_slist_ref_get(vchannel_channel_slist);
      channel_name = leoparse_get_id(object);
      TRACE_STR("======== processing real channel", channel_name);

      dir_channel = tbx_htable_get(dir->channel_htable, channel_name);
      if (!dir_channel)
	TBX_FAILURE("real channel not found");

      TRACE("real channel found");

      if (!dir_channel->public)
	TBX_FAILURE("real channel already in use");

      dir_channel->public = tbx_false;
      TRACE("real channel locked");

      dir_channel_pc = dir_channel->pc;
      if (!ntbx_pc_first_local_rank(dir_channel_pc, &rank))
	continue;

      tbx_slist_append(dir_channel_slist, dir_channel);
      dir_fchannel = make_fchannel(dir, dir_channel);
      tbx_slist_append(dir_fchannel_slist, dir_fchannel);

      do
	{
	  p_ntbx_process_info_t process_info = NULL;
	  p_ntbx_process_t      process      = NULL;
	  ntbx_process_grank_t  global_rank  =    0;
	  p_ntbx_process_info_t tmp_pi       = NULL;

	  process_info = ntbx_pc_get_local(dir_channel_pc, rank);

	  process     = process_info->process;
	  global_rank = process->global_rank;
	  TRACE_VAL("======== adding process", global_rank);

	  tmp_pi = ntbx_pc_get_global(dir_vchannel_pc, global_rank);

	  if (tmp_pi)
	    {
	      TRACE("process already added");
	    }
	  else
	    {
	      p_leo_dir_connection_t cnx = NULL;

	      TRACE("process is new for this vchannel");

	      cnx = leo_dir_connection_init();

	      ntbx_pc_add(dir_vchannel_pc, process, -1,
			  dir_vchannel, vchannel_reference_name, cnx);
	    }
	}
      while (ntbx_pc_next_local_rank(dir_channel_pc, &rank));
    }
  while (tbx_slist_ref_forward(vchannel_channel_slist));

  if (tbx_slist_is_nil(dir_channel_slist))
    {
      TRACE("vchannel is empty");
      goto end;
    }

  TRACE("====== Routing table generation");
  {
    typedef struct s_leo_path
    {
      ntbx_process_grank_t dest_rank;
      p_leo_dir_channel_t  dir_channel;
      p_leo_dir_fchannel_t dir_fchannel;
      tbx_bool_t           last;
    } leo_path_t, *p_leo_path_t;

    p_leo_path_t         *routing_table_current =     NULL;
    p_leo_path_t         *routing_table_next    =     NULL;
    ntbx_process_lrank_t  table_size            =        0;
    ntbx_process_grank_t  g_rank_src            =        0;
    int                   pass                  =        0;
    int                   pass_number           =        0;

    TRACE("====== Table initialization");
    table_size = dir_vchannel_pc->count;

    routing_table_current =
      TBX_CALLOC(table_size * table_size, sizeof(p_leo_path_t));

    routing_table_next =
      TBX_CALLOC(table_size * table_size, sizeof(p_leo_path_t));

    tbx_slist_ref_to_head(dir_channel_slist);
    tbx_slist_ref_to_head(dir_fchannel_slist);
    do
      {
	p_leo_dir_channel_t        dir_channel         = NULL;
	p_ntbx_process_container_t dir_channel_pc      = NULL;
	p_ntbx_topology_table_t    dir_channel_ttable  = NULL;
	p_leo_dir_fchannel_t       dir_fchannel        = NULL;

	dir_channel  = tbx_slist_ref_get(dir_channel_slist);
	TRACE_STR("======== channel", dir_channel->name);
	dir_channel_pc     = dir_channel->pc;
	dir_channel_ttable = dir_channel->ttable;

	dir_fchannel = tbx_slist_ref_get(dir_fchannel_slist);

	ntbx_pc_first_global_rank(dir_vchannel_pc, &g_rank_src);
	do
	  {
	    ntbx_process_grank_t g_rank_dst =  0;
	    ntbx_process_lrank_t c_rank_src = -1;
	    ntbx_process_lrank_t l_rank_src = -1;

	    c_rank_src = ntbx_pc_global_to_local(dir_channel_pc, g_rank_src);
	    if (c_rank_src < 0)
	      continue;

	    l_rank_src = ntbx_pc_global_to_local(dir_vchannel_pc, g_rank_src);

	    TRACE_VAL("======== source", l_rank_src);

	    ntbx_pc_first_global_rank(dir_vchannel_pc, &g_rank_dst);
	    do
	      {
		ntbx_process_lrank_t c_rank_dst =   -1;
		ntbx_process_lrank_t l_rank_dst =   -1;
		p_leo_path_t         path       = NULL;

		c_rank_dst = ntbx_pc_global_to_local(dir_channel_pc,
						     g_rank_dst);
		if (c_rank_dst < 0)
		  continue;

		if (!ntbx_topology_table_get(dir_channel_ttable,
					     c_rank_src, c_rank_dst))
		  continue;

		l_rank_dst = ntbx_pc_global_to_local(dir_vchannel_pc,
						     g_rank_dst);
		TRACE_VAL("======== destination", l_rank_dst);

		{
		  const ntbx_process_lrank_t location =
		    l_rank_src * table_size + l_rank_dst;

		  path = routing_table_current[location];

		  if (path)
		    continue;

		  path = TBX_MALLOC(sizeof(leo_path_t));

		  path->dest_rank    = g_rank_dst;
		  path->dir_channel  = dir_channel;
		  path->dir_fchannel = dir_fchannel;
		  path->last         = tbx_true;

		  routing_table_current[location] = path;
		}
	      }
	    while (ntbx_pc_next_global_rank(dir_vchannel_pc, &g_rank_dst));
	  }
	while (ntbx_pc_next_global_rank(dir_vchannel_pc, &g_rank_src));
      }
    while (   tbx_slist_ref_forward(dir_channel_slist)
	   && tbx_slist_ref_forward(dir_fchannel_slist));

    pass        = 0;
    pass_number = tbx_slist_get_length(dir_channel_slist);

    do
      {
	tbx_bool_t need_additional_pass  = tbx_false;
	tbx_bool_t process_is_converging = tbx_false;

	TRACE_VAL("====== Table filling, pass", pass);

	ntbx_pc_first_global_rank(dir_vchannel_pc, &g_rank_src);
	do
	  {
	    ntbx_process_grank_t g_rank_dst =  0;
	    ntbx_process_lrank_t l_rank_src = -1;

	    l_rank_src =
	      ntbx_pc_global_to_local(dir_vchannel_pc, g_rank_src);

	    TRACE_VAL("======== source", l_rank_src);

	    ntbx_pc_first_global_rank(dir_vchannel_pc, &g_rank_dst);

	    do
	      {
		ntbx_process_lrank_t l_rank_dst =   -1;
		ntbx_process_grank_t g_rank_med =   -1;

		l_rank_dst = ntbx_pc_global_to_local(dir_vchannel_pc,
						     g_rank_dst);
		TRACE_VAL("======== destination", l_rank_dst);

		{
		  p_leo_path_t               path     = NULL;
		  const ntbx_process_lrank_t location =
		    l_rank_src * table_size + l_rank_dst;

		  path = routing_table_current[location];

		  if (path)
		    {
		      routing_table_next[location] = path;
		      continue;
		    }

		  ntbx_pc_first_global_rank(dir_vchannel_pc, &g_rank_med);
		  do
		    {
		      ntbx_process_lrank_t l_rank_med = -1;
		      p_leo_path_t         path_src   = NULL;
		      p_leo_path_t         path_dst   = NULL;

		      l_rank_med = ntbx_pc_global_to_local(dir_vchannel_pc,
							   g_rank_med);

		      path_src   =
			routing_table_current[l_rank_src * table_size +
					     l_rank_med];

		      if (!path_src)
			continue;

		      path_dst =
			routing_table_current[l_rank_med * table_size +
					     l_rank_dst];

		      if (!path_dst)
			continue;

		      TRACE_VAL("found path through", l_rank_med);

		      path = TBX_MALLOC(sizeof(leo_path_t));

		      *path = *path_src;
		      path->last = tbx_false;

		      break;
		    }
		  while
		    (ntbx_pc_next_global_rank(dir_vchannel_pc, &g_rank_med));

		  if (path)
		    {
		      routing_table_next[location] = path;
		      process_is_converging        = tbx_true;
		    }
		  else
		    {
		      routing_table_next[location] = NULL;
		      need_additional_pass         = tbx_true;
		    }
		}
	      }
	    while (ntbx_pc_next_global_rank(dir_vchannel_pc, &g_rank_dst));
	  }
	while (ntbx_pc_next_global_rank(dir_vchannel_pc, &g_rank_src));

	if (!process_is_converging)
	  {
	    leo_terminate("routing table generation process "
			  "is not converging", leonie->settings);
	  }

	{
	  p_leo_path_t *temp = NULL;

	  temp                  = routing_table_current;
	  routing_table_current = routing_table_next;
	  routing_table_next    = temp;
	}

	if (!need_additional_pass)
	  break;
      }
    while (++pass < pass_number);
    TRACE_STR("====== Routing table ready for virtual channel", vchannel_name);

    TRACE_STR("====== Transferring routing table to the internal dir",
	     vchannel_name);
    ntbx_pc_first_global_rank(dir_vchannel_pc, &g_rank_src);
    do
      {
	ntbx_process_grank_t       g_rank_dst  =    0;
	ntbx_process_lrank_t       l_rank_src  =   -1;
	p_ntbx_process_info_t      pi          = NULL;
	p_leo_dir_connection_t     cnx         = NULL;
	p_ntbx_process_container_t pc          = NULL;
	p_ntbx_process_t           process_src = NULL;

	l_rank_src  = ntbx_pc_global_to_local(dir_vchannel_pc, g_rank_src);
	TRACE_VAL("======== source", l_rank_src);
	pi          = ntbx_pc_get_global(dir_vchannel_pc, g_rank_src);
	process_src = pi->process;
	cnx         = pi->specific;
	pc          = cnx->pc;

	ntbx_pc_first_global_rank(dir_vchannel_pc, &g_rank_dst);
	do
	  {
	    p_leo_dir_connection_data_t  cdata       = NULL;
	    ntbx_process_lrank_t         l_rank_dst  =   -1;
	    p_ntbx_process_t             process_dst = NULL;
	    char                        *ref_name    = NULL;

	    l_rank_dst = ntbx_pc_global_to_local(dir_vchannel_pc, g_rank_dst);
	    TRACE_VAL("======== destination", l_rank_dst);

	    {
	      p_leo_path_t               path     = NULL;
	      const ntbx_process_lrank_t location =
		l_rank_src * table_size + l_rank_dst;

	      path   = routing_table_current[location];
	      cdata = leo_dir_connection_data_init();

	      if (path->last)
		{
		  cdata->channel_name = strdup(path->dir_channel->name);
		}
	      else
		{
		  cdata->channel_name = strdup(path->dir_fchannel->name);
		}
	      cdata->destination_rank = path->dest_rank;
	    }

	    process_dst = ntbx_pc_get_local_process(dir_vchannel_pc,
						    l_rank_dst);
	    {
	      size_t len = 0;

	      len = strlen(dir_vchannel->name) + 8;
	      ref_name = TBX_MALLOC(len);

	      sprintf(ref_name, "%s:%d", dir_vchannel->name, l_rank_src);
	    }

	    ntbx_pc_add(pc, process_dst, l_rank_dst,
			process_src, ref_name, cdata);
	    free(ref_name);
	    ref_name = NULL;
	  }
	while (ntbx_pc_next_global_rank(dir_vchannel_pc, &g_rank_dst));
      }
    while (ntbx_pc_next_global_rank(dir_vchannel_pc, &g_rank_src));

    TRACE("====== Transfer complete");
  }

  tbx_htable_add(dir->vchannel_htable, dir_vchannel->name, dir_vchannel);
  tbx_slist_append(dir->vchannel_slist, dir_vchannel);
  TRACE("====== Processing done");

 end:
  LOG_OUT();
}

void
process_xchannel(p_leonie_t     leonie,
		 p_tbx_htable_t xchannel)
{
  char                       *xchannel_name           = NULL;
  p_tbx_slist_t               xchannel_channel_slist  = NULL;
  p_leo_dir_vxchannel_t       dir_xchannel            = NULL;
  p_ntbx_process_container_t  dir_xchannel_pc         = NULL;
  p_tbx_slist_t               dir_channel_slist       = NULL;
  p_tbx_slist_t               xchannel_sub_slist      = NULL;
  p_tbx_slist_t               sub_channel_name_slist  = NULL;
  char                       *xchannel_reference_name = NULL;
  p_leo_directory_t           dir                     = NULL;

  LOG_IN();
  dir = leonie->directory;

  xchannel_name = leoparse_read_id(xchannel, "name");
  TRACE_STR("====== Processing xchannel", xchannel_name);

  xchannel_channel_slist = leoparse_read_as_slist(xchannel, "channels");
  dir_xchannel           = leo_dir_vxchannel_init();
  dir_xchannel->name     = tbx_strdup(xchannel_name);
  dir_xchannel_pc        = dir_xchannel->pc;
  dir_channel_slist      = dir_xchannel->dir_channel_slist;
  xchannel_sub_slist     = leoparse_try_read_as_slist(xchannel, "sub_channels");
  sub_channel_name_slist = dir_xchannel->sub_channel_name_slist;

  xchannel_reference_name =
    TBX_MALLOC(strlen("xchannel") + strlen(xchannel_name) + 2);
  sprintf(xchannel_reference_name, "%s:%s", "xchannel", xchannel_name);

  if (tbx_slist_is_nil(xchannel_channel_slist))
    {
      leo_terminate("xchannel has empty real channel list", leonie->settings);
    }

  if (xchannel_sub_slist)
    {
      tbx_slist_ref_to_head(xchannel_sub_slist);
      do
	{
	  p_leoparse_object_t  object            = NULL;
	  char                *channel_name      = NULL;
	  char                *channel_name_copy = NULL;

	  object            = tbx_slist_ref_get(xchannel_sub_slist);
	  channel_name      = leoparse_get_id(object);
	  channel_name_copy = tbx_strdup(channel_name);
	  tbx_slist_append(sub_channel_name_slist, channel_name_copy);
	}
      while (tbx_slist_ref_forward(xchannel_sub_slist));
    }

  TRACE("====== Process list construction");
  tbx_slist_ref_to_head(xchannel_channel_slist);
  do
    {
      p_leoparse_object_t        object         = NULL;
      char                      *channel_name   = NULL;
      p_leo_dir_channel_t        dir_channel    = NULL;
      p_ntbx_process_container_t dir_channel_pc = NULL;
      ntbx_process_lrank_t       rank           =    0;

      object = tbx_slist_ref_get(xchannel_channel_slist);
      channel_name = leoparse_get_id(object);
      TRACE_STR("======== processing real channel", channel_name);

      dir_channel = tbx_htable_get(dir->channel_htable, channel_name);
      if (!dir_channel)
	TBX_FAILURE("real channel not found");

      TRACE("real channel found");

      if (!dir_channel->public)
	TBX_FAILURE("real channel already in use");

      dir_channel->public = tbx_false;
      TRACE("real channel locked");

      dir_channel_pc = dir_channel->pc;
      if (!ntbx_pc_first_local_rank(dir_channel_pc, &rank))
	continue;

      tbx_slist_append(dir_channel_slist, dir_channel);

      do
	{
	  p_ntbx_process_info_t process_info = NULL;
	  p_ntbx_process_t      process      = NULL;
	  ntbx_process_grank_t  global_rank  =    0;
	  p_ntbx_process_info_t tmp_pi       = NULL;

	  process_info = ntbx_pc_get_local(dir_channel_pc, rank);

	  process     = process_info->process;
	  global_rank = process->global_rank;
	  TRACE_VAL("======== adding process", global_rank);

	  tmp_pi = ntbx_pc_get_global(dir_xchannel_pc, global_rank);

	  if (tmp_pi)
	    {
	      TRACE("process already added");
	    }
	  else
	    {
	      p_leo_dir_connection_t cnx = NULL;

	      TRACE("process is new for this xchannel");

	      cnx = leo_dir_connection_init();

	      ntbx_pc_add(dir_xchannel_pc, process, -1,
			  dir_xchannel, xchannel_reference_name, cnx);
	    }
	}
      while (ntbx_pc_next_local_rank(dir_channel_pc, &rank));
    }
  while (tbx_slist_ref_forward(xchannel_channel_slist));

  if (tbx_slist_is_nil(dir_channel_slist))
    {
      TRACE("xchannel is empty");
      goto end;
    }

  TRACE("====== Routing table generation");
  {
    typedef struct s_leo_path
    {
      ntbx_process_grank_t dest_rank;
      p_leo_dir_channel_t  dir_channel;
      tbx_bool_t           last;
    } leo_path_t, *p_leo_path_t;

    p_leo_path_t         *routing_table_current =     NULL;
    p_leo_path_t         *routing_table_next    =     NULL;
    ntbx_process_lrank_t  table_size            =        0;
    ntbx_process_grank_t  g_rank_src            =        0;
    int                   pass                  =        0;
    int                   pass_number           =        0;

    TRACE("====== Table initialization");
    table_size = dir_xchannel_pc->count;

    routing_table_current =
      TBX_CALLOC(table_size * table_size, sizeof(p_leo_path_t));

    routing_table_next =
      TBX_CALLOC(table_size * table_size, sizeof(p_leo_path_t));

    tbx_slist_ref_to_head(dir_channel_slist);
    do
      {
	p_leo_dir_channel_t        dir_channel         = NULL;
	p_ntbx_process_container_t dir_channel_pc      = NULL;
	p_ntbx_topology_table_t    dir_channel_ttable  = NULL;

	dir_channel  = tbx_slist_ref_get(dir_channel_slist);
	TRACE_STR("======== channel", dir_channel->name);
	dir_channel_pc     = dir_channel->pc;
	dir_channel_ttable = dir_channel->ttable;

	ntbx_pc_first_global_rank(dir_xchannel_pc, &g_rank_src);
	do
	  {
	    ntbx_process_grank_t g_rank_dst =  0;
	    ntbx_process_lrank_t c_rank_src = -1;
	    ntbx_process_lrank_t l_rank_src = -1;

	    c_rank_src = ntbx_pc_global_to_local(dir_channel_pc, g_rank_src);
	    if (c_rank_src < 0)
	      continue;

	    l_rank_src = ntbx_pc_global_to_local(dir_xchannel_pc, g_rank_src);

	    TRACE_VAL("======== source", l_rank_src);

	    ntbx_pc_first_global_rank(dir_xchannel_pc, &g_rank_dst);
	    do
	      {
		ntbx_process_lrank_t c_rank_dst =   -1;
		ntbx_process_lrank_t l_rank_dst =   -1;
		p_leo_path_t         path       = NULL;

		c_rank_dst = ntbx_pc_global_to_local(dir_channel_pc,
						     g_rank_dst);
		if (c_rank_dst < 0)
		  continue;

		if (!ntbx_topology_table_get(dir_channel_ttable,
					     c_rank_src, c_rank_dst))
		  continue;

		l_rank_dst = ntbx_pc_global_to_local(dir_xchannel_pc,
						     g_rank_dst);
		TRACE_VAL("======== destination", l_rank_dst);

		{
		  const ntbx_process_lrank_t location =
		    l_rank_src * table_size + l_rank_dst;

		  path = routing_table_current[location];

		  if (path)
		    continue;

		  path = TBX_MALLOC(sizeof(leo_path_t));

		  path->dest_rank    = g_rank_dst;
		  path->dir_channel  = dir_channel;
		  path->last         = tbx_true;

		  routing_table_current[location] = path;
		}
	      }
	    while (ntbx_pc_next_global_rank(dir_xchannel_pc, &g_rank_dst));
	  }
	while (ntbx_pc_next_global_rank(dir_xchannel_pc, &g_rank_src));
      }
    while (tbx_slist_ref_forward(dir_channel_slist));

    pass        = 0;
    pass_number = tbx_slist_get_length(dir_channel_slist);

    do
      {
	tbx_bool_t need_additional_pass  = tbx_false;
	tbx_bool_t process_is_converging = tbx_false;

	TRACE_VAL("====== Table filling, pass", pass);

	ntbx_pc_first_global_rank(dir_xchannel_pc, &g_rank_src);
	do
	  {
	    ntbx_process_grank_t g_rank_dst =  0;
	    ntbx_process_lrank_t l_rank_src = -1;

	    l_rank_src =
	      ntbx_pc_global_to_local(dir_xchannel_pc, g_rank_src);

	    TRACE_VAL("======== source", l_rank_src);

	    ntbx_pc_first_global_rank(dir_xchannel_pc, &g_rank_dst);

	    do
	      {
		ntbx_process_lrank_t l_rank_dst =   -1;
		ntbx_process_grank_t g_rank_med =   -1;

		l_rank_dst = ntbx_pc_global_to_local(dir_xchannel_pc,
						     g_rank_dst);
		TRACE_VAL("======== destination", l_rank_dst);

		{
		  p_leo_path_t               path     = NULL;
		  const ntbx_process_lrank_t location =
		    l_rank_src * table_size + l_rank_dst;

		  path = routing_table_current[location];

		  if (path)
		    {
		      routing_table_next[location] = path;
		      continue;
		    }

		  ntbx_pc_first_global_rank(dir_xchannel_pc, &g_rank_med);
		  do
		    {
		      ntbx_process_lrank_t l_rank_med = -1;
		      p_leo_path_t         path_src   = NULL;
		      p_leo_path_t         path_dst   = NULL;

		      l_rank_med = ntbx_pc_global_to_local(dir_xchannel_pc,
							   g_rank_med);

		      path_src   =
			routing_table_current[l_rank_src * table_size +
					     l_rank_med];

		      if (!path_src)
			continue;

		      path_dst =
			routing_table_current[l_rank_med * table_size +
					     l_rank_dst];

		      if (!path_dst)
			continue;

		      TRACE_VAL("found path through", l_rank_med);

		      path = TBX_MALLOC(sizeof(leo_path_t));

		      *path = *path_src;
		      path->last = tbx_false;

		      break;
		    }
		  while
		    (ntbx_pc_next_global_rank(dir_xchannel_pc, &g_rank_med));

		  if (path)
		    {
		      routing_table_next[location] = path;
		      process_is_converging        = tbx_true;
		    }
		  else
		    {
		      routing_table_next[location] = NULL;
		      need_additional_pass         = tbx_true;
		    }
		}
	      }
	    while (ntbx_pc_next_global_rank(dir_xchannel_pc, &g_rank_dst));
	  }
	while (ntbx_pc_next_global_rank(dir_xchannel_pc, &g_rank_src));

	if (!process_is_converging)
	  {
	    leo_terminate("routing table generation process "
			  "is not converging", leonie->settings);
	  }

	{
	  p_leo_path_t *temp = NULL;

	  temp                  = routing_table_current;
	  routing_table_current = routing_table_next;
	  routing_table_next    = temp;
	}

	if (!need_additional_pass)
	  break;
      }
    while (++pass < pass_number);
    TRACE_STR("====== Routing table ready for virtual channel", xchannel_name);

    TRACE_STR("====== Transferring routing table to the internal dir",
	     xchannel_name);
    ntbx_pc_first_global_rank(dir_xchannel_pc, &g_rank_src);
    do
      {
	ntbx_process_grank_t       g_rank_dst  =    0;
	ntbx_process_lrank_t       l_rank_src  =   -1;
	p_ntbx_process_info_t      pi          = NULL;
	p_leo_dir_connection_t     cnx         = NULL;
	p_ntbx_process_container_t pc          = NULL;
	p_ntbx_process_t           process_src = NULL;

	l_rank_src  = ntbx_pc_global_to_local(dir_xchannel_pc, g_rank_src);
	TRACE_VAL("======== source", l_rank_src);
	pi          = ntbx_pc_get_global(dir_xchannel_pc, g_rank_src);
	process_src = pi->process;
	cnx         = pi->specific;
	pc          = cnx->pc;

	ntbx_pc_first_global_rank(dir_xchannel_pc, &g_rank_dst);
	do
	  {
	    p_leo_dir_connection_data_t  cdata       = NULL;
	    ntbx_process_lrank_t         l_rank_dst  =   -1;
	    p_ntbx_process_t             process_dst = NULL;
	    char                        *ref_name    = NULL;

	    l_rank_dst = ntbx_pc_global_to_local(dir_xchannel_pc, g_rank_dst);
	    TRACE_VAL("======== destination", l_rank_dst);

	    {
	      p_leo_path_t               path     = NULL;
	      const ntbx_process_lrank_t location =
		l_rank_src * table_size + l_rank_dst;

	      path   = routing_table_current[location];
	      cdata = leo_dir_connection_data_init();

	      cdata->channel_name = strdup(path->dir_channel->name);
	      cdata->destination_rank = path->dest_rank;
	    }

	    process_dst = ntbx_pc_get_local_process(dir_xchannel_pc,
						    l_rank_dst);
	    {
	      size_t len = 0;

	      len = strlen(dir_xchannel->name) + 8;
	      ref_name = TBX_MALLOC(len);

	      sprintf(ref_name, "%s:%d", dir_xchannel->name, l_rank_src);
	    }

	    ntbx_pc_add(pc, process_dst, l_rank_dst,
			process_src, ref_name, cdata);
	    free(ref_name);
	    ref_name = NULL;
	  }
	while (ntbx_pc_next_global_rank(dir_xchannel_pc, &g_rank_dst));
      }
    while (ntbx_pc_next_global_rank(dir_xchannel_pc, &g_rank_src));

    TRACE("====== Transfer complete");
  }

  tbx_htable_add(dir->xchannel_htable, dir_xchannel->name, dir_xchannel);
  tbx_slist_append(dir->xchannel_slist, dir_xchannel);
  TRACE("====== Processing done");

 end:
  LOG_OUT();
}

// process_application: analyses an application description table returned by
// Leoparse from the application configuration file parsing and builds
// the corresponding directory data structures.
void
process_application(p_leonie_t leonie)
{
  p_tbx_htable_t   application         = NULL;
  p_leo_settings_t settings            = NULL;
  p_tbx_slist_t    include_slist       = NULL;
  p_tbx_slist_t    channel_slist       = NULL;
  p_tbx_slist_t    dummy_host_slist    = NULL;
  p_tbx_slist_t    vchannel_slist      = NULL;
  p_tbx_slist_t    xchannel_slist      = NULL;
  p_leo_networks_t networks            = NULL;

  LOG_IN();
  networks    = leonie->networks;
  settings    = leonie->settings;
  application = leonie->application_htable;

  if (!settings->name)
    {
      settings->name = leoparse_try_read_id(application, "name")?:
	leoparse_read_string(application, "name");
    }

  TRACE_STR("==== Application name", settings->name);

  if (!settings->flavor)
    {
      settings->flavor = leoparse_try_read_id(application, "flavor")?:
	leoparse_read_string(application, "flavor");
    }

  TRACE_STR("==== Application flavor", settings->flavor);

  leonie->application_networks = leoparse_read_htable(application, "networks");
  include_slist        =
    leoparse_read_as_slist(leonie->application_networks, "include");

  if (settings->network_file_slist)
    {
      if (!include_slist)
	{
	  include_slist = tbx_slist_nil();
	}

      tbx_slist_merge_after(include_slist, settings->network_file_slist);
    }

  if (!include_slist || tbx_slist_is_nil(include_slist))
    {
      leo_terminate("parse error : no network include list", settings);
    }

  TRACE("==== Including Network configuration files");
  include_network_files(networks, include_slist);

  dummy_host_slist =
    leoparse_read_as_slist(leonie->application_networks, "hosts");

  if (!dummy_host_slist || tbx_slist_is_nil(dummy_host_slist))
    goto no_dummy_channel;
  tbx_slist_ref_to_head(dummy_host_slist);
  process_dummy_channel(leonie, dummy_host_slist);

 no_dummy_channel:
  channel_slist =
    leoparse_read_as_slist(leonie->application_networks, "channels");

  if ((!channel_slist || tbx_slist_is_nil(channel_slist)) && (!dummy_host_slist || tbx_slist_is_nil(dummy_host_slist)))
    {
      leo_terminate("parse error : no channel list, and no dummy channel list", settings);
    }

  if (channel_slist)
    {
      if (tbx_slist_is_nil(channel_slist))
	{
	  leo_terminate("parse error : empty channel list", settings);
	}

      tbx_slist_ref_to_head(channel_slist);

      do
        {
          p_leoparse_object_t object  = NULL;
          p_tbx_htable_t      channel = NULL;

          object  = tbx_slist_ref_get(channel_slist);
          channel = leoparse_get_htable(object);

          process_channel(leonie, channel);
        }
      while (tbx_slist_ref_forward(channel_slist));
  }

  vchannel_slist =
    leoparse_read_as_slist(leonie->application_networks, "vchannels");

  if (vchannel_slist)
    {
      if (tbx_slist_is_nil(vchannel_slist))
	{
	  leo_terminate("parse error : empty virtual channel list", settings);
	}

      tbx_slist_ref_to_head(vchannel_slist);

      do
	{
	  p_leoparse_object_t object   = NULL;
	  p_tbx_htable_t      vchannel = NULL;

	  object   = tbx_slist_ref_get(vchannel_slist);
	  vchannel = leoparse_get_htable(object);

	  process_vchannel(leonie, vchannel);
	}
      while (tbx_slist_ref_forward(vchannel_slist));
    }
  else
    {
      TRACE("no vchannel_slist");
    }

  xchannel_slist =
    leoparse_read_as_slist(leonie->application_networks, "xchannels");

  if (xchannel_slist)
    {
      if (tbx_slist_is_nil(xchannel_slist))
	{
	  leo_terminate("parse error : empty multiplexing channel list", settings);
	}

      tbx_slist_ref_to_head(xchannel_slist);

      do
	{
	  p_leoparse_object_t object   = NULL;
	  p_tbx_htable_t      xchannel = NULL;

	  object   = tbx_slist_ref_get(xchannel_slist);
	  xchannel = leoparse_get_htable(object);

	  process_xchannel(leonie, xchannel);
	}
      while (tbx_slist_ref_forward(xchannel_slist));
    }
  else
    {
      TRACE("no xchannel_slist");
    }
  TRACE("==== Internal directory generation completed");
  LOG_OUT();
}

