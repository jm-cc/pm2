
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

static
tbx_bool_t
connection_disconnect(p_mad_channel_t ch)
{
  p_mad_driver_t            d   = NULL;
  p_mad_driver_interface_t  i    = NULL;
  int                  command     = -1;
  ntbx_process_lrank_t remote_rank = -1;

  command = mad_leonie_receive_int();
  if (command == -1)
    {
      return tbx_false;
    }

  remote_rank = mad_leonie_receive_int();

  d = ch->adapter->driver;
  i = d->interface;

  if (!i->disconnect)
    goto channel_connection_closed;

  if (command)
    {
      // Output connection
      p_mad_connection_t out = NULL;

      TRACE_VAL("Closing connection to", remote_rank);

      out = tbx_darray_get(ch->out_connection_darray, remote_rank);
      if (d->connection_type == mad_bidirectional_connection)
        {
          if (!out->connected)
            goto channel_connection_closed;

          i->disconnect(out);
          out->reverse->connected = tbx_false;
        }
      else
        {
          i->disconnect(out);
        }
    }
  else
    {
      // Input connection
      p_mad_connection_t in = NULL;

      TRACE_VAL("Closing connection from", remote_rank);

      in = tbx_darray_get(ch->in_connection_darray, remote_rank);
      if (d->connection_type == mad_bidirectional_connection)
        {
          if (!in->connected)
            goto channel_connection_closed;

          i->disconnect(in);
          in->reverse->connected = tbx_false;
        }
      else
        {
          i->disconnect(in);
        }

    }

 channel_connection_closed:
  mad_leonie_send_int(-1);

  return tbx_true;
}

static
void
links_exit(p_mad_connection_t cnx)
{
  mad_link_id_t            link_id =   -1;
  p_mad_driver_interface_t i       = NULL;

  i = cnx->channel->adapter->driver->interface;

  LOG_IN();
  for (link_id = 0; link_id < cnx->nb_link; link_id++)
    {
      p_mad_link_t cnx_link  = NULL;

      cnx_link  = cnx->link_array[link_id];

      if (i->link_exit)
        {
          i->link_exit(cnx_link);
          cnx_link->specific = NULL;
        }
      else
        {
          if (cnx_link->specific)
            {
              TBX_FREE(cnx_link->specific);
              cnx_link->specific = NULL;
            }
        }

      memset(cnx_link, 0, sizeof(mad_link_t));

      TBX_FREE(cnx_link);
      cnx->link_array[link_id] = NULL;
    }

  TBX_FREE(cnx->link_array);
  cnx->link_array = NULL;
  LOG_OUT();
}

static
tbx_bool_t
connection_exit(p_mad_channel_t ch)
{
  p_mad_driver_t           d           = NULL;
  p_mad_driver_interface_t i           = NULL;
  p_tbx_darray_t           in_darray   = NULL;
  p_tbx_darray_t           out_darray  = NULL;
  int                      command     =   -1;
  ntbx_process_lrank_t     remote_rank =   -1;

  LOG_IN();
  command = mad_leonie_receive_int();
  if (command == -1)
    return tbx_false;

  in_darray  = ch->in_connection_darray;
  out_darray = ch->out_connection_darray;

  d = ch->adapter->driver;
  i = d->interface;

  remote_rank = mad_leonie_receive_int();

  TRACE_VAL("Freeing connection with", remote_rank);

  if (d->connection_type == mad_bidirectional_connection)
    {
      p_mad_connection_t in  = NULL;
      p_mad_connection_t out = NULL;

      in  = tbx_darray_get(in_darray, remote_rank);
      out = tbx_darray_get(out_darray, remote_rank);

      if (!in && !out)
        goto connection_freed;

      if (!in || !out)
        FAILURE("incoherent behaviour");

      links_exit(in);
      links_exit(out);

      if (i->connection_exit)
        {
          i->connection_exit(in, out);
        }
      else
        {
          if (in->specific)
            {
              TBX_FREE(in->specific);

              if (out->specific == in->specific)
                {
                  out->specific = NULL;
                }

              in->specific  = NULL;
            }

          if (out->specific)
            {
              TBX_FREE(out->specific);
              out->specific = NULL;
            }
        }

      memset(in,  0, sizeof(mad_connection_t));
      memset(out, 0, sizeof(mad_connection_t));

      TBX_FREE(in);
      TBX_FREE(out);

      tbx_darray_set(in_darray,  remote_rank, NULL);
      tbx_darray_set(out_darray, remote_rank, NULL);
    }
  else
    {
      p_tbx_darray_t     cnx_darray = NULL;
      p_mad_connection_t cnx        = NULL;

      cnx_darray = command?out_darray:in_darray;
      cnx = tbx_darray_get(cnx_darray, remote_rank);
      if (!cnx)
        FAILURE("invalid connection");

      links_exit(cnx);

      if (i->connection_exit)
        {
          if (command)
            {
              i->connection_exit(NULL, cnx);
            }
          else
            {
              i->connection_exit(cnx, NULL);
            }
        }
      else
        {
          if (cnx->specific)
            {
              TBX_FREE(cnx->specific);
              cnx->specific = NULL;
            }
        }

      memset(cnx, 0, sizeof(mad_connection_t));
      TBX_FREE(cnx);
      tbx_darray_set(cnx_darray, remote_rank, NULL);
    }

 connection_freed:
  mad_leonie_send_int(-1);

  LOG_OUT();
  return tbx_true;
}

static
void
mad_dir_vchannel_disconnect(p_mad_madeleine_t madeleine)
{
  p_mad_directory_t dir                = NULL;
#ifdef MARCEL
  p_tbx_slist_t     dir_vchannel_slist = NULL;
#endif // MARCEL
  p_tbx_htable_t    channel_htable     = NULL;

  LOG_IN();
  dir            = madeleine->dir;
  channel_htable = madeleine->channel_htable;

  // "Forward send" threads shutdown
#ifdef MARCEL
  dir_vchannel_slist = dir->vchannel_slist;

  if (!tbx_slist_is_nil(dir_vchannel_slist))
    {
      tbx_slist_ref_to_head(dir_vchannel_slist);
      do
	{
	  p_mad_dir_vchannel_t dir_vchannel = NULL;
	  p_mad_channel_t      mad_vchannel = NULL;
	  p_tbx_slist_t        slist        = NULL;

	  dir_vchannel = tbx_slist_ref_get(dir_vchannel_slist);
	  mad_vchannel = tbx_htable_get(channel_htable, dir_vchannel->name);

	  // Regular channels
	  slist = mad_vchannel->channel_slist;

	  tbx_slist_ref_to_head(slist);
	  do
	    {
	      p_mad_channel_t channel = NULL;

	      channel = tbx_slist_ref_get(slist);
	      mad_forward_stop_direct_retransmit(channel);
	    }
	  while (tbx_slist_ref_forward(slist));

	  // Forwarding channels
	  slist = mad_vchannel->fchannel_slist;

	  tbx_slist_ref_to_head(slist);
	  do
	    {
	      p_mad_channel_t channel = NULL;

	      channel = tbx_slist_ref_get(slist);
	      mad_forward_stop_indirect_retransmit(channel);
	    }
	  while (tbx_slist_ref_forward(slist));

	}
      while (tbx_slist_ref_forward(dir_vchannel_slist));
    }
#endif // MARCEL

  mad_leonie_send_int(-1);

  // "Forward receive" threads shutdown
  for (;;)
    {
      ntbx_process_lrank_t  lrank         = -1;
#ifdef MARCEL
      p_mad_channel_t       channel       = NULL;
      p_mad_channel_t       vchannel      = NULL;
#endif // MARCEL
      char                 *channel_name  = NULL;
      char                 *vchannel_name = NULL;


      vchannel_name = mad_leonie_receive_string();
      if (tbx_streq(vchannel_name, "-"))
	break;

      channel_name = mad_leonie_receive_string();
      lrank = mad_leonie_receive_int();

#ifdef MARCEL
      vchannel = tbx_htable_get(channel_htable, vchannel_name);
      if (!vchannel)
	FAILURE("channel not found");

      channel = tbx_htable_get(channel_htable, channel_name);
      if (!channel)
	FAILURE("channel not found");

      mad_forward_stop_reception(vchannel, channel, lrank);
#endif // MARCEL
      mad_leonie_send_int(-1);
    }

  // Vchannel closing

  LOG_OUT();
}

static
void
mad_dir_vchannel_exit(p_mad_madeleine_t madeleine)
{
  p_mad_directory_t dir                = NULL;
  p_tbx_htable_t    mad_channel_htable = NULL;

  LOG_IN();

  TRACE("Closing vchannels");
  dir                = madeleine->dir;
  mad_channel_htable = madeleine->channel_htable;

  while (1)
    {
#ifdef MARCEL
      p_mad_channel_t           mad_channel  = NULL;
      p_mad_adapter_t           mad_adapter  = NULL;
      p_mad_driver_t            mad_driver   = NULL;
      p_mad_driver_interface_t  interface    = NULL;
#endif // MARCEL
      char                     *channel_name = NULL;

      channel_name = mad_leonie_receive_string();
      if (tbx_streq(channel_name, "-"))
	break;

#ifdef MARCEL
      mad_channel = tbx_htable_extract(mad_channel_htable, channel_name);

      if (!mad_channel)
	FAILURE("vchannel not found");

      TRACE_STR("Vchannel", channel_name);

      mad_adapter = mad_channel->adapter;
      mad_driver  = mad_adapter->driver;
      interface   = mad_driver->interface;

      if (interface->before_close_channel)
	interface->before_close_channel(mad_channel);

      while (connection_disconnect(mad_channel))
	;

      mad_leonie_send_int(-1);

      if (interface->after_close_channel)
	interface->after_close_channel(mad_channel);

      while (connection_exit(mad_channel))
	;

      tbx_darray_free(mad_channel->in_connection_darray);
      tbx_darray_free(mad_channel->out_connection_darray);

      mad_channel->in_connection_darray  = NULL;
      mad_channel->out_connection_darray = NULL;

      if (interface->channel_exit)
	interface->channel_exit(mad_channel);

      tbx_htable_extract(mad_adapter->channel_htable, mad_channel->name);

      TBX_FREE(mad_channel->name);
      mad_channel->name = NULL;

      {
	p_tbx_slist_t slist = NULL;

	slist = mad_channel->channel_slist;
	while (!tbx_slist_is_nil(slist))
	  {
	    tbx_slist_extract(slist);
	  }

	tbx_slist_free(slist);
	mad_channel->channel_slist = NULL;

	slist = mad_channel->fchannel_slist;
	while (!tbx_slist_is_nil(slist))
	  {
	    tbx_slist_extract(slist);
	  }

	tbx_slist_free(slist);
	mad_channel->fchannel_slist = NULL;
      }

      memset(mad_channel, 0, sizeof(mad_channel_t));
      TBX_FREE(mad_channel);
      mad_channel = NULL;
#endif // MARCEL

      mad_leonie_send_int(-1);
    }
  LOG_OUT();
}

static
void
mad_dir_xchannel_disconnect(p_mad_madeleine_t madeleine)
{
  p_mad_directory_t dir                = NULL;
#ifdef MARCEL
  p_tbx_slist_t     dir_xchannel_slist = NULL;
#endif // MARCEL
  p_tbx_htable_t    channel_htable     = NULL;

  LOG_IN();
  dir            = madeleine->dir;
  channel_htable = madeleine->channel_htable;

  // "Forward send" threads shutdown
#ifdef MARCEL
  dir_xchannel_slist = dir->xchannel_slist;

  if (!tbx_slist_is_nil(dir_xchannel_slist))
    {
      tbx_slist_ref_to_head(dir_xchannel_slist);
      do
	{
	  p_mad_dir_xchannel_t dir_xchannel = NULL;
	  p_mad_channel_t      mad_xchannel = NULL;
	  p_tbx_slist_t        slist        = NULL;

	  dir_xchannel = tbx_slist_ref_get(dir_xchannel_slist);
	  mad_xchannel = tbx_htable_get(channel_htable, dir_xchannel->name);

	  // Regular channels
	  slist = mad_xchannel->channel_slist;

	  tbx_slist_ref_to_head(slist);
	  do
	    {
	      p_mad_channel_t channel = NULL;

	      channel = tbx_slist_ref_get(slist);
	      mad_mux_stop_indirect_retransmit(channel);
	    }
	  while (tbx_slist_ref_forward(slist));

	}
      while (tbx_slist_ref_forward(dir_xchannel_slist));
    }
#endif // MARCEL

  mad_leonie_send_int(-1);

  // "Forward receive" threads shutdown
  for (;;)
    {
      ntbx_process_lrank_t  lrank         = -1;
#ifdef MARCEL
      p_mad_channel_t       channel       = NULL;
      p_mad_channel_t       xchannel      = NULL;
#endif // MARCEL
      char                 *channel_name  = NULL;
      char                 *xchannel_name = NULL;


      xchannel_name = mad_leonie_receive_string();
      if (tbx_streq(xchannel_name, "-"))
	break;

      channel_name = mad_leonie_receive_string();
      lrank = mad_leonie_receive_int();

#ifdef MARCEL
      xchannel = tbx_htable_get(channel_htable, xchannel_name);
      if (!xchannel)
	FAILURE("channel not found");

      channel = tbx_htable_get(channel_htable, channel_name);
      if (!channel)
	FAILURE("channel not found");

      mad_mux_stop_reception(xchannel, channel, lrank);
#endif // MARCEL
      mad_leonie_send_int(-1);
    }

  // Xchannel closing

  LOG_OUT();
}

static
void
mad_dir_xchannel_exit(p_mad_madeleine_t madeleine)
{
  p_mad_directory_t dir                = NULL;
  p_tbx_htable_t    mad_channel_htable = NULL;

  LOG_IN();

  TRACE("Closing xchannels");
  dir                = madeleine->dir;
  mad_channel_htable = madeleine->channel_htable;

  while (1)
    {
#ifdef MARCEL
      p_mad_channel_t           mad_channel  = NULL;
      p_mad_adapter_t           mad_adapter  = NULL;
      p_mad_driver_t            mad_driver   = NULL;
      p_mad_driver_interface_t  interface    = NULL;
#endif // MARCEL
      char                     *channel_name = NULL;

      channel_name = mad_leonie_receive_string();
      if (tbx_streq(channel_name, "-"))
	break;

#ifdef MARCEL
      mad_channel = tbx_htable_extract(mad_channel_htable, channel_name);

      if (!mad_channel)
	FAILURE("xchannel not found");

      TRACE_STR("Xchannel", channel_name);

      mad_adapter = mad_channel->adapter;
      mad_driver  = mad_adapter->driver;
      interface   = mad_driver->interface;

      if (interface->before_close_channel)
	interface->before_close_channel(mad_channel);

      while (connection_disconnect(mad_channel))
	;

      mad_leonie_send_int(-1);

      if (interface->after_close_channel)
	interface->after_close_channel(mad_channel);

      while (connection_exit(mad_channel))
	;

      tbx_darray_free(mad_channel->in_connection_darray);
      tbx_darray_free(mad_channel->out_connection_darray);

      mad_channel->in_connection_darray  = NULL;
      mad_channel->out_connection_darray = NULL;

      if (interface->channel_exit)
	interface->channel_exit(mad_channel);

      tbx_htable_extract(mad_adapter->channel_htable, mad_channel->name);

      TBX_FREE(mad_channel->name);
      mad_channel->name = NULL;

      {
	p_tbx_slist_t slist = NULL;

	slist = mad_channel->channel_slist;
	while (!tbx_slist_is_nil(slist))
	  {
	    tbx_slist_extract(slist);
	  }

	tbx_slist_free(slist);
	mad_channel->channel_slist = NULL;
      }

      memset(mad_channel, 0, sizeof(mad_channel_t));
      TBX_FREE(mad_channel);
      mad_channel = NULL;
#endif // MARCEL

      mad_leonie_send_int(-1);
    }
  LOG_OUT();
}

static
void
mad_dir_fchannel_exit(p_mad_madeleine_t madeleine)
{
  p_mad_directory_t dir                = NULL;
  p_tbx_htable_t    mad_channel_htable = NULL;

  LOG_IN();

  TRACE("Closing fchannels");
  dir                = madeleine->dir;
  mad_channel_htable = madeleine->channel_htable;

  while (1)
    {
      p_mad_channel_t           mad_channel  = NULL;
      p_mad_adapter_t           mad_adapter  = NULL;
      p_mad_driver_t            mad_driver   = NULL;
      p_mad_driver_interface_t  interface    = NULL;
      char                     *channel_name = NULL;

      channel_name = mad_leonie_receive_string();
      if (tbx_streq(channel_name, "-"))
	break;

      mad_channel = tbx_htable_extract(mad_channel_htable, channel_name);

      if (!mad_channel)
	FAILURE("fchannel not found");

      TRACE_STR("Fchannel", channel_name);

      mad_adapter = mad_channel->adapter;
      mad_driver  = mad_adapter->driver;
      interface   = mad_driver->interface;

      if (interface->before_close_channel)
	interface->before_close_channel(mad_channel);

      while (connection_disconnect(mad_channel))
	;

      mad_leonie_send_int(-1);

      if (interface->after_close_channel)
	interface->after_close_channel(mad_channel);

      while (connection_exit(mad_channel))
	;

      tbx_darray_free(mad_channel->in_connection_darray);
      tbx_darray_free(mad_channel->out_connection_darray);

      mad_channel->in_connection_darray  = NULL;
      mad_channel->out_connection_darray = NULL;

      if (interface->channel_exit)
	interface->channel_exit(mad_channel);

      tbx_htable_extract(mad_adapter->channel_htable, mad_channel->name);

      TBX_FREE(mad_channel->name);
      mad_channel->name = NULL;

      memset(mad_channel, 0, sizeof(mad_channel_t));
      TBX_FREE(mad_channel);
      mad_channel = NULL;

      mad_leonie_send_int(-1);
    }
  LOG_OUT();
}

static
void
mad_dir_channel_exit(p_mad_madeleine_t madeleine)
{
  p_mad_directory_t dir                = NULL;
  p_tbx_htable_t    mad_channel_htable = NULL;

  LOG_IN();

  TRACE("Closing channels");
  dir                = madeleine->dir;
  mad_channel_htable = madeleine->channel_htable;

  while (1)
    {
      p_mad_channel_t           mad_channel  = NULL;
      p_mad_adapter_t           mad_adapter  = NULL;
      p_mad_driver_t            mad_driver   = NULL;
      p_mad_driver_interface_t  interface    = NULL;
      p_tbx_darray_t            in_darray    = NULL;
      p_tbx_darray_t            out_darray   = NULL;
      char                     *channel_name = NULL;

      channel_name = mad_leonie_receive_string();
      TRACE_STR("Pass 1 - Channel", channel_name);
      if (tbx_streq(channel_name, "-"))
	break;


      mad_channel = tbx_htable_extract(mad_channel_htable, channel_name);

      if (!mad_channel)
	FAILURE("channel not found");

      mad_adapter = mad_channel->adapter;
      mad_driver  = mad_adapter->driver;
      interface   = mad_driver->interface;

      if (interface->before_close_channel)
	interface->before_close_channel(mad_channel);

      while (connection_disconnect(mad_channel))
	;

      TRACE("Pass 2 - sending -1 ack");
      mad_leonie_send_int(-1);

      if (interface->after_close_channel)
	interface->after_close_channel(mad_channel);

      while (connection_exit(mad_channel))
	;

      tbx_darray_free(mad_channel->in_connection_darray);
      tbx_darray_free(mad_channel->out_connection_darray);

      mad_channel->in_connection_darray  = NULL;
      mad_channel->out_connection_darray = NULL;

      if (interface->channel_exit)
	interface->channel_exit(mad_channel);

      tbx_htable_extract(mad_adapter->channel_htable, mad_channel->name);

      TBX_FREE(mad_channel->name);
      mad_channel->name = NULL;

      memset(mad_channel, 0, sizeof(mad_channel_t));
      TBX_FREE(mad_channel);
      mad_channel = NULL;

      TRACE("Pass 3 - sending -1 ack");
      mad_leonie_send_int(-1);
    }
  LOG_OUT();
}

void
mad_dir_channels_exit(p_mad_madeleine_t madeleine)
{
  LOG_IN();
  // Meta-channels disconnection
  mad_dir_vchannel_disconnect(madeleine);
  mad_dir_xchannel_disconnect(madeleine);

  // Virtual channels
  mad_dir_vchannel_exit(madeleine);

  // Mux channels
  mad_dir_xchannel_exit(madeleine);

  // Forwarding channels
  mad_dir_fchannel_exit(madeleine);

  // Regular channels
  mad_dir_channel_exit(madeleine);
  LOG_OUT();
}

void
mad_dir_driver_exit(p_mad_madeleine_t madeleine)
{
  p_mad_session_t      session             = NULL;
  p_mad_directory_t    dir                 = NULL;
  p_tbx_htable_t       mad_driver_htable   = NULL;

  LOG_IN();
  session           = madeleine->session;
  dir               = madeleine->dir;
  mad_driver_htable = madeleine->driver_htable;

  // Adapters
  while (1)
    {
      p_mad_driver_t            mad_driver         = NULL;
      p_mad_driver_interface_t  interface          = NULL;
      p_tbx_htable_t            mad_adapter_htable = NULL;
      char                     *driver_name        = NULL;

      driver_name = mad_leonie_receive_string();
      if (tbx_streq(driver_name, "-"))
	break;

      mad_driver = tbx_htable_get(mad_driver_htable, driver_name);
      if (!mad_driver)
	FAILURE("driver not available");

      TRACE_STR("Shutting down adapters of driver", driver_name);
      interface = mad_driver->interface;
      mad_leonie_send_int(1);

      mad_adapter_htable = mad_driver->adapter_htable;

      while (1)
	{
	  p_mad_adapter_t  mad_adapter  = NULL;
	  char            *adapter_name = NULL;

	  adapter_name = mad_leonie_receive_string();
	  if (tbx_streq(adapter_name, "-"))
	    break;

	  mad_adapter =
	    tbx_htable_extract(mad_adapter_htable, adapter_name);
	  if (!mad_adapter)
	    FAILURE("adapter not found");

	  TRACE_STR("Shutting down adapter", adapter_name);

	  if (interface->adapter_exit)
	    interface->adapter_exit(mad_adapter);

	  if (mad_adapter->selector)
	    {
	      TBX_FREE(mad_adapter->selector);
	      mad_adapter->selector = NULL;
	    }

	  if (mad_adapter->parameter)
	    {
	      TBX_FREE(mad_adapter->parameter);
	      mad_adapter->parameter = NULL;
	    }

	  if (mad_adapter->specific)
	    {
	      TBX_FREE(mad_adapter->specific);
	      mad_adapter->specific = NULL;
	    }

	  tbx_htable_free(mad_adapter->channel_htable);
	  mad_adapter->channel_htable = NULL;

	  TBX_FREE(mad_adapter);
	  mad_adapter = NULL;

	  mad_leonie_send_int(1);
	}

      tbx_htable_free(mad_adapter_htable);
      mad_adapter_htable         = NULL;
      mad_driver->adapter_htable = NULL;
    }

  // Drivers
  while (1)
    {
      p_mad_driver_t            mad_driver  = NULL;
      p_mad_driver_interface_t  interface   = NULL;
      char                     *driver_name = NULL;

      driver_name = mad_leonie_receive_string();
      if (tbx_streq(driver_name, "-"))
	break;

      mad_driver =
	tbx_htable_extract(mad_driver_htable, driver_name);
      if (!mad_driver)
	FAILURE("driver not available");

      TRACE_STR("Shutting down driver", driver_name);
      interface = mad_driver->interface;
      if (interface->driver_exit)
	interface->driver_exit(mad_driver);

      if (mad_driver->specific)
	{
	  TBX_FREE(mad_driver->specific);
	  mad_driver->specific = NULL;
	}

      TBX_FREE(interface);
      interface             = NULL;
      mad_driver->interface = NULL;

      TBX_FREE(mad_driver->name);
      mad_driver->name = NULL;

      TBX_FREE(mad_driver);
      mad_driver = NULL;

      mad_leonie_send_int(1);
    }

  LOG_OUT();
}

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

/*
 * Local variables:
 *  c-basic-offset: 2
 *  c-hanging-braces-alist: '((defun-open before after) (class-open before after) (inline-open before after) (block-open before after) (brace-list-open) (brace-entry-open) (substatement-open before after) (block-close . c-snug-do-while) (extern-lang-open before after) (inexpr-class-open before after) (inexpr-class-close before))
 * End:
 */
