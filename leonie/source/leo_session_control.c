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
 * leo_session_control.c
 * =====================
 *
 * - routines responsible for session initialization and monitoring
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
wait_for_ack(p_ntbx_client_t client) {
  int data = 0;

  data = leo_receive_int(client);
  if (data != -1)
    TBX_FAILURE("synchronization error");
}

static
void
str_sync(p_leo_directory_t dir) {
  void _sync(p_ntbx_client_t _client) {
    leo_send_string(_client, "-");
  }

  with_all_processes(dir, _sync);
}

void _int_sync(p_ntbx_client_t _client) {
  leo_send_int(_client, -1);
  wait_for_ack(_client);
}

// init_drivers: initializes the selected network drivers on each session
// process.
void
init_drivers(p_leonie_t leonie)
{
  p_leo_directory_t  dir = NULL;

  void _pass1(void *_dir_driver) {
    p_leo_dir_driver_t dir_driver = _dir_driver;

    void _driver_name(p_ntbx_client_t _client) {
      leo_send_string(_client, dir_driver->network_name);
    }

    void _adapters(p_ntbx_client_t  _client, void *_dps) {
      p_leo_dir_driver_process_specific_t dps = _dps;

      void _adapter_data(void *_dir_adapter) {
        p_leo_dir_adapter_t dir_adapter = _dir_adapter;

        leo_send_string(_client, dir_adapter->name);
        dir_adapter->parameter = leo_receive_string(_client);
        dir_adapter->mtu       = leo_receive_unsigned_int(_client);
      }

      do_slist(dps->adapter_slist, _adapter_data);

      leo_send_string(_client, "-");
    }

    do_pc_send(dir_driver->pc, _driver_name);
    do_pc_send(dir_driver->pc, wait_for_ack);
    do_pc_send_s(dir_driver->pc, _adapters);
  }

  void _pass2(p_ntbx_client_t _client) {
    void _driver(void *_dir_driver) {
      p_leo_dir_driver_t dir_driver  = _dir_driver;

        void _f(ntbx_process_grank_t global_rank, void *_dps) {
            p_leo_dir_driver_process_specific_t dps = _dps;

            void _g(void *_adapter) {
              p_leo_dir_adapter_t adapter = _adapter;

              leo_send_string(_client, adapter->name);

              leo_send_string(_client, adapter->parameter);
              leo_send_unsigned_int(_client, adapter->mtu);
            }

            leo_send_int(_client, global_rank);
            do_slist(dps->adapter_slist, _g);
            leo_send_string(_client, "-");
        }

        leo_send_string(_client, dir_driver->network_name);
        do_pc_global_s(dir_driver->pc, _f);
        leo_send_int(_client, -1);
    }

    do_slist(dir->driver_slist, _driver);
    leo_send_string(_client, "-");
  }

  dir = leonie->directory;

  do_slist(dir->driver_slist, _pass1);
  str_sync(dir);

  with_all_processes(dir, _pass2);
}

// init_channels: initiates the Madeleine III channels network connexions over
// the session. This function also initiates connections for forwarding
// channels and virtual channels.
void
init_channels(p_leonie_t leonie)
{
  void _g(void *_dir_channel) {
    p_leo_dir_channel_t dir_channel = _dir_channel;

    void _f(p_ntbx_client_t client, void *_cnx) {
      p_leo_dir_connection_t cnx = _cnx;

      cnx->parameter = leo_receive_string(client);
    }

    void _src_func1(ntbx_process_lrank_t  sl,
                    p_ntbx_client_t       src,
                    void                 *_cnx_src) {
      p_leo_dir_connection_t cnx_src = _cnx_src;

      void _dst_func1(ntbx_process_lrank_t  dl,
                      p_ntbx_client_t       dst,
                      void                 *_cnx_dst) {
        p_leo_dir_connection_t cnx_dst = _cnx_dst;
        char *src_cnx_parameter = NULL;
        char *dst_cnx_parameter = NULL;

        if (!ntbx_topology_table_get(dir_channel->ttable, sl, dl)) {
          return;
        }

        if (sl == dl) {
          leo_send_int(src, 0);
          leo_send_int(src, dl);

          src_cnx_parameter = leo_receive_string(src);
          tbx_darray_expand_and_set(cnx_src->out_connection_parameter_darray,
                                    dl, src_cnx_parameter);
          leo_send_string(src, cnx_src->parameter);
          leo_send_string(src, src_cnx_parameter);

          wait_for_ack(src);
        } else {
          leo_send_int(src, 0);
          leo_send_int(src, dl);
          leo_send_int(dst, 1);
          leo_send_int(dst, sl);

          src_cnx_parameter = leo_receive_string(src);
          tbx_darray_expand_and_set(cnx_src->out_connection_parameter_darray,
                                    dl, src_cnx_parameter);

          dst_cnx_parameter = leo_receive_string(dst);
          tbx_darray_expand_and_set(cnx_dst->in_connection_parameter_darray,
                                    sl, dst_cnx_parameter);

          leo_send_string(src, cnx_dst->parameter);
          leo_send_string(src, dst_cnx_parameter);
          leo_send_string(dst, cnx_src->parameter);
          leo_send_string(dst, src_cnx_parameter);

          wait_for_ack(src);
          wait_for_ack(dst);
        }
      }

      do_pc_send_local_s(dir_channel->pc, _dst_func1);
    }

    void _src_func2(ntbx_process_lrank_t sl,
                    p_ntbx_client_t      src) {
      void _dst_func2(ntbx_process_lrank_t  dl,
                      p_ntbx_client_t       dst) {

        if (!ntbx_topology_table_get(dir_channel->ttable, sl, dl)) {
          return;
        }

        if (sl == dl) {
          leo_send_int(src, 0);
          leo_send_int(src, dl);
          wait_for_ack(src);
        } else {
          leo_send_int(src, 0);
          leo_send_int(src, dl);
          leo_send_int(dst, 1);
          leo_send_int(dst, sl);

          wait_for_ack(src);
          wait_for_ack(dst);
        }
      }

      do_pc_send_local(dir_channel->pc, _dst_func2);
    }


    do_pc_send_string  (dir_channel->pc, dir_channel->name);
    do_pc_send_s       (dir_channel->pc, _f);
    do_pc_send_local_s (dir_channel->pc, _src_func1);
    do_pc_send         (dir_channel->pc, _int_sync);
    do_pc_send_local   (dir_channel->pc, _src_func2);
    do_pc_send         (dir_channel->pc, _int_sync);
  }

  do_slist(leonie->directory->channel_slist, _g);
  str_sync(leonie->directory);
}

void
init_fchannels(p_leonie_t leonie)
{
  void _g(void *_dir_fchannel) {

    p_leo_dir_fchannel_t       dir_fchannel = _dir_fchannel;
    p_leo_dir_channel_t        dir_channel  = NULL;

    void _f(p_ntbx_client_t client, void *_cnx) {
      p_leo_dir_connection_t cnx = _cnx;

      leo_send_string(client, dir_fchannel->name);
      cnx->parameter = leo_receive_string(client);
    }

    void _src_func1(ntbx_process_lrank_t  sl,
                    p_ntbx_client_t       src,
                    void                 *_cnx_src) {
      p_leo_dir_connection_t cnx_src = _cnx_src;

      void _dst_func1(ntbx_process_lrank_t  dl,
                      p_ntbx_client_t       dst,
                      void                 *_cnx_dst) {
        p_leo_dir_connection_t cnx_dst = _cnx_dst;
        char *src_cnx_parameter = NULL;
        char *dst_cnx_parameter = NULL;

        if (!ntbx_topology_table_get(dir_channel->ttable, sl, dl))
          return;

        leo_send_int(src, 0);
        leo_send_int(src, dl);
        leo_send_int(dst, 1);
        leo_send_int(dst, sl);

        src_cnx_parameter = leo_receive_string(src);
        tbx_darray_expand_and_set(cnx_src->out_connection_parameter_darray,
                                  dl, src_cnx_parameter);

        dst_cnx_parameter = leo_receive_string(dst);
        tbx_darray_expand_and_set(cnx_dst->in_connection_parameter_darray,
                                  sl, dst_cnx_parameter);

        leo_send_string(src, cnx_dst->parameter);
        leo_send_string(src, dst_cnx_parameter);
        leo_send_string(dst, cnx_src->parameter);
        leo_send_string(dst, src_cnx_parameter);

        wait_for_ack(src);
        wait_for_ack(dst);
      }

      do_pc_send_local_s(dir_fchannel->pc, _dst_func1);
    }

    void _src_func2(ntbx_process_lrank_t sl,
                    p_ntbx_client_t      src) {
      void _dst_func2(ntbx_process_lrank_t  dl,
                      p_ntbx_client_t       dst) {

        if (!ntbx_topology_table_get(dir_channel->ttable, sl, dl))
          return;

        leo_send_int(src, 0);
        leo_send_int(src, dl);
        leo_send_int(dst, 1);
        leo_send_int(dst, sl);

        wait_for_ack(src);
        wait_for_ack(dst);
      }

      do_pc_send_local(dir_fchannel->pc, _dst_func2);
    }

    dir_channel = tbx_htable_get(leonie->directory->channel_htable,
                                 dir_fchannel->channel_name);
    do_pc_send_s       (dir_fchannel->pc, _f);
    do_pc_send_local_s (dir_fchannel->pc, _src_func1);
    do_pc_send         (dir_fchannel->pc, _int_sync);
    do_pc_send_local   (dir_fchannel->pc, _src_func2);
    do_pc_send         (dir_fchannel->pc, _int_sync);
  }

  do_slist(leonie->directory->fchannel_slist, _g);
  str_sync(leonie->directory);
}


void
init_vchannels(p_leonie_t leonie)
{
  void _g(void *_dir_vchannel) {
    p_leo_dir_vxchannel_t dir_vchannel = _dir_vchannel;

    void _f(p_ntbx_client_t client) {
      leo_send_string(client, dir_vchannel->name);
    }

    do_pc_send(dir_vchannel->pc, _f);
    do_pc_send(dir_vchannel->pc, wait_for_ack);
  }

  do_slist(leonie->directory->vchannel_slist, _g);
  str_sync(leonie->directory);
}

void
init_xchannels(p_leonie_t leonie)
{
  void _g(void *_dir_xchannel) {
    p_leo_dir_vxchannel_t dir_xchannel = _dir_xchannel;

    void _f(p_ntbx_client_t client) {
      leo_send_string(client, dir_xchannel->name);
    }

    do_pc_send(dir_xchannel->pc, _f);
    do_pc_send(dir_xchannel->pc, wait_for_ack);
  }

  do_slist(leonie->directory->xchannel_slist, _g);
  str_sync(leonie->directory);
}

// exit_session: has to purposes:
// - MadIII/Leonie interactions functions (data display, synchronizations)
// - session termination synchronization.
void
exit_session(p_leonie_t leonie)
{
  p_leo_directory_t  dir           = NULL;
  p_ntbx_client_t   *client_array  = NULL;
  int                nb_clients    =    0;
  tbx_bool_t         finishing     = tbx_false;
  int                barrier_count =    0;

  dir = leonie->directory;

  nb_clients    = tbx_slist_get_length(dir->process_slist);
  client_array  = TBX_CALLOC(nb_clients, sizeof(ntbx_client_t));

  /* Sauvegarde du nombre de client pour attente des processus fils */
  leonie->settings->nbclient = nb_clients;

  {
    int i = 0;
    void _fill_array(p_ntbx_client_t client) {
      client_array[i++] = client;
    }

    with_all_processes(dir, _fill_array);
  }

  while (nb_clients) {
    int status = -1;

    status = ntbx_tcp_read_poll(nb_clients, client_array);

    if (status > 0) {
      int j = 0;

      while (status && (j < nb_clients)) {

        p_ntbx_client_t    client      = NULL;
        int                read_status = ntbx_failure;
        int                data        = 0;
        ntbx_pack_buffer_t pack_buffer;

        client = client_array[j];

        if (!client->read_ok) {
          j++;
          continue;
        }

        read_status = ntbx_tcp_read_pack_buffer(client, &pack_buffer);
        if (read_status == -1)
          TBX_FAILURE("control link failure");

        data = ntbx_unpack_int(&pack_buffer);

        switch (data) {
        case leo_command_end: {
          status--;
          nb_clients--;
          memmove(client_array + j, client_array + j + 1,
                  (nb_clients - j) * sizeof(p_ntbx_client_t));
          finishing = tbx_true;
        } break;

        case leo_command_print: {
	  int with_newline = 0;
          char *string = NULL;

	  with_newline = leo_receive_int(client);
          string = leo_receive_string(client);
	  if (with_newline) {
	    fprintf(stderr, "%s\n", string);
	  }
	  else {
	    fprintf(stderr, "%s", string);
	  }
          free(string);
          string = NULL;
          status--;
          j++;
        } break;

        case leo_command_barrier: {
          if (finishing)
            TBX_FAILURE("barrier request during session clean-up");

          barrier_count++;

          if (barrier_count >= nb_clients) {
            if (barrier_count == nb_clients) {
              void _barrier_passed(p_ntbx_client_t tmp_client) {
                leo_send_int(tmp_client, leo_command_barrier_passed);
              }

              with_all_processes(dir, _barrier_passed);
              barrier_count = 0;
            } else
              TBX_FAILURE("incoherent behaviour");
          }

          status --;
          j++;
        } break;

        case leo_command_beat:
          leo_send_int(client, leo_command_beat_ack);
          status--;
          j++;
          break;

        default:
          TBX_FAILUREF("unknown command [%d] or synchronization error", data);
        }
      }

      if (status)
        TBX_FAILURE("incoherent behaviour");
    }
  }
}
