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
 * leo_spawn.c
 * ===========
 *
 * - loader-independant session building routines
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#define NEED_LEO_HELPERS
#include "leonie.h"

// connect_processes: establishes the network connections between a set of
// processes of the session and the Leonie server.
// The 'process_slist' argument contains the list of the set of processes
// to connect to Leonie. For identifications purposes, each process has to
// send its host name to Leonie. The matching between the host name sent
// by the client and the host names availables in the internal directory
// is performed by this function, possibly by normalizing and/or trying
// host name aliases.

static
p_ntbx_process_t
find_process(p_tbx_htable_t    node_htable,
             p_ntbx_client_t   client,
             char            **p_host_name)
{
  p_ntbx_process_t  process   = NULL;
  p_tbx_slist_t     slist     = NULL;
  char             *true_name = NULL;

  slist = tbx_htable_get(node_htable, *p_host_name);
  if (slist)
    goto found;

  true_name = ntbx_true_name(*p_host_name);
  slist     = tbx_htable_get(node_htable, true_name);
  if (slist)
    {
      TBX_FREE(*p_host_name);
      *p_host_name = true_name;
      true_name = NULL;
      goto found;
    }

    {

      char *alias = client->remote_host;

      slist = tbx_htable_get(node_htable, alias);
      if (slist)
        {
          TBX_FREE(*p_host_name);
          *p_host_name = tbx_strdup(alias);
          goto found;
        }

      true_name = ntbx_true_name(alias);
      slist = tbx_htable_get(node_htable, true_name);
      if (slist)
        {
          TBX_FREE(*p_host_name);
          *p_host_name = true_name;
          true_name = NULL;
          goto found;
        }

    }

  if (tbx_slist_is_nil(client->remote_alias))
    TBX_FAILURE("invalid client answer");

  tbx_slist_ref_to_head(client->remote_alias);
  do
    {
      char *alias = NULL;

      alias = tbx_slist_ref_get(client->remote_alias);

      slist = tbx_htable_get(node_htable, alias);
      if (slist)
        {
          TBX_FREE(*p_host_name);
          *p_host_name = tbx_strdup(alias);
          goto found;
        }

      true_name = ntbx_true_name(alias);
      slist = tbx_htable_get(node_htable, true_name);
      if (slist)
        {
          TBX_FREE(*p_host_name);
          *p_host_name = true_name;
          true_name = NULL;
          goto found;
        }

    }
  while (tbx_slist_ref_forward(client->remote_alias));

  TBX_FAILURE("client hostname not found");

 found:
  process = tbx_slist_extract(slist);

  if (tbx_slist_is_nil(slist))
    {
      slist = tbx_htable_extract(node_htable, *p_host_name);
      tbx_slist_free(slist);
      slist = NULL;
    }

  return process;
}

static
void
connect_processes(p_leonie_t    leonie,
		  p_tbx_slist_t process_slist)
{
  p_ntbx_server_t net_server  = NULL;
  p_tbx_htable_t  node_htable = NULL;

  void _build_host_list(void *_process) {
    p_ntbx_process_t  process   = _process;
    p_leo_dir_node_t  dir_node  = NULL;
    p_tbx_slist_t     slist     = NULL;

    dir_node = tbx_htable_get(process->ref, "node");
    slist    = tbx_htable_get(node_htable, dir_node->name);

    if (!slist)
      {
        slist = tbx_slist_nil();
        tbx_htable_add(node_htable, dir_node->name, slist);
      }

    tbx_slist_append(slist, process);
  }

  if (tbx_slist_is_nil(process_slist)) {
    return;
  }

  net_server  = leonie->net_server;
  node_htable = leo_htable_init();

  do_slist(process_slist, _build_host_list);

  do
    {
      p_ntbx_client_t           client           = NULL;
      char                     *host_name        = NULL;
      p_ntbx_process_t          process          = NULL;
      p_leo_process_specific_t  process_specific = NULL;
      int                       status           = ntbx_failure;

      client = ntbx_client_cons();
      ntbx_tcp_client_init(client);
      status = ntbx_tcp_server_accept(net_server, client);
      if (status == ntbx_failure)
	TBX_FAILURE("client failed to connect");

      host_name = leo_receive_string(client);

      process = find_process(node_htable, client, &host_name);

      process_specific = process->specific;
      process_specific->client = client;

      leo_send_int(client, process->global_rank);

      //backward compatibility with leony
      //session id
      leo_send_int(client, 0);
       
      TBX_FREE(host_name);
    }
  while (!tbx_htable_empty(node_htable));

  tbx_htable_free(node_htable);
}

// spawn_processes: spawns the session processes by groups using the static
// variable 'spawn_groups'. Each group may use a different loader function.
void
spawn_processes(p_leonie_t leonie)
{
  p_leo_spawn_groups_t spawn_groups = NULL;
  p_tbx_htable_t       loaders      = NULL;
  p_leo_settings_t     settings     = NULL;
  p_ntbx_server_t      net_server   = NULL;

  spawn_groups = leonie->spawn_groups;
  loaders      = leonie->loaders;
  settings     = leonie->settings;
  net_server   = leonie->net_server;

  tbx_slist_ref_to_head(spawn_groups->slist);
  do
    {
      p_leo_spawn_group_t spawn_group = NULL;
      p_leo_loader_t      loader      = NULL;

      spawn_group = tbx_slist_ref_get(spawn_groups->slist);

      if (tbx_slist_is_nil(spawn_group->process_slist))
	continue;

      loader = tbx_htable_get(loaders, spawn_group->loader_name);
      if (!loader)
	TBX_FAILURE("loader unavailable");

      loader->loader_func(settings,
			  net_server,
			  spawn_group->process_slist);

      connect_processes(leonie, spawn_group->process_slist);
    }
  while (tbx_slist_ref_forward(spawn_groups->slist));
}

// send_directory: sends the internal directory structure to each session
// process.
static
void
send_processes(p_leo_directory_t dir,
               p_ntbx_client_t   client)
{
  void _send_data(void *_dir_process) {
    p_ntbx_process_t dir_process = _dir_process;

    leo_send_int(client, dir_process->global_rank);
  }

  int len = 0;

  len = tbx_slist_get_length(dir->process_slist);
  if (len <= 0)
    TBX_FAILURE("invalid number of processes");

  leo_send_int(client, len);
  do_slist(dir->process_slist, _send_data);
  leo_send_string(client, "end{processes}");
}

static
void
send_nodes(p_leo_directory_t dir,
           p_ntbx_client_t   client)
{
  void _send_data(void *_dir_node) {
    p_leo_dir_node_t dir_node = _dir_node;

    void _f(ntbx_process_lrank_t l) {
      ntbx_process_grank_t g = -1;
      g = ntbx_pc_local_to_global(dir_node->pc, l);
      leo_send_int(client, g);
      leo_send_int(client, l);
    }

    leo_send_string(client, dir_node->name);
    do_pc_local(dir_node->pc, _f);
    leo_send_int(client, -1);
  }

  int len = 0;

  len = tbx_slist_get_length(dir->node_slist);
  if (len <= 0)
    TBX_FAILURE("invalid number of nodes");

  leo_send_int(client, len);
  do_slist(dir->node_slist, _send_data);
  leo_send_string(client, "end{nodes}");
}

static
void
send_adapters(p_tbx_slist_t   adapter_slist,
              p_ntbx_client_t client)
{
  void _send_adapter_data(void *_dir_adapter) {
    p_leo_dir_adapter_t dir_adapter = _dir_adapter;

    leo_send_string(client, dir_adapter->name);
    leo_send_string(client, dir_adapter->selector);
  }

  int len = 0;

  len = tbx_slist_get_length(adapter_slist);
  leo_send_int(client, len);
  do_slist(adapter_slist, _send_adapter_data);
}

static
void
send_drivers(p_leo_directory_t dir,
             p_ntbx_client_t   client)
{
  void _send_driver_data(void *_dir_driver) {
    p_leo_dir_driver_t dir_driver = _dir_driver;

    void _process_specific(ntbx_process_lrank_t l, void *_dps) {
      p_leo_dir_driver_process_specific_t dps = _dps;
      ntbx_process_grank_t                g   = -1;

      g = ntbx_pc_local_to_global(dir_driver->pc, l);
      leo_send_int(client, g);
      leo_send_int(client, l);
      send_adapters(dps->adapter_slist, client);
      leo_send_string(client, dps->parameter);
    }

    leo_send_string(client, dir_driver->network_name);
    leo_send_string(client, dir_driver->device_name);
    do_pc_local_s(dir_driver->pc, _process_specific);
    leo_send_int(client, -1);
  }

  int len = 0;

  len = tbx_slist_get_length(dir->driver_slist);

  if (len < 0)
    TBX_FAILURE("invalid number of drivers");

  leo_send_int(client, len);
  do_slist(dir->driver_slist, _send_driver_data);
  leo_send_string(client, "end{drivers}");
}

static
void
send_channels(p_leo_directory_t dir,
              p_ntbx_client_t   client)
{
  void _send_channel_data(void *_dir_channel) {
    p_leo_dir_channel_t dir_channel = _dir_channel;

    void _f(ntbx_process_lrank_t l, void *_cnx) {
      p_leo_dir_connection_t cnx = _cnx;
      ntbx_process_grank_t                 g   = -1;

      g = ntbx_pc_local_to_global(dir_channel->pc, l);
      leo_send_int(client, g);
      leo_send_int(client, l);
      leo_send_string(client, cnx->adapter_name);
    }

    void _g(ntbx_process_lrank_t sl,
            ntbx_process_lrank_t dl) {
      if (ntbx_topology_table_get(dir_channel->ttable, sl, dl)) {
        leo_send_int(client, 1);
      } else {
        leo_send_int(client, 0);
      }
    }

    leo_send_string(client, dir_channel->name);
    leo_send_unsigned_int(client, dir_channel->public);
    leo_send_unsigned_int(client, 0 ); // mergeable ; backward compatibility
    leo_send_string(client, dir_channel->driver->network_name);

    do_pc_local_s(dir_channel->pc, _f);
    leo_send_int(client, -1);
    do_pc2_local(dir_channel->pc, _g);
  }

  int len = 0;

  len = tbx_slist_get_length(dir->channel_slist);

  if (len < 0)
    TBX_FAILURE("invalid number of channels");

  leo_send_int(client, len);
  do_slist(dir->channel_slist, _send_channel_data);
  leo_send_string(client, "end{channels}");
}

void
send_fchannels(p_leo_directory_t dir,
               p_ntbx_client_t   client)
{
  void _send_fchannel_data(void *_dir_fchannel) {
    p_leo_dir_fchannel_t dir_fchannel = _dir_fchannel;

    void _f(ntbx_process_lrank_t l, void *_cnx) {
      ntbx_process_grank_t                 g   = -1;

      g = ntbx_pc_local_to_global(dir_fchannel->pc, l);
      leo_send_int(client, g);
      leo_send_int(client, l);
      leo_send_string(client, "default");
    }

    leo_send_string(client, dir_fchannel->name);
    leo_send_string(client, dir_fchannel->channel_name);

    do_pc_local_s(dir_fchannel->pc, _f);
    leo_send_int(client, -1);
  }

  int len = 0;

  len = tbx_slist_get_length(dir->fchannel_slist);

  if (len < 0)
    TBX_FAILURE("invalid number of forwarding channels");

  leo_send_int(client, len);
  do_slist(dir->fchannel_slist, _send_fchannel_data);
  leo_send_string(client, "end{fchannels}");
}

static
void
send_table(p_leo_dir_vxchannel_t dir_vxchannel,
           p_ntbx_client_t       client)
{
  void _cnx_src(ntbx_process_grank_t sg, ntbx_process_lrank_t sl, void *_cnx) {
    p_leo_dir_connection_t cnx  = _cnx;

    void _cnx_dst(ntbx_process_grank_t dg, ntbx_process_lrank_t dl, void *_cdata) {
      p_leo_dir_connection_data_t cdata =
        _cdata;

      leo_send_int   (client, dg);
      leo_send_int   (client, dl);
      leo_send_string(client, cdata->channel_name);
      leo_send_int   (client, cdata->destination_rank);
    }

    leo_send_int(client, sg);
    leo_send_int(client, sl);
    do_pc_global_local_s(cnx->pc, _cnx_dst);
    leo_send_int(client, -1);
  }

  do_pc_global_local_s(dir_vxchannel->pc, _cnx_src);
}

void
send_vchannels(p_leo_directory_t dir,
               p_ntbx_client_t   client)
{
  void _send_vchannel_data(void *_dir_vchannel) {
      void _send_channel(void *_dir_channel) {
        p_leo_dir_channel_t dir_channel = _dir_channel;
        leo_send_string(client, dir_channel->name);
      }

      void _send_fchannel(void *_dir_fchannel) {
        p_leo_dir_fchannel_t dir_fchannel = _dir_fchannel;
        leo_send_string(client, dir_fchannel->name);
      }

    p_leo_dir_vxchannel_t dir_vchannel = _dir_vchannel;
    int                   len          = 0;

    leo_send_string(client, dir_vchannel->name);

    len = tbx_slist_get_length(dir_vchannel->dir_channel_slist);
    leo_send_int(client, len);
    do_slist(dir_vchannel->dir_channel_slist, _send_channel);

    len = tbx_slist_get_length(dir_vchannel->dir_fchannel_slist);
    leo_send_int(client, len);
    do_slist(dir_vchannel->dir_fchannel_slist, _send_fchannel);

    send_table(dir_vchannel, client);
    leo_send_int(client, -1);
  }

  int len = 0;

  len = tbx_slist_get_length(dir->vchannel_slist);

  if (len < 0)
    TBX_FAILURE("invalid number of virtual channels");

  leo_send_int(client, len);
  do_slist(dir->vchannel_slist, _send_vchannel_data);
  leo_send_string(client, "end{vchannels}");
}

void
send_xchannels(p_leo_directory_t dir,
               p_ntbx_client_t   client)
{
  void _send_xchannel_data(void *_dir_xchannel) {
    void _send_channel(void *_dir_channel) {
      p_leo_dir_channel_t dir_channel = _dir_channel;
      leo_send_string(client, dir_channel->name);
    }

    void _send_sub_channel_name(void *_name) {
      leo_send_string(client, _name);
    }

    p_leo_dir_vxchannel_t dir_xchannel = _dir_xchannel;
    int                   len          = 0;

    leo_send_string(client, dir_xchannel->name);

    len = tbx_slist_get_length(dir_xchannel->dir_channel_slist);
    leo_send_int(client, len);
    do_slist(dir_xchannel->dir_channel_slist, _send_channel);

    len = tbx_slist_get_length(dir_xchannel->sub_channel_name_slist);
    leo_send_int(client, len);
    do_slist(dir_xchannel->sub_channel_name_slist, _send_sub_channel_name);

    send_table(dir_xchannel, client);
    leo_send_int(client, -1);
  }

  int len = 0;

  len = tbx_slist_get_length(dir->xchannel_slist);

  if (len < 0)
    TBX_FAILURE("invalid number of mux channels");

  leo_send_int(client, len);
  do_slist(dir->xchannel_slist, _send_xchannel_data);
  leo_send_string(client, "end{xchannels}");
}

void
send_directory(p_leonie_t leonie)
{
  p_leo_directory_t  dir         = NULL;
  p_tbx_slist_nref_t process_ref = NULL;

  dir = leonie->directory;

  process_ref = tbx_slist_nref_alloc(dir->process_slist);

  tbx_slist_nref_to_head(process_ref);
  do
    {
      p_ntbx_client_t  client  = NULL;
      p_ntbx_process_t process = NULL;
      //p_tbx_slist_t    slist   = NULL;
      //int              len     =    0;

      process = tbx_slist_nref_get(process_ref);

      client = process_to_client(process);
      ntbx_tcp_client_setup_delay(client);

      // Processes
      send_processes(dir, client);

      // Nodes
      send_nodes(dir, client);

      // Drivers
      send_drivers(dir, client);

      // Channels
      send_channels(dir, client);

      // Forwarding channels
      send_fchannels(dir, client);

      // Virtual channels
      send_vchannels(dir, client);

      // MultipleXing channels
      send_xchannels(dir, client);

      ntbx_tcp_client_setup_nodelay(client);

      // End
      leo_send_string(client, "end{directory}");
     }
  while (tbx_slist_nref_forward(process_ref));

  tbx_slist_nref_free(process_ref);

}

