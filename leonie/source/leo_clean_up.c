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
 * leonie_clean_up.c
 * =================
 *
 * - session disconnect routines
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

static
void
wait_for_ack(p_ntbx_client_t client)
{
  int data = 0;

  data = leo_receive_int(client);
  if (data != -1)
    TBX_FAILURE("synchronization error");
}

static
void
str_sync(p_leo_directory_t dir)
{
  void _sync(p_ntbx_client_t _client)
    {
      leo_send_string(_client, "-");
    }

  with_all_processes(dir, _sync);
}

static
void
channel_disconnect(p_leo_dir_vxchannel_t vxchannel,
                   p_leo_dir_channel_t   channel)
{
  {
    void _d(ntbx_process_lrank_t dl, p_ntbx_client_t dst) {
      tbx_bool_t _s(ntbx_process_lrank_t sl, p_ntbx_client_t src) {
        if (sl == dl)
          return tbx_true;

        if (!ntbx_topology_table_get(channel->ttable, sl, dl))
          return tbx_true;

        leo_send_string(src, vxchannel->name);
        leo_send_string(src, channel->name);
        leo_send_int(src, dl);

        leo_send_string(dst, vxchannel->name);
        leo_send_string(dst, channel->name);
        leo_send_int(dst, -1);

        wait_for_ack(src);
        wait_for_ack(dst);

        return tbx_false;
      }

      do_pc_cond_send_local(channel->pc, _s);
    }

    do_pc_send_local(channel->pc, _d);
  }
}

static
void
fchannel_disconnect(p_leo_dir_vxchannel_t vchannel,
                    p_leo_dir_fchannel_t  fchannel,
                    p_leo_dir_channel_t   channel)
{
  {
    void _d(ntbx_process_lrank_t dl, p_ntbx_client_t dst) {
      tbx_bool_t _s(ntbx_process_lrank_t sl, p_ntbx_client_t src) {
        if (sl == dl)
          return tbx_true;

        if (!ntbx_topology_table_get(channel->ttable, sl, dl))
          return tbx_true;

        leo_send_string(src, vchannel->name);
        leo_send_string(src, fchannel->name);
        leo_send_int(src, dl);

        leo_send_string(dst, vchannel->name);
        leo_send_string(dst, fchannel->name);
        leo_send_int(dst, -1);

        wait_for_ack(src);
        wait_for_ack(dst);

        return tbx_false;
      }

      do_pc_cond_send_local(channel->pc, _s);
    }

    do_pc_send_local(channel->pc, _d);
  }
}

// dir_vchannel_disconnect: manages virtual channels disconnection
void
dir_vchannel_disconnect(p_leonie_t leonie)
{
  p_leo_directory_t dir = leonie->directory;

  void int_sync(p_ntbx_client_t _client)
    {
      leo_send_int(_client, 1);
      wait_for_ack(_client);
    }

  void _virtual(void *_p)
    {
      p_leo_dir_vxchannel_t vchannel = _p;

      void _regular(void *_p1)
        {
          p_leo_dir_channel_t channel = _p1;
          channel_disconnect(vchannel, channel);
        }

      void _forward(void *_p2)
        {
          p_leo_dir_fchannel_t fchannel = _p2;
          p_leo_dir_channel_t  channel  = NULL;

          channel = tbx_htable_get(dir->channel_htable,
                                    fchannel->channel_name);
          fchannel_disconnect(vchannel, fchannel, channel);
        }

      do_slist(vchannel->dir_channel_slist,  _regular);
      do_slist(vchannel->dir_fchannel_slist, _forward);
    }

  with_all_processes(dir, int_sync);
  do_slist(dir->vchannel_slist, _virtual);
  str_sync(dir);
}


// dir_xchannel_disconnect: manages multipleXing channels disconnection
void
dir_xchannel_disconnect(p_leonie_t leonie)
{
  p_leo_directory_t dir = leonie->directory;

  void int_sync(p_ntbx_client_t _client)
    {
      wait_for_ack(_client);
    }

  void _mux(void *_p)
    {
      p_leo_dir_vxchannel_t xchannel = _p;

      void _regular(void *_p1)
        {
          p_leo_dir_channel_t channel = _p1;
          channel_disconnect(xchannel, channel);
        }

      do_slist(xchannel->dir_channel_slist, _regular);
    }

  with_all_processes(dir, int_sync);
  do_slist(dir->xchannel_slist, _mux);
  str_sync(dir);
}

static
void
barrier(p_ntbx_process_container_t pc)
{
  void _f(p_ntbx_client_t _client)
  {
      leo_send_int(_client, -1);
      wait_for_ack(_client);
  }

  do_pc_send(pc, _f);
}

static
void
vchannel_exit(void *_p)
{
  p_leo_dir_vxchannel_t      dir_vchannel = _p;
  p_ntbx_process_container_t pc           = NULL;
  int                        src_cmd      = -1;
  int                        dst_cmd      = -1;

  void _process_command() {
    void _s(ntbx_process_lrank_t sl, p_ntbx_client_t src) {
      void _d(ntbx_process_lrank_t dl, p_ntbx_client_t dst) {
        leo_send_int(src, src_cmd);
        leo_send_int(src, dl);

        leo_send_int(dst, dst_cmd);
        leo_send_int(dst, sl);

        wait_for_ack(src);
        wait_for_ack(dst);
      }

      do_pc_send_local(pc, _d);
    }

    do_pc_send_local(pc, _s);
  }

  pc = dir_vchannel->pc;

  do_pc_send_string(pc, dir_vchannel->name);

  src_cmd = 0;
  dst_cmd = 1;
  _process_command();
  barrier(pc);

  src_cmd = 1;
  dst_cmd = 0;
  _process_command();
  barrier(pc);
}

// dir_vchannel_exit: manages virtual channels data structures clean-up
static
void
dir_vchannel_exit(p_leo_directory_t dir)
{
  do_slist(dir->vchannel_slist, vchannel_exit);
  str_sync(dir);
}

static
void
xchannel_exit(void *_p)
{
  p_leo_dir_vxchannel_t      dir_xchannel = _p;
  p_ntbx_process_container_t pc           = NULL;
  int                        src_cmd      = -1;
  int                        dst_cmd      = -1;

  void _process_command() {
    void _s(ntbx_process_lrank_t sl, p_ntbx_client_t src) {
      void _d(ntbx_process_lrank_t dl, p_ntbx_client_t dst) {
        leo_send_int(src, src_cmd);
        leo_send_int(src, dl);

        leo_send_int(dst, dst_cmd);
        leo_send_int(dst, sl);

        wait_for_ack(src);
        wait_for_ack(dst);
      }

      do_pc_send_local(pc, _d);
    }

    do_pc_send_local(pc, _s);
  }

  pc = dir_xchannel->pc;

  do_pc_send_string(pc, dir_xchannel->name);

  src_cmd = 0;
  dst_cmd = 1;
  _process_command();
  barrier(pc);

  src_cmd = 1;
  dst_cmd = 0;
  _process_command();
  barrier(pc);
}

static
void
dir_xchannel_exit(p_leo_directory_t dir)
{
  do_slist(dir->xchannel_slist, xchannel_exit);
  str_sync(dir);
}

static
void
dir_fchannel_exit(p_leo_directory_t dir)
{
  void fchannel_exit(void *_p) {
    p_leo_dir_fchannel_t       dir_fchannel = _p;
    p_leo_dir_channel_t        dir_channel  = NULL;
    p_ntbx_process_container_t pc           = NULL;
    int                        src_cmd      = -1;
    int                        dst_cmd      = -1;

    void _process_command() {
      void _s(ntbx_process_lrank_t sl, p_ntbx_client_t src) {
        void _d(ntbx_process_lrank_t dl, p_ntbx_client_t dst) {
          if (!ntbx_topology_table_get(dir_channel->ttable, sl, dl))
            return;

          leo_send_int(src, src_cmd);
          leo_send_int(src, dl);

          leo_send_int(dst, dst_cmd);
          leo_send_int(dst, sl);

          wait_for_ack(src);
          wait_for_ack(dst);
        }

        do_pc_send_local(pc, _d);
      }

      do_pc_send_local(pc, _s);
    }

    dir_channel = tbx_htable_get(dir->channel_htable,
                                 dir_fchannel->channel_name);
    pc          = dir_channel->pc;

    do_pc_send_string(pc, dir_fchannel->name);

    src_cmd = 0;
    dst_cmd = 1;
    _process_command();
    barrier(pc);

    src_cmd = 1;
    dst_cmd = 0;
    _process_command();
    barrier(pc);
  }

  do_slist(dir->fchannel_slist, fchannel_exit);
  str_sync(dir);
}

// dir_channel_exit: manages regular channels data structures clean-up
static
void
channel_exit(void *_p)
{
  p_leo_dir_channel_t        dir_channel = _p;
  p_ntbx_process_container_t pc          = NULL;
  int                        src_cmd     = -1;
  int                        dst_cmd     = -1;

  void _process_command() {
    void _s(ntbx_process_lrank_t sl, p_ntbx_client_t src) {
      void _d(ntbx_process_lrank_t dl, p_ntbx_client_t dst) {
        if (!ntbx_topology_table_get(dir_channel->ttable, sl, dl))
          return;

        leo_send_int(src, src_cmd);
        leo_send_int(src, dl);

        leo_send_int(dst, dst_cmd);
        leo_send_int(dst, sl);

        wait_for_ack(src);
        wait_for_ack(dst);
      }

      do_pc_send_local(pc, _d);
    }

    do_pc_send_local(pc, _s);
  }

  pc = dir_channel->pc;

  do_pc_send_string(pc, dir_channel->name);

  src_cmd = 0;
  dst_cmd = 1;
  _process_command();
  barrier(pc);

  src_cmd = 1;
  dst_cmd = 0;
  _process_command();
  barrier(pc);
}

static
void
dir_channel_exit(p_leo_directory_t dir)
{
  do_slist(dir->channel_slist, channel_exit);
  str_sync(dir);
}

// dir_driver_exit: manages driver data structures clean-up
static
void
driver_exit_pass1(void *_dir_driver)
{
  p_leo_dir_driver_t dir_driver = _dir_driver;

  void _adapters(p_ntbx_client_t _client, void *_p) {
    p_leo_dir_driver_process_specific_t dps = _p;

    void _f(void *_q) {
      p_leo_dir_adapter_t dir_adapter = _q;
      leo_send_string(_client, dir_adapter->name);
    }

    do_slist(dps->adapter_slist, _f);
    leo_send_string(_client, "-");
  }

  do_pc_send_string(dir_driver->pc, dir_driver->network_name);
  do_pc_send_s     (dir_driver->pc, _adapters);
  do_pc_send       (dir_driver->pc, wait_for_ack);
}

static
void
driver_exit_pass2(void *_dir_driver)
{
  p_leo_dir_driver_t dir_driver = _dir_driver;

  do_pc_send_string(dir_driver->pc, dir_driver->network_name);
  do_pc_send(dir_driver->pc, wait_for_ack);
}

static
void
dir_driver_exit(p_leo_directory_t dir)
{

  do_slist(dir->driver_slist, driver_exit_pass1);
  str_sync(dir);

  do_slist(dir->driver_slist, driver_exit_pass2);
  str_sync(dir);
}

// dir_vchannel_cleanup: manages local virtual channels data structures
// clean-up
static
void
dir_vchannel_cleanup(p_leo_directory_t dir)
{
  void _h(void *_r) {
      p_leo_dir_vxchannel_t dir_vchannel = _r;

      void _g(void *_q) {
        p_leo_dir_connection_t cnx = _q;

        void _f(void *_p) {
          p_leo_dir_connection_data_t cnx_data = _p;
          TBX_FREE2(cnx_data->channel_name);
        }

        do_pc_s(cnx->pc, _f);
        ntbx_pc_dest(cnx->pc, tbx_default_specific_dest);
        cnx->pc = NULL;
      }

      tbx_htable_extract(dir->vchannel_htable, dir_vchannel->name);

      tbx_slist_clear(dir_vchannel->dir_channel_slist);
      tbx_slist_free(dir_vchannel->dir_channel_slist);
      dir_vchannel->dir_channel_slist = NULL;

      tbx_slist_clear(dir_vchannel->dir_fchannel_slist);
      tbx_slist_free(dir_vchannel->dir_fchannel_slist);
      dir_vchannel->dir_fchannel_slist = NULL;

      do_pc_s(dir_vchannel->pc, _g);

      ntbx_pc_dest(dir_vchannel->pc, tbx_default_specific_dest);
      dir_vchannel->pc   = NULL;

      TBX_FREE2(dir_vchannel->name);
      TBX_FREE(dir_vchannel);
    }

  do_extract_slist(dir->vchannel_slist, _h);

  tbx_slist_free(dir->vchannel_slist);
  dir->vchannel_slist = NULL;

  tbx_htable_free(dir->vchannel_htable);
  dir->vchannel_htable = NULL;
}

// dir_vchannel_cleanup: manages local virtual channels data structures
// clean-up
static
void
dir_xchannel_cleanup(p_leo_directory_t dir)
{
  void _h(void *_r) {
    p_leo_dir_vxchannel_t dir_xchannel   = _r;

    void _g(void *_q) {
      p_leo_dir_connection_t cnx = _q;

      void _f(void *_p) {
        p_leo_dir_connection_data_t cnx_data = _p;
        TBX_FREE2(cnx_data->channel_name);
      }

      do_pc_s(cnx->pc, _f);
      ntbx_pc_dest(cnx->pc, tbx_default_specific_dest);
      cnx->pc = NULL;
    }

    tbx_htable_extract(dir->xchannel_htable, dir_xchannel->name);

    tbx_slist_clear(dir_xchannel->dir_channel_slist);
    tbx_slist_free(dir_xchannel->dir_channel_slist);
    dir_xchannel->dir_channel_slist = NULL;

    do_pc_s(dir_xchannel->pc, _g);

    ntbx_pc_dest(dir_xchannel->pc, tbx_default_specific_dest);
    dir_xchannel->pc = NULL;

    TBX_FREE2(dir_xchannel->name);
    TBX_FREE(dir_xchannel);
  }

  do_extract_slist(dir->xchannel_slist, _h);

  tbx_slist_free(dir->xchannel_slist);
  dir->xchannel_slist = NULL;

  tbx_htable_free(dir->xchannel_htable);
  dir->xchannel_htable = NULL;
}

// dir_fchannel_cleanup: manages local forwarding channels data structures
// clean-up
static
void
dir_fchannel_cleanup(p_leo_directory_t dir)
{
  void _f(void *_p)
    {
      p_leo_dir_fchannel_t dir_fchannel = _p;

      tbx_htable_extract(dir->fchannel_htable, dir_fchannel->name);

      TBX_FREE2(dir_fchannel->name);
      TBX_FREE2(dir_fchannel->channel_name);
      TBX_FREE(dir_fchannel);
    }

  do_extract_slist(dir->fchannel_slist, _f);

  tbx_slist_free(dir->fchannel_slist);
  dir->fchannel_slist = NULL;

  tbx_htable_free(dir->fchannel_htable);
  dir->fchannel_htable = NULL;
}

// dir_channel_cleanup: manages local regular channels data structures
// clean-up
static
void
dir_channel_cleanup(p_leo_directory_t dir)
{
  void _g(void *_q) {
    p_leo_dir_channel_t dir_channel = _q;

    void _f(void *_p) {
      p_leo_dir_connection_t cnx = _p;

      if (cnx) {
        TBX_CFREE2(cnx->adapter_name);
      }
    }

    tbx_htable_extract(dir->channel_htable, dir_channel->name);

    ntbx_topology_table_exit(dir_channel->ttable);
    dir_channel->ttable = NULL;

    do_pc_s(dir_channel->pc, _f);
    ntbx_pc_dest(dir_channel->pc, tbx_default_specific_dest);
    dir_channel->pc = NULL;

    TBX_FREE2(dir_channel->name);

    dir_channel->driver = NULL;
    dir_channel->public = tbx_false;

    TBX_FREE(dir_channel);
  }

  do_extract_slist(dir->channel_slist, _g);

  tbx_slist_free(dir->channel_slist);
  dir->channel_slist = NULL;

  tbx_htable_free(dir->channel_htable);
  dir->channel_htable = NULL;
}

static
void
dps_cleanup(void *_s)
{
  p_leo_dir_driver_process_specific_t dps = _s;

  void _f(void *_p) {
    p_leo_dir_adapter_t dir_adapter = _p;

    tbx_htable_extract(dps->adapter_htable, dir_adapter->name);

    TBX_FREE2(dir_adapter->name);
    TBX_CFREE2(dir_adapter->selector);
    TBX_CFREE2(dir_adapter->parameter);
    dir_adapter->mtu = 0;
    TBX_FREE(dir_adapter);
  }

  if (dps) {
    do_extract_slist(dps->adapter_slist, _f);

    tbx_slist_free(dps->adapter_slist);
    dps->adapter_slist = NULL;

    tbx_htable_free(dps->adapter_htable);
    dps->adapter_htable = NULL;
  }
}

static
void
dir_driver_cleanup(p_leo_directory_t dir)
{
  void _f(void *_p) {
      p_leo_dir_driver_t dir_driver = _p;

      tbx_htable_extract(dir->driver_htable, dir_driver->network_name);

      do_pc_s(dir_driver->pc, dps_cleanup);
      ntbx_pc_dest(dir_driver->pc, tbx_default_specific_dest);
      dir_driver->pc = NULL;

      TBX_FREE(dir_driver->network_name);
      dir_driver->network_name = NULL;

      TBX_FREE(dir_driver->device_name);
      dir_driver->device_name = NULL;

      TBX_FREE(dir_driver);
    }

  do_extract_slist(dir->driver_slist, _f);

  tbx_slist_free(dir->driver_slist);
  dir->driver_slist = NULL;

  tbx_htable_free(dir->driver_htable);
  dir->driver_htable = NULL;
}

// dir_node_cleanup: manages local node data structures clean-up
static
void
dir_node_cleanup(p_leo_directory_t dir)
{
  void _f(void *_p)
    {
      p_leo_dir_node_t dir_node = _p;

      tbx_htable_extract(dir->node_htable, dir_node->name);

      ntbx_pc_dest(dir_node->pc, tbx_default_specific_dest);
      dir_node->pc = NULL;

      TBX_FREE(dir_node->name);
      dir_node->name = NULL;

      TBX_FREE(dir_node);
    }

  do_extract_slist(dir->node_slist, _f);

  tbx_slist_free(dir->node_slist);
  dir->node_slist = NULL;

  tbx_htable_free(dir->node_htable);
  dir->node_htable = NULL;
}

// dir_process_cleanup: manages local process data structures clean-up
static
void
dir_process_cleanup(p_leo_directory_t dir)
{
  void _f(void *_p) {
      p_ntbx_process_t process = _p;

      void _g(void *key) {
	  tbx_htable_extract(process->ref, key);
	  TBX_FREE(key);
      }

      p_tbx_slist_t keys = NULL;

      keys = tbx_htable_get_key_slist(process->ref);
      do_extract_slist(keys, _g);
      tbx_slist_free(keys);
      tbx_htable_free(process->ref);
      process->ref = NULL;
  }

  do_extract_slist(dir->process_slist, _f);
  tbx_slist_free(dir->process_slist);
  dir->process_slist = NULL;
}

// directory_exit: local directory data structure clean-up
void
directory_exit(p_leonie_t leonie)
{
  void _f(p_ntbx_client_t _client) {
    leo_send_string(_client, "clean{directory}");
  }

  p_leo_directory_t dir   = NULL;

  dir = leonie->directory;

  dir_vchannel_exit(dir);
  dir_xchannel_exit(dir);
  dir_fchannel_exit(dir);
  dir_channel_exit(dir);
  dir_driver_exit(dir);

  with_all_processes(dir, _f);

  dir_vchannel_cleanup(dir);
  dir_xchannel_cleanup(dir);
  dir_fchannel_cleanup(dir);
  dir_channel_cleanup(dir);
  dir_driver_cleanup(dir);
  dir_node_cleanup(dir);
  dir_process_cleanup(dir);
}


void
wait_processes(p_leonie_t leonie)
{
  p_leo_settings_t     settings     = NULL;

  int                  nb_clients   =    0;
  int                  nb_clientsl  =    0;
  int                  pid;

  settings     = leonie->settings;

  nb_clients   = settings->nbclient;
  pid          = (-getpid());

  while(nb_clientsl < nb_clients)
    {
      int                   ret;
      int                   status;

      ret = waitpid(-1, &status, 0);

      if (ret == -1) {
	leo_error("waitpid", settings);
      }

      nb_clientsl++;
    }

}

