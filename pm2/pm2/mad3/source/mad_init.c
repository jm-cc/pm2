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
 * Mad_init.c
 * ==========
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
void
mad_dir_driver_init(p_mad_madeleine_t madeleine)
{
  p_mad_session_t      session           = NULL;
  p_mad_directory_t    dir               = NULL;
  p_tbx_htable_t       mad_driver_htable = NULL;
  p_tbx_htable_t       dir_driver_htable = NULL;
  ntbx_process_grank_t g      =   -1;
  mad_driver_id_t      mad_driver_id     =    0;

  LOG_IN();
  session           = madeleine->session;
  dir               = madeleine->dir;
  mad_driver_htable = madeleine->driver_htable;
  g      = session->process_rank;
  dir_driver_htable = dir->driver_htable;


  TRACE("Driver initialization: first pass");
  while (1)
    {
      p_mad_driver_t                       mad_driver         = NULL;
      p_mad_driver_interface_t             interface          = NULL;
      p_tbx_htable_t                       mad_adapter_htable = NULL;
      p_mad_dir_driver_t                   dir_driver         = NULL;
      p_ntbx_process_container_t           pc                 = NULL;
      p_ntbx_process_info_t                process_info       = NULL;
      p_mad_dir_driver_process_specific_t  pi_specific        = NULL;
      p_tbx_htable_t                       dir_adapter_htable = NULL;
      char                                *driver_name        = NULL;
      mad_adapter_id_t                     mad_adapter_id     =    0;

      driver_name = mad_leonie_receive_string();
      if (tbx_streq(driver_name, "-"))
        {
          TBX_FREE(driver_name);
          break;
        }

      dir_driver = tbx_htable_get(dir_driver_htable, driver_name);
      if (!dir_driver)
	FAILURE("driver not found");

      mad_driver = tbx_htable_get(mad_driver_htable, driver_name);
      if (!mad_driver)
	FAILURE("driver not available");

      TRACE_STR("Initializing driver", driver_name);
      mad_driver->id         = mad_driver_id++;
      mad_driver->dir_driver = dir_driver;

      interface = mad_driver->interface;
      interface->driver_init(mad_driver);

      mad_leonie_send_int(1);

      mad_adapter_htable = mad_driver->adapter_htable;

      pc                 = dir_driver->pc;
      process_info       = ntbx_pc_get_global(pc, g);
      pi_specific        = process_info->specific;
      dir_adapter_htable = pi_specific->adapter_htable;

      TRACE("Adapter initialization");
      while (1)
	{
	  p_mad_dir_adapter_t  dir_adapter  = NULL;
	  p_mad_adapter_t      mad_adapter  = NULL;
	  char                *adapter_name = NULL;

	  adapter_name = mad_leonie_receive_string();
	  if (tbx_streq(adapter_name, "-"))
            {
              TBX_FREE(adapter_name);
              break;
            }

	  dir_adapter = tbx_htable_get(dir_adapter_htable, adapter_name);
	  if (!dir_adapter)
	    FAILURE("adapter not found");

	  TRACE_STR("Initializing adapter", adapter_name);
	  mad_adapter = mad_adapter_cons();

	  mad_adapter->driver         = mad_driver;
	  mad_adapter->id             = mad_adapter_id++;
	  mad_adapter->selector       = tbx_strdup(dir_adapter->selector);
	  mad_adapter->channel_htable = tbx_htable_empty_table();
	  mad_adapter->dir_adapter    = dir_adapter;

	  tbx_htable_add(mad_adapter_htable, dir_adapter->name, mad_adapter);

	  TRACE_STR("Adapter selector", mad_adapter->selector);
	  interface->adapter_init(mad_adapter);
	  TRACE_STR("Adapter connection parameter",
		   mad_adapter->parameter);
	  mad_leonie_send_string(mad_adapter->parameter);
	  if (!mad_adapter->mtu)
	    {
	      mad_adapter->mtu = 0xFFFFFFFFUL;
	    }
	  mad_leonie_send_unsigned_int(mad_adapter->mtu);
          TBX_FREE(adapter_name);
	}

      TBX_FREE(driver_name);
    }

  TRACE("Driver initialization: second pass");
  while (1)
    {
      p_mad_dir_driver_t          dir_driver  = NULL;
      p_ntbx_process_container_t  pc          = NULL;
      char                       *driver_name = NULL;

      driver_name = mad_leonie_receive_string();
      if (tbx_streq(driver_name, "-"))
        {
          TBX_FREE(driver_name);
          break;
        }

      dir_driver = tbx_htable_get(dir_driver_htable, driver_name);
      if (!dir_driver)
	FAILURE("driver not found");
      pc = dir_driver->pc;

      TRACE_STR("Initializing driver", driver_name);
      while (1)
	{
	  p_ntbx_process_info_t                process_info       = NULL;
	  p_mad_dir_driver_process_specific_t  pi_specific        = NULL;
	  ntbx_process_grank_t                 global_rank        =   -1;
	  p_tbx_htable_t                       adapter_htable     = NULL;

	  global_rank = mad_leonie_receive_int();
	  if (global_rank == -1)
	    break;

	  TRACE_VAL("Process", global_rank);

	  process_info   = ntbx_pc_get_global(pc, global_rank);
	  pi_specific    = process_info->specific;
	  adapter_htable = pi_specific->adapter_htable;

	  while (1)
	    {
	      p_mad_dir_adapter_t  dir_adapter  = NULL;
	      char                *adapter_name = NULL;

	      adapter_name = mad_leonie_receive_string();
	      if (tbx_streq(adapter_name, "-"))
                {
                  TBX_FREE(adapter_name);
                  break;
                }

	      dir_adapter = tbx_htable_get(adapter_htable, adapter_name);
	      if (!dir_adapter)
		FAILURE("adapter not found");

	      TRACE_STR("Adapter", adapter_name);
	      dir_adapter->parameter = mad_leonie_receive_string();
	      TRACE_STR("- parameter", dir_adapter->parameter);
	      dir_adapter->mtu = mad_leonie_receive_unsigned_int();
	      TRACE_VAL("- mtu", dir_adapter->mtu);
	      if (!dir_adapter->mtu)
		FAILURE("invalid mtu");
              TBX_FREE(adapter_name);
	    }
	}

      TBX_FREE(driver_name);
    }

#ifdef MARCEL
  {
    p_mad_driver_t           forwarding_driver = NULL;
    p_mad_driver_interface_t interface         = NULL;

    forwarding_driver  =
      tbx_htable_get(madeleine->driver_htable, "forward");

    interface = forwarding_driver->interface;
    interface->driver_init(forwarding_driver);
  }

  {
    p_mad_driver_t           mux_driver = NULL;
    p_mad_driver_interface_t interface  = NULL;

    mux_driver  =
      tbx_htable_get(madeleine->driver_htable, "mux");

    interface = mux_driver->interface;
    interface->driver_init(mux_driver);
  }
#endif // MARCEL
  LOG_OUT();
}


static
unsigned int
connection_mtu(p_mad_dir_channel_t  channel,
			   ntbx_process_grank_t src,
			   ntbx_process_grank_t dst)
{
  p_mad_dir_driver_t driver  = NULL;
  unsigned int       src_mtu = 0;
  unsigned int       dst_mtu = 0;
  unsigned int       mtu     = 0;

  LOG_IN();
  driver = channel->driver;

  {
    p_mad_dir_driver_process_specific_t  dps     = NULL;
    p_mad_dir_channel_process_specific_t cps     = NULL;
    p_mad_dir_adapter_t                  adapter = NULL;

    dps     = ntbx_pc_get_global_specific(driver->pc, src);
    cps     = ntbx_pc_get_global_specific(channel->pc, src);
    adapter = tbx_htable_get(dps->adapter_htable, cps->adapter_name);

    src_mtu = adapter->mtu;
    if (!src_mtu)
      FAILURE("invalid mtu");
  }

  {
    p_mad_dir_driver_process_specific_t  dps     = NULL;
    p_mad_dir_channel_process_specific_t cps     = NULL;
    p_mad_dir_adapter_t                  adapter = NULL;

    dps     = ntbx_pc_get_global_specific(driver->pc, dst);
    cps     = ntbx_pc_get_global_specific(channel->pc, dst);
    adapter = tbx_htable_get(dps->adapter_htable, cps->adapter_name);

    dst_mtu = adapter->mtu;
    if (!dst_mtu)
      FAILURE("invalid mtu");
  }

  mtu = min(src_mtu, dst_mtu);
  LOG_OUT();

  return mtu;
}

static
unsigned int
compute_mtu(p_mad_directory_t          dir,
                              p_ntbx_process_container_t src_pc,
                              ntbx_process_grank_t       src,
                              ntbx_process_grank_t       dst,
                              const tbx_bool_t           dedicated_fchannel)
{
  p_mad_dir_vxchannel_process_specific_t      vps    = NULL;
  p_ntbx_process_container_t                  dst_pc = NULL;
  p_mad_dir_vxchannel_process_routing_table_t rtable = NULL;
  ntbx_process_grank_t                        med    =   -1;
  unsigned int                                mtu    =    0;

  LOG_IN();
  vps    = ntbx_pc_get_global_specific(src_pc, src);
  dst_pc = vps->pc;
  rtable = ntbx_pc_get_global_specific(dst_pc, dst);
  med    = rtable->destination_rank;

  if (med == dst)
    {
      p_mad_dir_channel_t channel = NULL;

      channel = tbx_htable_get(dir->channel_htable, rtable->channel_name);

      mtu = connection_mtu(channel, src, dst);
    }
  else
    {
      p_mad_dir_channel_t  channel  = NULL;
      unsigned int         mtu1     =    0;
      unsigned int         mtu2     =    0;

      if (dedicated_fchannel)
        {
          p_mad_dir_fchannel_t fchannel = NULL;

          fchannel =
            tbx_htable_get(dir->fchannel_htable, rtable->channel_name);
          channel  =
            tbx_htable_get(dir->channel_htable, fchannel->channel_name);
        }
      else
        {
          channel = tbx_htable_get(dir->channel_htable,  rtable->channel_name);
        }

      mtu1 = connection_mtu(channel, src, med);
      mtu2 = compute_mtu(dir, src_pc, med, dst, dedicated_fchannel);
      mtu  = min(mtu1, mtu2);
    }
  LOG_OUT();

  return mtu;
}

static
p_mad_dir_vxchannel_process_routing_table_t
rtable_get(p_ntbx_process_container_t pc,
           ntbx_process_grank_t       src,
           ntbx_process_grank_t       dst)
{
  p_mad_dir_vxchannel_process_routing_table_t rtable = NULL;
  p_mad_dir_vxchannel_process_specific_t      ps     = NULL;
  p_ntbx_process_container_t                 ppc    = NULL;

  LOG_IN();
  ps     = ntbx_pc_get_global_specific(pc, src);
  ppc    = ps->pc;
  rtable = ntbx_pc_get_global_specific(ppc, dst);
  LOG_OUT();

  return rtable;
}

static
p_mad_dir_vxchannel_process_routing_table_t
reverse_routing(p_ntbx_process_container_t  pc,
                ntbx_process_grank_t       *src,
                ntbx_process_grank_t        dst)
{
  p_mad_dir_vxchannel_process_routing_table_t rtable = NULL;

  LOG_IN();
  rtable = rtable_get(pc, *src, dst);

  if (rtable->destination_rank != dst)
    {
      *src   = rtable->destination_rank;
      rtable = reverse_routing(pc, src, dst);
    }
  LOG_OUT();

  return rtable;
}

static
void
links_init(p_mad_driver_interface_t interface,
                   p_mad_connection_t       cnx)
{
  mad_link_id_t link_id = -1;

  LOG_IN();
  if (!cnx)
    {
      LOG_OUT();

      return;
    }

  cnx->link_array  =
    TBX_CALLOC(cnx->nb_link, sizeof(p_mad_link_t));

  for (link_id = 0; link_id < cnx->nb_link; link_id++)
    {
      p_mad_link_t cnx_link = NULL;

      cnx_link = mad_link_cons();

      cnx_link->connection = cnx;
      cnx_link->id         = link_id;

      cnx->link_array[link_id] = cnx_link;

      if (interface->link_init)
        {
          interface->link_init(cnx_link);
        }
    }
  LOG_OUT();
}

static
void
regular_connection_init(p_mad_driver_interface_t  interface,
                                p_mad_channel_t           channel,
                                ntbx_process_lrank_t      remote_rank,
                                p_mad_connection_t       *p_in,
                                p_mad_connection_t       *p_out)
{
  LOG_IN();
  if (p_in && p_out)
    {
      p_mad_connection_t out = NULL;
      p_mad_connection_t in      = NULL;

      in  = mad_connection_cons();
      out = mad_connection_cons();

      in->nature  = mad_connection_nature_regular;
      out->nature = mad_connection_nature_regular;

      in->remote_rank  = remote_rank;
      out->remote_rank = remote_rank;

      in->channel  = channel;
      out->channel = channel;

      in->reverse  = out;
      out->reverse = in;

      in->regular  = NULL;
      out->regular = NULL;

      in->way  = mad_incoming_connection;
      out->way = mad_outgoing_connection;

      if (interface->connection_init)
        {
          interface->connection_init(in, out);
          if (   (in->nb_link  <= 0)
                 || (in->nb_link != out->nb_link))
            FAILURE("mad_open_channel: invalid link number");
        }
      else
        {
          in->nb_link  = 1;
          out->nb_link = 1;
        }

      links_init(interface, in);
      links_init(interface, out);

      *p_in  = in;
      *p_out = out;
    }
  else
    {
      tbx_bool_t         w   = tbx_false;
      p_mad_connection_t cnx = NULL;

      if (!p_in && !p_out)
        FAILURE("invalid state");

      w = (p_out != NULL);

      cnx = mad_connection_cons();

      cnx->nature      = mad_connection_nature_regular;
      cnx->remote_rank = remote_rank;
      cnx->channel     = channel;
      cnx->way         = w?mad_outgoing_connection:mad_incoming_connection;
      cnx->regular     = NULL;
      cnx->reverse     =
        tbx_darray_expand_and_get(w?channel->in_connection_darray:channel->out_connection_darray,
                                  remote_rank);

      if (cnx->reverse)
        {
          cnx->reverse->reverse = cnx;
        }

      if (interface->connection_init)
        {
          if (w)
            {
              interface->connection_init(NULL, cnx);
            }
          else
            {
              interface->connection_init(cnx, NULL);
            }

          if (cnx->nb_link <= 0)
            FAILURE("mad_open_channel: invalid link number");
        }
      else
        {
          cnx->nb_link = 1;
        }

      links_init(interface, cnx);

      *(w?p_out:p_in) = cnx;
    }

  LOG_OUT();
}

static
p_mad_dir_channel_common_process_specific_t
mad_dir_get_ccps(p_mad_dir_channel_t  dir_channel,
                 ntbx_process_lrank_t remote_rank)
{
  p_ntbx_process_container_t                  cpc  = NULL;
  p_mad_dir_channel_common_process_specific_t ccps = NULL;

  LOG_IN();
  cpc  = dir_channel->common->pc;

  ccps = ntbx_pc_get_local_specific(cpc, remote_rank);

  if (!ccps)
    {
      p_ntbx_process_t remote_process = NULL;
      char             common_name[strlen("common_") +
                                   strlen(dir_channel->name) + 1];

      remote_process =
        ntbx_pc_get_local_process(dir_channel->pc, remote_rank);

      strcpy(common_name, "common_");
      strcat(common_name, dir_channel->name);

      ccps = mad_dir_channel_common_process_specific_cons();
      ntbx_pc_add(cpc, remote_process, remote_rank,
                  dir_channel, common_name, ccps);
    }
  LOG_OUT();

  return ccps;
}

static
tbx_bool_t
connection_init(p_mad_channel_t mad_channel)
{
  p_mad_driver_t           mad_driver  = NULL;
  p_mad_driver_interface_t interface   = NULL;
  p_tbx_darray_t           in_darray   = NULL;
  p_tbx_darray_t           out_darray  = NULL;
  int                      command     =   -1;
  ntbx_process_lrank_t     remote_rank =   -1;

  LOG_IN();
  command = mad_leonie_receive_int();
  if (command == -1)
    {
      TRACE("Pass 2 - request for synchronization");
      LOG_OUT();
      return tbx_false;
    }

  TRACE_VAL("Pass 1 - command", command);

  remote_rank = mad_leonie_receive_int();

  mad_driver = mad_channel->adapter->driver;
  interface  = mad_driver->interface;
  in_darray  = mad_channel->in_connection_darray;
  out_darray = mad_channel->out_connection_darray;

  if (command)
    {
      // Output connection
      p_mad_connection_t out = NULL;

      TRACE_VAL("Initializing connection to", remote_rank);

      if (mad_driver->connection_type == mad_bidirectional_connection)
        {
          out = tbx_darray_expand_and_get(out_darray, remote_rank);

          if (!out)
            {
              p_mad_connection_t in = NULL;

              regular_connection_init(interface, mad_channel, remote_rank, &in, &out);

              tbx_darray_set(out_darray, remote_rank, out);
              tbx_darray_expand_and_set(in_darray, remote_rank, in);
            }
        }
      else
        {
          regular_connection_init(interface, mad_channel, remote_rank, NULL, &out);

          tbx_darray_expand_and_set(out_darray, remote_rank, out);
        }

      TRACE_STR("Out connection parameter", out->parameter);
      mad_leonie_send_string(out->parameter);
    }
  else
    {
      // Input connection
      p_mad_connection_t in = NULL;

      TRACE_VAL("Initializing connection from", remote_rank);

      if (mad_driver->connection_type == mad_bidirectional_connection)
        {
          in = tbx_darray_expand_and_get(in_darray, remote_rank);

          if (!in)
            {
              p_mad_connection_t out = NULL;

              regular_connection_init(interface, mad_channel, remote_rank, &in, &out);

              tbx_darray_set(in_darray, remote_rank, in);
              tbx_darray_expand_and_set(out_darray, remote_rank, out);
            }
        }
      else
        {
          regular_connection_init(interface, mad_channel, remote_rank, &in, NULL);
          tbx_darray_expand_and_set(in_darray, remote_rank, in);
        }

      TRACE_STR("In connection parameter", in->parameter);
      mad_leonie_send_string(in->parameter);
    }

  {
    char                                        *tmp  = NULL;
    p_mad_dir_channel_common_process_specific_t  ccps = NULL;

    ccps = mad_dir_get_ccps(mad_channel->dir_channel, remote_rank);

    tmp = mad_leonie_receive_string();
    TRACE_STR("remote channel parameter", tmp);

    if (!ccps->parameter)
      {
        ccps->parameter = tmp;
      }
    else
      {
        TBX_FREE(tmp);
      }

    tmp = mad_leonie_receive_string();

    if (command)
      {
        TRACE_STR("remote in connection parameter", tmp);

        tbx_darray_expand_and_set(ccps->in_connection_parameter_darray,
                                  mad_channel->process_lrank, tmp);
      }
    else
      {

        TRACE_STR("remote out connection parameter", tmp);

        tbx_darray_expand_and_set(ccps->out_connection_parameter_darray,
                                  mad_channel->process_lrank, tmp);
      }

  }
  TRACE("Pass 1 - sending ack");
  mad_leonie_send_int(-1);

  LOG_OUT();

  return tbx_true;
}

static
p_mad_adapter_info_t
get_adapter_info(p_mad_channel_t      ch,
                                 ntbx_process_lrank_t remote_rank)
{
  p_mad_driver_t                       d   = NULL;
  p_ntbx_process_info_t                rpi = NULL;
  p_ntbx_process_t                     rp  = NULL;
  p_mad_dir_channel_process_specific_t cps = NULL;
  p_mad_dir_driver_process_specific_t  dps = NULL;
  p_mad_adapter_info_t                 ai  = NULL;

  LOG_IN();
  d = ch->adapter->driver;

  rpi = ntbx_pc_get_local(ch->dir_channel->pc, remote_rank);
  rp  = rpi->process;
  cps = rpi->specific;
  dps =
    ntbx_pc_get_global_specific(d->dir_driver->pc, rp->global_rank);

  ai = TBX_MALLOC(sizeof(mad_adapter_info_t));
  ai->dir_node    = tbx_htable_get(rp->ref, "node");
  ai->dir_adapter = tbx_htable_get(dps->adapter_htable,
                                   cps->adapter_name);

  if (!ai->dir_adapter)
    FAILURE("adapter not found");

  LOG_OUT();

  return ai;
}

static
tbx_bool_t
connection_open(p_mad_channel_t mad_channel)
{
  p_mad_driver_t                              mad_driver  = NULL;
  p_mad_driver_interface_t                    interface   = NULL;
  p_tbx_darray_t                              in_darray   = NULL;
  p_tbx_darray_t                              out_darray  = NULL;
  int                                         command     =   -1;
  ntbx_process_lrank_t                        remote_rank =   -1;
  p_mad_adapter_info_t                        rai         = NULL;
  p_ntbx_process_container_t                  cpc         = NULL;
  p_mad_dir_channel_common_process_specific_t ccps        = NULL;

  LOG_IN();
  command = mad_leonie_receive_int();

  if (command == -1)
    {
      TRACE("Pass 3 - request for synchronization");
      LOG_OUT();

      return tbx_false;
    }

  TRACE_VAL("Pass 2 - command", command);

  mad_driver = mad_channel->adapter->driver;
  interface  = mad_driver->interface;
  in_darray  = mad_channel->in_connection_darray;
  out_darray = mad_channel->out_connection_darray;
  cpc        = mad_channel->dir_channel->common->pc;

  remote_rank = mad_leonie_receive_int();
  TRACE_VAL("Pass 2 - remote_rank", remote_rank);

  if (!interface->accept || !interface->connect)
    goto channel_connection_established;

  rai = get_adapter_info(mad_channel, remote_rank);

  ccps = ntbx_pc_get_local_specific(cpc, remote_rank);
  if (!ccps)
    FAILURE("unknown connection");

  rai->channel_parameter = ccps->parameter;

  if (command)
    {
      // Output connection
      p_mad_connection_t out = NULL;

      TRACE_VAL("Connection to", remote_rank);
      out = tbx_darray_get(out_darray, remote_rank);

      rai->connection_parameter =
        tbx_darray_get(ccps->in_connection_parameter_darray, mad_channel->process_lrank);

      if (mad_driver->connection_type == mad_bidirectional_connection)
        {
          if (out->connected)
            goto channel_connection_established;

          interface->connect(out, rai);
          out->reverse->connected = tbx_true;
        }
      else
        {
          interface->connect(out, rai);
        }

      out->connected = tbx_true;
    }
  else
    {
      // Input connection
      p_mad_connection_t in = NULL;

      TRACE_VAL("Accepting connection from", remote_rank);
      in = tbx_darray_expand_and_get(in_darray, remote_rank);

      rai->connection_parameter =
        tbx_darray_get(ccps->out_connection_parameter_darray, mad_channel->process_lrank);

      if (mad_driver->connection_type == mad_bidirectional_connection)
        {
          if (in->connected)
            goto channel_connection_established;

          interface->accept(in, rai);
          in->reverse->connected = tbx_true;
        }
      else
        {
          interface->accept(in, rai);
        }

      in->connected = tbx_true;
    }

 channel_connection_established:
  TRACE("Pass 2 - sending ack");
  mad_leonie_send_int(-1);

  TBX_FREE(rai);

  LOG_OUT();

  return tbx_true;
}

static
tbx_bool_t
channel_open(p_mad_madeleine_t  madeleine,
                     p_mad_channel_id_t p_channel_id)
{
  ntbx_process_grank_t        g =   -1;
  p_mad_dir_channel_t         dir_channel  = NULL;
  p_mad_channel_t             mad_channel  = NULL;
  p_mad_adapter_t             mad_adapter  = NULL;
  p_mad_driver_t              mad_driver   = NULL;
  p_mad_dir_driver_t          dir_driver   = NULL;
  p_mad_driver_interface_t    interface    = NULL;
  char                       *channel_name = NULL;

  LOG_IN();
  channel_name = mad_leonie_receive_string();
  TRACE_STR("Pass 1 - channel", channel_name);

  if (tbx_streq(channel_name, "-"))
    {
      TBX_FREE(channel_name);
      LOG_OUT();

      return tbx_false;
    }

  g = madeleine->session->process_rank;

  dir_channel = tbx_htable_get(madeleine->dir->channel_htable, channel_name);
  if (!dir_channel)
    FAILURE("channel not found");

  mad_driver =
    tbx_htable_get(madeleine->driver_htable, dir_channel->driver->name);
  if (!mad_driver)
    FAILURE("driver not found");

  dir_driver = mad_driver->dir_driver;
  interface  = mad_driver->interface;

  {
    p_mad_dir_channel_process_specific_t cps = NULL;

    cps = ntbx_pc_get_global_specific(dir_channel->pc, g);
    mad_adapter = tbx_htable_get(mad_driver->adapter_htable, cps->adapter_name);
    if (!mad_adapter)
      FAILURE("adapter not found");
  }

  mad_channel                = mad_channel_cons();
  mad_channel->process_lrank =
    ntbx_pc_global_to_local(dir_channel->pc, g);
  mad_channel->type          = mad_channel_type_regular;
  mad_channel->id            = (*p_channel_id)++;
  mad_channel->name          = tbx_strdup(dir_channel->name);
  mad_channel->pc            = dir_channel->pc;
  mad_channel->not_private   = dir_channel->not_private;
  mad_channel->dir_channel   = dir_channel;
  mad_channel->adapter       = mad_adapter;

  if (interface->channel_init)
    interface->channel_init(mad_channel);

  TRACE_STR("Pass 1 - channel connection parameter", mad_channel->parameter);
  mad_leonie_send_string(mad_channel->parameter);

  mad_channel->in_connection_darray  = tbx_darray_init();
  mad_channel->out_connection_darray = tbx_darray_init();

  while (connection_init(mad_channel))
    ;

  TRACE("Pass 2 - sending -1 sync");
  mad_leonie_send_int(-1);

  if (interface->before_open_channel)
    interface->before_open_channel(mad_channel);

  while (connection_open(mad_channel))
    ;

  if (interface->after_open_channel)
    interface->after_open_channel(mad_channel);

  tbx_htable_add(mad_adapter->channel_htable, dir_channel->name, mad_channel);
  tbx_htable_add(madeleine->channel_htable,   dir_channel->name, mad_channel);
  TRACE("Pass 3 - sending -1 sync");
  mad_leonie_send_int(-1);
  TBX_FREE(channel_name);
  LOG_OUT();

  return tbx_true;
}

static
tbx_bool_t
fchannel_open(p_mad_madeleine_t  madeleine,
              p_mad_channel_id_t p_channel_id)
{
  ntbx_process_grank_t        g =   -1;
  p_mad_dir_fchannel_t        dir_fchannel = NULL;
  p_mad_dir_channel_t         dir_channel  = NULL;
  p_mad_channel_t             mad_channel  = NULL;
  p_mad_adapter_t             mad_adapter  = NULL;
  p_mad_driver_t              mad_driver   = NULL;
  p_mad_dir_driver_t          dir_driver   = NULL;
  p_mad_driver_interface_t    interface    = NULL;
  char                       *channel_name = NULL;

  channel_name = mad_leonie_receive_string();
  TRACE_STR("Pass 1 - channel", channel_name);
  if (tbx_streq(channel_name, "-"))
    {
      TBX_FREE(channel_name);
      return tbx_false;
    }

  g = madeleine->session->process_rank;

  dir_fchannel = tbx_htable_get(madeleine->dir->fchannel_htable, channel_name);
  if (!dir_fchannel)
    FAILURE("channel not found");

  dir_channel =
    tbx_htable_get(madeleine->dir->channel_htable, dir_fchannel->channel_name);

  mad_driver  = tbx_htable_get(madeleine->driver_htable,
                               dir_channel->driver->name);
  if (!mad_driver)
    FAILURE("driver not found");

  dir_driver = mad_driver->dir_driver;
  interface  = mad_driver->interface;

  {
    p_mad_dir_channel_process_specific_t cps = NULL;

    cps = ntbx_pc_get_global_specific(dir_channel->pc, g);
    mad_adapter = tbx_htable_get(mad_driver->adapter_htable, cps->adapter_name);

    if (!mad_adapter)
      FAILURE("adapter not found");
  }

  mad_channel                = mad_channel_cons();
  mad_channel->process_lrank =
    ntbx_pc_global_to_local(dir_channel->pc, g);
  mad_channel->type          = mad_channel_type_forwarding;
  mad_channel->id            = (*p_channel_id)++;
  mad_channel->name          = tbx_strdup(dir_fchannel->name);
  mad_channel->pc            = dir_channel->pc;
  mad_channel->not_private   = tbx_false;
  mad_channel->dir_channel   = dir_channel;
  mad_channel->dir_fchannel  = dir_fchannel;
  mad_channel->adapter       = mad_adapter;

  if (interface->channel_init)
    interface->channel_init(mad_channel);

  TRACE_STR("Pass 1 - channel connection parameter", mad_channel->parameter);
  mad_leonie_send_string(mad_channel->parameter);

  mad_channel->in_connection_darray  = tbx_darray_init();
  mad_channel->out_connection_darray = tbx_darray_init();

  while (connection_init(mad_channel))
    ;

  TRACE("Pass 2 - sending -1 sync");
  mad_leonie_send_int(-1);

  if (interface->before_open_channel)
    interface->before_open_channel(mad_channel);

  while (connection_open(mad_channel))
    ;

  if (interface->after_open_channel)
    interface->after_open_channel(mad_channel);

  tbx_htable_add(mad_adapter->channel_htable, dir_fchannel->name, mad_channel);
  tbx_htable_add(madeleine->channel_htable,   dir_fchannel->name, mad_channel);
  mad_leonie_send_int(-1);
  TBX_FREE(channel_name);

  return tbx_true;
}

#ifdef MARCEL
static
void
vchannel_connection_open(p_mad_madeleine_t          madeleine,
                         p_mad_channel_t            mad_channel,
                         p_ntbx_process_container_t vchannel_pc,
                         p_ntbx_process_container_t vchannel_ppc,
                         ntbx_process_grank_t       g_dst)
{
  p_mad_driver_interface_t                    interface = NULL;
  p_mad_dir_vxchannel_process_routing_table_t rt        = NULL;
  p_mad_connection_t                          in        = NULL;
  p_mad_connection_t                          out       = NULL;
  ntbx_process_lrank_t                        l_dst     =   -1;
  ntbx_process_grank_t                        g         =   -1;

  interface = mad_channel->adapter->driver->interface;

  g = madeleine->session->process_rank;

  l_dst = ntbx_pc_global_to_local(vchannel_ppc, g_dst);

  in  = mad_connection_cons();
  out = mad_connection_cons();

  in->remote_rank  = l_dst;
  out->remote_rank = l_dst;

  in->channel  = mad_channel;
  out->channel = mad_channel;

  in->reverse  = out;
  out->reverse = in;

  in->way  = mad_incoming_connection;
  out->way = mad_outgoing_connection;

  // in->regular
  rt = rtable_get(vchannel_pc, g_dst, g);

  {
    ntbx_process_lrank_t l_rt =   -1;
    p_mad_channel_t      ch   = NULL;

    if (rt->destination_rank == g)
      {
        // direct
        in->nature  = mad_connection_nature_direct_virtual;
        ch   = tbx_htable_get(madeleine->channel_htable, rt->channel_name);
        l_rt = ntbx_pc_global_to_local(ch->pc, g_dst);
      }
    else
      {
        // indirect
        ntbx_process_grank_t g_rt =   -1;

        in->nature = mad_connection_nature_indirect_virtual;
        g_rt       = rt->destination_rank;
        rt         = reverse_routing(vchannel_pc, &g_rt, g);

        ch   = tbx_htable_get(madeleine->channel_htable, rt->channel_name);
        l_rt = ntbx_pc_global_to_local(ch->pc, g_rt);
      }

    in->regular = tbx_darray_get(ch->in_connection_darray, l_rt);

    if (!in->regular)
      FAILURE("invalid connection");
  }

  // out->regular
  out->mtu = compute_mtu(madeleine->dir, vchannel_pc, g, g_dst, tbx_true);
  if (!out->mtu)
    FAILURE("invalid MTU");

  rt = ntbx_pc_get_global_specific(vchannel_ppc, g_dst);

  if (rt->destination_rank == g_dst)
    {
      // direct
      out->nature = mad_connection_nature_direct_virtual;
    }
  else
    {
      // indirect
      out->nature = mad_connection_nature_indirect_virtual;
    }

    {
      p_mad_channel_t      ch   = NULL;
      ntbx_process_lrank_t l_rt =   -1;

      ch = tbx_htable_get(madeleine->channel_htable, rt->channel_name);
      l_rt = ntbx_pc_global_to_local(ch->pc, rt->destination_rank);
      out->regular = tbx_darray_get(ch->out_connection_darray, l_rt);
    }

  if (interface->connection_init)
    {
      interface->connection_init(in, out);

      if ((in->nb_link != out->nb_link)||(in->nb_link <= 0))
        FAILURE("mad_open_channel: invalid link number");
    }
  else
    {
      in->nb_link  = 1;
      out->nb_link = 1;
    }

  links_init(interface, in);
  links_init(interface, out);

  tbx_darray_expand_and_set(mad_channel->in_connection_darray, l_dst, in);
  tbx_darray_expand_and_set(mad_channel->out_connection_darray, l_dst, out);
}
#endif /* MARCEL */

static
tbx_bool_t
vchannel_open(p_mad_madeleine_t  madeleine,
              p_mad_channel_id_t p_channel_id)
{
  char                                  *vchannel_name = NULL;
#ifdef MARCEL
  p_mad_adapter_t                        mad_adapter   = NULL;
  p_mad_driver_t                         mad_driver    = NULL;
  p_mad_dir_vchannel_t                   dir_vchannel  = NULL;
  p_ntbx_process_container_t             vchannel_pc   = NULL;
  p_mad_dir_vxchannel_process_specific_t vchannel_ps   = NULL;
  p_ntbx_process_container_t             vchannel_ppc  = NULL;
  p_mad_channel_t                        mad_channel   = NULL;
  ntbx_process_grank_t                   g_rank_dst    =   -1;
  p_mad_driver_interface_t               interface     = NULL;
  ntbx_process_grank_t                   g  =   -1;
#endif // MARCEL

  vchannel_name = mad_leonie_receive_string();
  if (tbx_streq(vchannel_name, "-"))
    {
      TBX_FREE(vchannel_name);
      return tbx_false;
    }

#ifdef MARCEL
  TRACE_STR("Virtual channel", vchannel_name);

  g = madeleine->session->process_rank;

  dir_vchannel =
    tbx_htable_get(madeleine->dir->vchannel_htable, vchannel_name);
  if (!dir_vchannel)
    FAILURE("virtual channel not found");

  mad_driver  = tbx_htable_get(madeleine->driver_htable, "forward");
  if (!mad_driver)
    FAILURE("forwarding driver not found");

  mad_adapter = tbx_htable_get(mad_driver->adapter_htable, "forward");
  if (!mad_adapter)
    FAILURE("forwarding adapter not found");

  vchannel_pc                 = dir_vchannel->pc;
  mad_channel                 = mad_channel_cons();
  mad_channel->process_lrank  =
    ntbx_pc_global_to_local(vchannel_pc, g);
  mad_channel->type           = mad_channel_type_virtual;
  mad_channel->id             = (*p_channel_id)++;
  mad_channel->name           = tbx_strdup(dir_vchannel->name);
  mad_channel->pc             = dir_vchannel->pc;
  mad_channel->not_private    = tbx_true;
  mad_channel->dir_vchannel   = dir_vchannel;
  mad_channel->adapter        = mad_adapter;

  mad_channel->channel_slist  = tbx_slist_nil();
  {
    tbx_slist_ref_to_head(dir_vchannel->dir_channel_slist);
    do
      {
        p_mad_dir_channel_t dir_channel     = NULL;
        p_mad_channel_t     regular_channel = NULL;

        dir_channel = tbx_slist_ref_get(dir_vchannel->dir_channel_slist);
        regular_channel = tbx_htable_get(madeleine->channel_htable,
                                         dir_channel->name);
        if (regular_channel)
          {
            tbx_slist_append(mad_channel->channel_slist, regular_channel);
          }
      }
    while (tbx_slist_ref_forward(dir_vchannel->dir_channel_slist));
  }

  mad_channel->fchannel_slist = tbx_slist_nil();
  {
    tbx_slist_ref_to_head(dir_vchannel->dir_fchannel_slist);
    do
      {
        p_mad_dir_fchannel_t dir_fchannel     = NULL;
        p_mad_channel_t      regular_channel = NULL;

        dir_fchannel = tbx_slist_ref_get(dir_vchannel->dir_fchannel_slist);
        regular_channel = tbx_htable_get(madeleine->channel_htable,
                                         dir_fchannel->name);
        if (regular_channel)
          {
            tbx_slist_append(mad_channel->fchannel_slist, regular_channel);
          }
      }
    while (tbx_slist_ref_forward(dir_vchannel->dir_fchannel_slist));
  }
  interface = mad_driver->interface;

  if (interface->channel_init)
    interface->channel_init(mad_channel);

  mad_channel->in_connection_darray  = tbx_darray_init();
  mad_channel->out_connection_darray = tbx_darray_init();

  // virtual connections construction
  vchannel_ps  = ntbx_pc_get_global_specific(vchannel_pc, g);
  vchannel_ppc = vchannel_ps->pc;

  ntbx_pc_first_global_rank(vchannel_ppc, &g_rank_dst);
  do
    {
      vchannel_connection_open(madeleine,
                               mad_channel,
                               vchannel_pc,
                               vchannel_ppc,
                               g_rank_dst);
    }
  while (ntbx_pc_next_global_rank(vchannel_ppc, &g_rank_dst));

  if (interface->before_open_channel)
    interface->before_open_channel(mad_channel);

  if (interface->after_open_channel)
    interface->after_open_channel(mad_channel);

  tbx_htable_add(mad_adapter->channel_htable, dir_vchannel->name, mad_channel);
  tbx_htable_add(madeleine->channel_htable,   dir_vchannel->name, mad_channel);
#endif // MARCEL

  // Virtual channel ready
  TBX_FREE(vchannel_name);
  mad_leonie_send_string("ok");

  return tbx_true;
}

#ifdef MARCEL
static
void
xchannel_connection_open(p_mad_madeleine_t          madeleine,
                         p_mad_channel_t            mad_channel,
                         p_ntbx_process_container_t xchannel_pc,
                         p_ntbx_process_container_t xchannel_ppc,
                         ntbx_process_grank_t       g_dst)
{
  p_mad_driver_interface_t                    interface = NULL;
  p_mad_dir_vxchannel_process_routing_table_t rt        = NULL;
  p_mad_connection_t                          in        = NULL;
  p_mad_connection_t                          out       = NULL;
  ntbx_process_lrank_t                        l_dst     =   -1;
  ntbx_process_grank_t                        g         =   -1;

  interface = mad_channel->adapter->driver->interface;

  g = madeleine->session->process_rank;

  l_dst = ntbx_pc_global_to_local(xchannel_ppc, g_dst);

  in  = mad_connection_cons();
  out = mad_connection_cons();

  in->remote_rank  = l_dst;
  out->remote_rank = l_dst;

  in->channel  = mad_channel;
  out->channel = mad_channel;

  in->reverse  = out;
  out->reverse = in;

  in->way  = mad_incoming_connection;
  out->way = mad_outgoing_connection;

  // in->regular
  rt = rtable_get(xchannel_pc, g_dst, g);

  {
    p_mad_channel_t      ch   = NULL;
    ntbx_process_lrank_t l_rt =   -1;

    if (rt->destination_rank == g)
      {
        // no routing required

        in->nature  = mad_connection_nature_mux;
        ch   = tbx_htable_get(madeleine->channel_htable, rt->channel_name);
        l_rt = ntbx_pc_global_to_local(ch->pc, g_dst);
      }
    else
      {
        // routing required
        ntbx_process_grank_t g_rt =   -1;

        in->nature  = mad_connection_nature_mux;
        g_rt        = rt->destination_rank;
        rt          = reverse_routing(xchannel_pc, &g_rt, g);
        ch          =
          tbx_htable_get(madeleine->channel_htable, rt->channel_name);
        l_rt        = ntbx_pc_global_to_local(ch->pc, g_rt);
      }

    in->regular = tbx_darray_get(ch->in_connection_darray, l_rt);

    if (!in->regular)
      FAILURE("invalid connection");
  }

  // out->regular
  out->mtu =
    compute_mtu(madeleine->dir, xchannel_pc, g, g_dst, tbx_false);

  if (!out->mtu)
    FAILURE("invalid MTU");

  rt = ntbx_pc_get_global_specific(xchannel_ppc, g_dst);

  {
    p_mad_channel_t      ch   = NULL;
    ntbx_process_lrank_t l_rt =   -1;

    out->nature = mad_connection_nature_mux;
    ch   = tbx_htable_get(madeleine->channel_htable, rt->channel_name);
    l_rt = ntbx_pc_global_to_local(ch->pc, rt->destination_rank);
    out->regular = tbx_darray_get(ch->out_connection_darray, l_rt);
  }

  if (interface->connection_init)
    {
      interface->connection_init(in, out);

      if ((in->nb_link != out->nb_link)||(in->nb_link <= 0))
        FAILURE("mad_open_channel: invalid link number");
    }
  else
    {
      in->nb_link  = 1;
      out->nb_link = 1;
    }

  links_init(interface, in);
  links_init(interface, out);

  tbx_darray_expand_and_set(mad_channel->in_connection_darray,  l_dst, in);
  tbx_darray_expand_and_set(mad_channel->out_connection_darray, l_dst, out);
}
#endif /* MARCEL */

static
tbx_bool_t
xchannel_open(p_mad_madeleine_t  madeleine,
              p_mad_channel_id_t p_channel_id)
{
  char                                  *xchannel_name = NULL;
#ifdef MARCEL
  p_mad_adapter_t                        mad_adapter   = NULL;
  p_mad_driver_t                         mad_driver    = NULL;
  p_mad_dir_xchannel_t                   dir_xchannel  = NULL;
  p_ntbx_process_container_t             xchannel_pc   = NULL;
  p_mad_dir_vxchannel_process_specific_t xchannel_ps   = NULL;
  p_ntbx_process_container_t             xchannel_ppc  = NULL;
  p_mad_channel_t                        mad_channel   = NULL;
  ntbx_process_grank_t                   g_dst         =   -1;
  p_mad_driver_interface_t               interface     = NULL;
  ntbx_process_grank_t                   g             =   -1;
#endif // MARCEL

  LOG_IN();
  xchannel_name = mad_leonie_receive_string();
  if (tbx_streq(xchannel_name, "-"))
    {
      TBX_FREE(xchannel_name);
      return tbx_false;
    }

#ifdef MARCEL
  TRACE_STR("Mux channel", xchannel_name);

  g = madeleine->session->process_rank;

  dir_xchannel =
    tbx_htable_get(madeleine->dir->xchannel_htable, xchannel_name);
  if (!dir_xchannel)
    FAILURE("mux channel not found");

  mad_driver  = tbx_htable_get(madeleine->driver_htable, "mux");
  if (!mad_driver)
    FAILURE("mux driver not found");

  mad_adapter = tbx_htable_get(mad_driver->adapter_htable, "mux");
  if (!mad_adapter)
    FAILURE("mux adapter not found");

  xchannel_pc                     = dir_xchannel->pc;
  mad_channel                     = mad_channel_cons();
  mad_channel->mux_list_darray    = tbx_darray_init();
  mad_channel->mux_channel_darray = tbx_darray_init();
  mad_channel->sub_list_darray    = tbx_darray_init();
  mad_channel->process_lrank      =
    ntbx_pc_global_to_local(xchannel_pc, g);
  mad_channel->type               = mad_channel_type_mux;
  mad_channel->id                 = (*p_channel_id)++;
  mad_channel->name               = dir_xchannel->name;
  mad_channel->pc                 = dir_xchannel->pc;
  mad_channel->not_private        = tbx_true;
  mad_channel->dir_xchannel       = dir_xchannel;
  mad_channel->adapter            = mad_adapter;

  tbx_darray_expand_and_set(mad_channel->mux_channel_darray, 0, mad_channel);
  tbx_darray_expand_and_set(mad_channel->mux_list_darray, 0,
                            mad_channel->sub_list_darray);

  mad_channel->channel_slist      = tbx_slist_nil();
  {
    tbx_slist_ref_to_head(dir_xchannel->dir_channel_slist);
    do
      {
        p_mad_dir_channel_t dir_channel     = NULL;
        p_mad_channel_t     regular_channel = NULL;

        dir_channel     = tbx_slist_ref_get(dir_xchannel->dir_channel_slist);
        regular_channel = tbx_htable_get(madeleine->channel_htable,
                                         dir_channel->name);
        if (regular_channel)
          {
            tbx_slist_append(mad_channel->channel_slist, regular_channel);
          }
      }
    while (tbx_slist_ref_forward(dir_xchannel->dir_channel_slist));
  }

  interface = mad_driver->interface;

  if (interface->channel_init)
    interface->channel_init(mad_channel);

  mad_channel->in_connection_darray  = tbx_darray_init();
  mad_channel->out_connection_darray = tbx_darray_init();

  // mux connections construction
  xchannel_ps  = ntbx_pc_get_global_specific(xchannel_pc, g);
  xchannel_ppc = xchannel_ps->pc;

  ntbx_pc_first_global_rank(xchannel_ppc, &g_dst);
  do
    {
      xchannel_connection_open(madeleine,
                               mad_channel,
                               xchannel_pc,
                               xchannel_ppc,
                               g_dst);
    }
  while (ntbx_pc_next_global_rank(xchannel_ppc, &g_dst));

  if (interface->before_open_channel)
    interface->before_open_channel(mad_channel);

  if (interface->after_open_channel)
    interface->after_open_channel(mad_channel);

  tbx_htable_add(mad_adapter->channel_htable, dir_xchannel->name, mad_channel);
  tbx_htable_add(madeleine->channel_htable,   dir_xchannel->name, mad_channel);

  mad_mux_add_named_sub_channels(mad_channel);
#endif // MARCEL

  // Mux channel ready
  mad_leonie_send_string("ok");
  TBX_FREE(xchannel_name);
  LOG_OUT();

  return tbx_true;
}

void
mad_dir_channel_init(p_mad_madeleine_t madeleine)
{
  mad_channel_id_t channel_id = 0;

  LOG_IN();

  TRACE("Opening channels");
  while (channel_open(madeleine, &channel_id))
    ;

  TRACE("Opening forwarding channels");
  while (fchannel_open(madeleine, &channel_id))
    ;

  TRACE("Opening virtual channels");
  while (vchannel_open(madeleine, &channel_id))
    ;

  TRACE("Opening mux channels");
  while (xchannel_open(madeleine, &channel_id))
    ;

  LOG_OUT();
}

/*
 * Local variables:
 *  c-basic-offset: 2
 *  c-hanging-braces-alist: '((defun-open before after) (class-open before after) (inline-open before after) (block-open before after) (brace-list-open) (brace-entry-open) (substatement-open before after) (block-close . c-snug-do-while) (extern-lang-open before after) (inexpr-class-open before after) (inexpr-class-close before))
 * End:
 */
