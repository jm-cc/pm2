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

static
tbx_bool_t
adapter_init(p_mad_driver_t mad_driver,
             p_tbx_htable_t mad_adapter_htable,
             p_tbx_htable_t dir_adapter_htable)
{
  p_mad_dir_adapter_t  dir_adapter  = NULL;
  p_mad_adapter_t      mad_adapter  = NULL;
  char                *adapter_name = NULL;

  LOG_IN();
  adapter_name = mad_leonie_receive_string();
  if (tbx_streq(adapter_name, "-"))
    {
      TBX_FREE(adapter_name);
      LOG_OUT();

      return tbx_false;
    }

  dir_adapter = tbx_htable_get(dir_adapter_htable, adapter_name);
  if (!dir_adapter)
    TBX_FAILURE("adapter not found");

  TRACE_STR("Initializing adapter", adapter_name);
  mad_adapter = mad_adapter_cons();
  mad_adapter->id             = tbx_htable_get_size(mad_adapter_htable);
  mad_adapter->driver         = mad_driver;
  mad_adapter->selector       = tbx_strdup(dir_adapter->selector);
  mad_adapter->channel_htable = tbx_htable_empty_table();
  mad_adapter->dir_adapter    = dir_adapter;

  tbx_htable_add(mad_adapter_htable, dir_adapter->name, mad_adapter);

  TRACE_STR("Adapter selector", mad_adapter->selector);
  mad_driver->interface->adapter_init(mad_adapter);
  TRACE_STR("Adapter connection parameter", mad_adapter->parameter);
  mad_leonie_send_string(mad_adapter->parameter);
  mad_leonie_send_unsigned_int(0xFFFFFFFFUL);	/* mtu (unused) */
  TBX_FREE(adapter_name);
  LOG_OUT();

  return tbx_true;
}

static
tbx_bool_t
driver_init_1(p_mad_madeleine_t      madeleine,
              p_tbx_htable_t         mad_device_htable,
              p_tbx_htable_t         mad_network_htable,
              p_tbx_htable_t         dir_driver_htable,
              ntbx_process_grank_t   g,
              int                   *p_argc,
              char                ***p_argv)
{
  p_mad_driver_t                       mad_driver         = NULL;
  p_mad_driver_interface_t             interface          = NULL;
  p_tbx_htable_t                       mad_adapter_htable = NULL;
  p_mad_dir_driver_t                   dir_driver         = NULL;
  p_mad_dir_driver_process_specific_t  pi_specific        = NULL;
  p_tbx_htable_t                       dir_adapter_htable = NULL;
  char                                *network_name       = NULL;
  char                                *device_name        = NULL;

  network_name = mad_leonie_receive_string();
  if (tbx_streq(network_name, "-"))
    {
      TBX_FREE(network_name);
      LOG_OUT();

      return tbx_false;
    }

  dir_driver = tbx_htable_get(dir_driver_htable, network_name);
  if (!dir_driver)
    TBX_FAILURE("driver instance not found");

  mad_driver = tbx_htable_get(mad_network_htable, network_name);

  if (mad_driver)
    TBX_FAILUREF("driver instance %s already initialized", network_name);

  device_name = dir_driver->device_name;

  TRACE("Initializing driver instance %s of %s", network_name, device_name);
  interface = tbx_htable_get(mad_device_htable, device_name);
  if (!interface)
    TBX_FAILUREF("driver %s not available", device_name);

  mad_driver = mad_driver_cons();

  mad_driver->madeleine      = madeleine;
  mad_driver->network_name   = tbx_strdup(network_name);
  mad_driver->device_name    = tbx_strdup(device_name);
  mad_driver->adapter_htable = tbx_htable_empty_table();
  mad_driver->interface      = interface;

  tbx_htable_add(mad_network_htable, network_name, mad_driver);

  mad_driver->dir_driver = dir_driver;

  mad_driver->process_lrank = ntbx_pc_global_to_local(dir_driver->pc, g);

  interface->driver_init(mad_driver, p_argc, p_argv);

  mad_leonie_send_int(-1);

  mad_adapter_htable = mad_driver->adapter_htable;

  pi_specific        = ntbx_pc_get_global_specific(dir_driver->pc, g);
  dir_adapter_htable = pi_specific->adapter_htable;

  TRACE("Adapter initialization");
  while (adapter_init(mad_driver, mad_adapter_htable, dir_adapter_htable))
    ;

  TBX_FREE(network_name);
  LOG_OUT();

  return tbx_true;
}

static
tbx_bool_t
driver_init_2(p_tbx_htable_t dir_driver_htable)
{
  p_mad_dir_driver_t  dir_driver  = NULL;
  char               *driver_name = NULL;

  LOG_IN();
  driver_name = mad_leonie_receive_string();
  if (tbx_streq(driver_name, "-"))
    {
      TBX_FREE(driver_name);
      LOG_OUT();

      return tbx_false;
    }

  dir_driver = tbx_htable_get(dir_driver_htable, driver_name);
  if (!dir_driver)
    TBX_FAILURE("driver not found");

  TRACE_STR("Initializing driver", driver_name);
  while (1)
    {
      p_mad_dir_driver_process_specific_t pi_specific    = NULL;
      ntbx_process_grank_t                global_rank    =   -1;
      p_tbx_htable_t                      adapter_htable = NULL;

      global_rank = mad_leonie_receive_int();
      if (global_rank == -1)
        break;

      TRACE_VAL("Process", global_rank);

      pi_specific    = ntbx_pc_get_global_specific(dir_driver->pc, global_rank);
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
            TBX_FAILURE("adapter not found");

          TRACE_STR("Adapter", adapter_name);
          dir_adapter->parameter = mad_leonie_receive_string();
          TRACE_STR("- parameter", dir_adapter->parameter);
          mad_leonie_receive_unsigned_int();	/* mtu (unused) */
          TBX_FREE(adapter_name);
        }
    }

  TBX_FREE(driver_name);
  LOG_OUT();

  return tbx_true;
}

void
mad_dir_driver_init(p_mad_madeleine_t    madeleine,
                    int                 *p_argc,
                    char              ***p_argv)
{
  p_tbx_htable_t mad_device_htable  = NULL;
  p_tbx_htable_t mad_network_htable = NULL;
  p_tbx_htable_t dir_driver_htable  = NULL;

  LOG_IN();
  mad_device_htable  = madeleine->device_htable;
  mad_network_htable = madeleine->network_htable;
  dir_driver_htable  = madeleine->dir->driver_htable;


  TRACE("Driver initialization: first pass");
  while (driver_init_1(madeleine,
                       mad_device_htable, mad_network_htable, dir_driver_htable,
                       madeleine->session->process_rank, p_argc, p_argv))
    ;

  TRACE("Driver initialization: second pass");
  while (driver_init_2(dir_driver_htable))
    ;
#if 0
//#ifdef MARCEL
  {
    p_mad_driver_t            mad_driver   = NULL;
    p_mad_driver_interface_t  interface    = NULL;
    const char               *device_name  = "forward";
    const char               *network_name = "forward_network";

    interface = tbx_htable_get(mad_device_htable, device_name);
    mad_driver = mad_driver_cons();

    mad_driver->madeleine      = madeleine;
    mad_driver->network_name   = tbx_strdup(network_name);
    mad_driver->device_name    = tbx_strdup(device_name);
    mad_driver->adapter_htable = tbx_htable_empty_table();
    mad_driver->interface      = interface;

    tbx_htable_add(mad_network_htable, network_name, mad_driver);

    mad_driver->interface->driver_init(mad_driver, NULL, NULL);
  }

  {
    p_mad_driver_t            mad_driver   = NULL;
    p_mad_driver_interface_t  interface    = NULL;
    const char               *device_name  = "mux";
    const char               *network_name = "mux_network";

    interface = tbx_htable_get(mad_device_htable, device_name);
    mad_driver = mad_driver_cons();

    mad_driver->madeleine      = madeleine;
    mad_driver->network_name   = tbx_strdup(network_name);
    mad_driver->device_name    = tbx_strdup(device_name);
    mad_driver->adapter_htable = tbx_htable_empty_table();
    mad_driver->interface      = interface;

    tbx_htable_add(mad_network_htable, network_name, mad_driver);

    mad_driver->interface->driver_init(mad_driver, NULL, NULL);
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
    p_mad_dir_connection_t dir_connection     = NULL;
    p_mad_dir_adapter_t                  adapter = NULL;

    dps     = ntbx_pc_get_global_specific(driver->pc, src);
    dir_connection     = ntbx_pc_get_global_specific(channel->pc, src);
    adapter = tbx_htable_get(dps->adapter_htable, dir_connection->adapter_name);

    src_mtu = adapter->mtu;
    if (!src_mtu)
      TBX_FAILURE("invalid mtu");
  }

  {
    p_mad_dir_driver_process_specific_t  dps     = NULL;
    p_mad_dir_connection_t dir_connection     = NULL;
    p_mad_dir_adapter_t                  adapter = NULL;

    dps     = ntbx_pc_get_global_specific(driver->pc, dst);
    dir_connection     = ntbx_pc_get_global_specific(channel->pc, dst);
    adapter = tbx_htable_get(dps->adapter_htable, dir_connection->adapter_name);

    dst_mtu = adapter->mtu;
    if (!dst_mtu)
      TBX_FAILURE("invalid mtu");
  }

  mtu = tbx_min(src_mtu, dst_mtu);
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
  p_mad_dir_connection_t dir_connection    = NULL;
  p_ntbx_process_container_t           dst_pc = NULL;
  p_mad_dir_connection_data_t          cdata  = NULL;
  ntbx_process_grank_t                 med    =   -1;
  unsigned int                         mtu    =    0;

  LOG_IN();
  dir_connection    = ntbx_pc_get_global_specific(src_pc, src);
  dst_pc = dir_connection->pc;
  cdata    = ntbx_pc_get_global_specific(dst_pc, dst);
  med    = cdata->destination_rank;

  if (med == dst)
    {
      p_mad_dir_channel_t channel = NULL;

      channel = tbx_htable_get(dir->channel_htable, cdata->channel_name);

      mtu = connection_mtu(channel, src, dst);
    }
  else
    {
      p_mad_dir_channel_t channel  = NULL;
      unsigned int        mtu1     =    0;
      unsigned int        mtu2     =    0;

      if (dedicated_fchannel)
        {
          p_mad_dir_channel_t fchannel = NULL;

          fchannel =
            tbx_htable_get(dir->channel_htable, cdata->channel_name);
          channel  =
            tbx_htable_get(dir->channel_htable, fchannel->cloned_channel_name);
        }
      else
        {
          channel = tbx_htable_get(dir->channel_htable, cdata->channel_name);
        }

      mtu1 = connection_mtu(channel, src, med);
      mtu2 = compute_mtu(dir, src_pc, med, dst, dedicated_fchannel);
      mtu  = tbx_min(mtu1, mtu2);
    }
  LOG_OUT();

  return mtu;
}

static
p_mad_dir_connection_data_t
connection_data_get(p_ntbx_process_container_t pc,
                    ntbx_process_grank_t       src,
                    ntbx_process_grank_t       dst)
{
  p_mad_dir_connection_t dir_connection   = NULL;
  p_ntbx_process_container_t           ppc   = NULL;
  p_mad_dir_connection_data_t          cdata = NULL;

  LOG_IN();
  dir_connection = ntbx_pc_get_global_specific(pc, src);
  ppc = dir_connection->pc;
  cdata = ntbx_pc_get_global_specific(ppc, dst);
  LOG_OUT();

  return cdata;
}

static
p_mad_dir_connection_data_t
reverse_routing(p_ntbx_process_container_t  pc,
                ntbx_process_grank_t       *src,
                ntbx_process_grank_t        dst)
{
  p_mad_dir_connection_data_t cdata = NULL;

  LOG_IN();
  cdata = connection_data_get(pc, *src, dst);

  if (cdata->destination_rank != dst)
    {
      *src   = cdata->destination_rank;
      cdata = reverse_routing(pc, src, dst);
    }
  LOG_OUT();

  return cdata;
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
            TBX_FAILURE("mad_open_channel: invalid link number");
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
        TBX_FAILURE("invalid state");

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
            TBX_FAILURE("mad_open_channel: invalid link number");
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

//static
tbx_bool_t
connection_init(p_mad_channel_t mad_channel)
{
  p_mad_driver_t            mad_driver  = NULL;
  p_mad_driver_interface_t  interface   = NULL;
  p_tbx_darray_t            in_darray   = NULL;
  p_tbx_darray_t            out_darray  = NULL;
  int                       command     =   -1;
  ntbx_process_lrank_t      remote_rank =   -1;
 p_ntbx_process_container_t pc          = NULL;

  LOG_IN();
  pc = (mad_channel->cloned_dir_channel)?mad_channel->cloned_dir_channel->pc:mad_channel->dir_channel->pc;

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
    char                                 *tmp = NULL;
    p_mad_dir_connection_t  dir_connection = NULL;

    dir_connection = ntbx_pc_get_local_specific(pc, remote_rank);

    tmp = mad_leonie_receive_string();
    TRACE_STR("remote channel parameter", tmp);

    if (!dir_connection->channel_parameter)
      {
        dir_connection->channel_parameter = tmp;
      }
    else
      {
        TBX_FREE(tmp);
      }

    tmp = mad_leonie_receive_string();

    if (command)
      {
        TRACE_STR("remote in connection parameter", tmp);
        dir_connection->in_parameter = tmp;
      }
    else
      {

        TRACE_STR("remote out connection parameter", tmp);
        dir_connection->out_parameter = tmp;
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
  p_mad_dir_connection_t dir_connection = NULL;
  p_mad_dir_driver_process_specific_t  dps = NULL;
  p_mad_adapter_info_t                 ai  = NULL;
  p_ntbx_process_container_t           pc  = NULL;

  LOG_IN();
  d = ch->adapter->driver;

  pc = (ch->cloned_dir_channel)?ch->cloned_dir_channel->pc:ch->dir_channel->pc;
  rpi = ntbx_pc_get_local(pc, remote_rank);
  rp  = rpi->process;
  dir_connection = rpi->specific;
  dps =
    ntbx_pc_get_global_specific(d->dir_driver->pc, rp->global_rank);

  ai = TBX_MALLOC(sizeof(mad_adapter_info_t));
  ai->dir_node    = tbx_htable_get(rp->ref, "node");
  ai->dir_adapter = tbx_htable_get(dps->adapter_htable,
                                   dir_connection->adapter_name);

  if (!ai->dir_adapter)
    TBX_FAILURE("adapter not found");

  LOG_OUT();

  return ai;
}

//static
tbx_bool_t
connection_open(p_mad_channel_t mad_channel)
{
  p_mad_driver_t                       mad_driver  = NULL;
  p_mad_driver_interface_t             interface   = NULL;
  p_tbx_darray_t                       in_darray   = NULL;
  p_tbx_darray_t                       out_darray  = NULL;
  int                                  command     =   -1;
  ntbx_process_lrank_t                 remote_rank =   -1;
  p_mad_adapter_info_t                 rai         = NULL;
  p_ntbx_process_container_t           pc          = NULL;
  p_mad_dir_connection_t dir_connection         = NULL;

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
  pc = (mad_channel->cloned_dir_channel)?mad_channel->cloned_dir_channel->pc:mad_channel->dir_channel->pc;

  remote_rank = mad_leonie_receive_int();
  TRACE_VAL("Pass 2 - remote_rank", remote_rank);

  if (!interface->accept || !interface->connect)
    goto channel_connection_established;

  rai = get_adapter_info(mad_channel, remote_rank);

  dir_connection = ntbx_pc_get_local_specific(pc, remote_rank);
  if (!dir_connection)
    TBX_FAILURE("unknown connection");

  rai->channel_parameter = dir_connection->channel_parameter;

  if (command)
    {
      // Output connection
      p_mad_connection_t out = NULL;

      TRACE_VAL("Connection to", remote_rank);
      out = tbx_darray_get(out_darray, remote_rank);

      rai->connection_parameter = dir_connection->in_parameter;

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

      rai->connection_parameter = dir_connection->out_parameter;

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
    TBX_FAILURE("channel not found");

  mad_driver =
    tbx_htable_get(madeleine->network_htable, dir_channel->driver->network_name);
  if (!mad_driver)
    TBX_FAILURE("driver not found");

  dir_driver = mad_driver->dir_driver;
  interface  = mad_driver->interface;

  {
    p_mad_dir_connection_t dir_connection = NULL;

    dir_connection = ntbx_pc_get_global_specific(dir_channel->pc, g);
    mad_adapter = tbx_htable_get(mad_driver->adapter_htable, dir_connection->adapter_name);
    if (!mad_adapter)
      TBX_FAILURE("adapter not found");
  }

  mad_channel                = mad_channel_cons();
  mad_channel->process_lrank =
    ntbx_pc_global_to_local(dir_channel->pc, g);
  mad_channel->type          = mad_channel_type_regular;
  mad_channel->id            = (*p_channel_id)++;
  mad_channel->name          = tbx_strdup(dir_channel->name);
  mad_channel->pc            = dir_channel->pc;
  mad_channel->not_private   = dir_channel->not_private;
  mad_channel->mergeable     = dir_channel->mergeable;

  if ((madeleine->settings->leonie_dynamic_mode) &&
      (madeleine->session->session_id > 0) &&
      (mad_channel->mergeable))
     {
	madeleine->dynamic->mergeable = tbx_true;
     }

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
  p_mad_dir_channel_t         dir_fchannel = NULL;
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

  dir_fchannel = tbx_htable_get(madeleine->dir->channel_htable, channel_name);
  if (!dir_fchannel)
    TBX_FAILURE("channel not found");

  dir_channel =
    tbx_htable_get(madeleine->dir->channel_htable, dir_fchannel->cloned_channel_name);

  mad_driver  = tbx_htable_get(madeleine->network_htable,
                               dir_channel->driver->network_name);
  if (!mad_driver)
    TBX_FAILURE("driver not found");

  dir_driver = mad_driver->dir_driver;
  interface  = mad_driver->interface;

  {
    p_mad_dir_connection_t dir_connection = NULL;

    dir_connection = ntbx_pc_get_global_specific(dir_channel->pc, g);
    mad_adapter = tbx_htable_get(mad_driver->adapter_htable, dir_connection->adapter_name);

    if (!mad_adapter)
      TBX_FAILURE("adapter not found");
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
  mad_channel->cloned_dir_channel  = dir_fchannel;
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


static
tbx_bool_t
vchannel_open(p_mad_madeleine_t  madeleine TBX_UNUSED,
              p_mad_channel_id_t p_channel_id TBX_UNUSED)
{
  char  *vchannel_name = NULL;

  vchannel_name = mad_leonie_receive_string();
  if (tbx_streq(vchannel_name, "-"))
    {
      TBX_FREE(vchannel_name);
      return tbx_false;
    }

  // Virtual channel ready
  TBX_FREE(vchannel_name);
  mad_leonie_send_int(-1);

  return tbx_true;
}


static
tbx_bool_t
xchannel_open(p_mad_madeleine_t  madeleine TBX_UNUSED,
              p_mad_channel_id_t p_channel_id TBX_UNUSED)
{
  char                                 *xchannel_name = NULL;

  LOG_IN();
  xchannel_name = mad_leonie_receive_string();
  if (tbx_streq(xchannel_name, "-"))
    {
      TBX_FREE(xchannel_name);
      return tbx_false;
    }

  // Mux channel ready
  mad_leonie_send_int(-1);
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
