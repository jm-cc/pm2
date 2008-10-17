
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

#define _LEO_OUT_CONNECTION 0
#define _LEO_IN_CONNECTION  1

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

  if (command == _LEO_OUT_CONNECTION)
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
  else if (command == _LEO_IN_CONNECTION)
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

#ifdef MARCEL
tbx_bool_t
  connection_disconnect2(p_mad_channel_t ch)
{
   p_mad_driver_t            d      = NULL;
   p_mad_driver_interface_t  i      = NULL;
   int                  command     = -1;
   ntbx_process_lrank_t remote_rank = -1;

   marcel_fprintf(stderr, "================= Getting command ...\n");
   command = mad_leonie_receive_int();
   marcel_fprintf(stderr, "Getting command %i \n", command );

   if (command == -1)
     {
	return tbx_false;
     }

   remote_rank = mad_leonie_receive_int();

   d = ch->adapter->driver;
   i = d->interface;

   if (!i->disconnect)
     goto channel_connection_closed;

   if (command == _LEO_OUT_CONNECTION)
     {
	// Output connection
	      p_mad_connection_t out = NULL;

	TRACE_VAL("Closing connection to", remote_rank);

	out = tbx_darray_get(ch->out_connection_darray, remote_rank);
	if (d->connection_type == mad_bidirectional_connection)
	  {
	     marcel_fprintf(stderr, "Remote rank is %i \n", remote_rank );
	     marcel_fprintf(stderr, "channel is %p \n", ch );
	     marcel_fprintf(stderr, "out is %p \n", out );

	     //      out = tbx_darray_get(ch->out_connection_darray, 1);
	     //      marcel_fprintf(stderr, "out is %p \n", out );

	     if (!out->connected)
	       goto channel_connection_closed;

	     marcel_fprintf(stderr, "Disconnecting out \n");
	     i->disconnect(out);
	     marcel_fprintf(stderr, "Disconnecting out done \n");
	     out->reverse->connected = tbx_false;
	  }
	else
	  {
	     i->disconnect(out);
	  }
     }
   else if (command == _LEO_IN_CONNECTION)
     {
	// Input connection
	p_mad_connection_t in = NULL;

	in = tbx_darray_get(ch->in_connection_darray, remote_rank);
	      if (d->connection_type == mad_bidirectional_connection)
	  {
	     marcel_fprintf(stderr, "Remote rank is %i \n", remote_rank );
	     marcel_fprintf(stderr, "channel is %p \n", ch );
	     marcel_fprintf(stderr, "in is %p \n", in );

	     //     in = tbx_darray_get(ch->in_connection_darray, 0);
	     //      marcel_fprintf(stderr, "in is %p \n", in );
	     if (!in->connected)
	       goto channel_connection_closed;

	     marcel_fprintf(stderr, "Disconnecting in  \n");
	     i->disconnect(in);
	     marcel_fprintf(stderr, "Disconnecting in done \n");
	     in->reverse->connected = tbx_false;
	  }
	else
	  {
	     i->disconnect(in);
	  }
     }

   channel_connection_closed:
   marcel_fprintf(stderr, "Sending int for ack \n");
   mad_leonie_send_int(-1);
   marcel_fprintf(stderr, "Sending int for ack done \n");

   return tbx_true;
}
#endif /* MARCEL */

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

      TBX_FREE(cnx_link->buffer_list);
      TBX_FREE(cnx_link->user_buffer_list);
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
        TBX_FAILURE("incoherent behaviour");

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

      TBX_FREE(in->user_buffer_list_reference);
      TBX_FREE(in->user_buffer_list);
      TBX_FREE(in->buffer_list);
      TBX_FREE(in->buffer_group_list);
      TBX_FREE(in->pair_list);

      TBX_FREE(out->user_buffer_list_reference);
      TBX_FREE(out->user_buffer_list);
      TBX_FREE(out->buffer_list);
      TBX_FREE(out->buffer_group_list);
      TBX_FREE(out->pair_list);

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
        TBX_FAILURE("invalid connection");

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

      TBX_FREE(cnx->user_buffer_list_reference);
      TBX_FREE(cnx->user_buffer_list);
      TBX_FREE(cnx->buffer_list);
      TBX_FREE(cnx->buffer_group_list);
      TBX_FREE(cnx->pair_list);

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
common_channel_exit(p_mad_channel_t mad_channel)
{
  p_mad_adapter_t          mad_adapter = NULL;
  p_mad_driver_t           mad_driver  = NULL;
  p_mad_driver_interface_t interface   = NULL;

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
    {
      interface->channel_exit(mad_channel);
    }
  else if (mad_channel->specific)
    {
      TBX_FREE(mad_channel->specific);
      mad_channel->specific = NULL;
    }

  tbx_htable_extract(mad_adapter->channel_htable, mad_channel->name);

  TBX_FREE(mad_channel->name);
  mad_channel->name = NULL;

  tbx_darray_free(mad_channel->sub_channel_darray);
  mad_channel->sub_channel_darray = NULL;
}

#ifdef MARCEL
void
  common_channel_exit2(p_mad_channel_t mad_channel)
{
   p_mad_adapter_t          mad_adapter = NULL;
   p_mad_driver_t           mad_driver  = NULL;
   p_mad_driver_interface_t interface   = NULL;

   mad_adapter = mad_channel->adapter;
   mad_driver  = mad_adapter->driver;
   interface   = mad_driver->interface;

   if (interface->before_close_channel)
     interface->before_close_channel(mad_channel);

   fprintf(stderr,"PAss 1\n");

   while (connection_disconnect2(mad_channel))
     ;

   fprintf(stderr,"PAss 2\n");
   mad_leonie_send_int(-1);

   fprintf(stderr,"PAss 3\n");
   if (interface->after_close_channel)
     interface->after_close_channel(mad_channel);

   fprintf(stderr,"PAss 4\n");
   while (connection_exit(mad_channel))
     ;

   fprintf(stderr,"PAss 5\n");
   tbx_darray_free(mad_channel->in_connection_darray);
   tbx_darray_free(mad_channel->out_connection_darray);

   mad_channel->in_connection_darray  = NULL;
   mad_channel->out_connection_darray = NULL;

   if (interface->channel_exit)
     {
	interface->channel_exit(mad_channel);
     }
   else if (mad_channel->specific)
     {
	TBX_FREE(mad_channel->specific);
	mad_channel->specific = NULL;
     }

   fprintf(stderr,"PAss 6\n");

   tbx_htable_extract(mad_adapter->channel_htable, mad_channel->name);

   TBX_FREE(mad_channel->name);
     mad_channel->name = NULL;

   tbx_darray_free(mad_channel->sub_channel_darray);
   mad_channel->sub_channel_darray = NULL;
}
#endif /* MARCEL*/

static
void
mad_dir_vchannel_disconnect(p_mad_madeleine_t madeleine)
{
  p_mad_directory_t dir                = NULL;
#if 0
  //#ifdef MARCEL
  p_tbx_slist_t     dir_vchannel_slist = NULL;
#endif // MARCEL
  p_tbx_htable_t    channel_htable     = NULL;

  LOG_IN();
  dir            = madeleine->dir;
  channel_htable = madeleine->channel_htable;

  // "Forward send" threads shutdown
#if 0
//#ifdef MARCEL
  dir_vchannel_slist = dir->vchannel_slist;

  if (!tbx_slist_is_nil(dir_vchannel_slist))
    {
      tbx_slist_ref_to_head(dir_vchannel_slist);
      do
	{
	  p_mad_dir_channel_t dir_vchannel = NULL;
	  p_mad_channel_t     mad_vchannel = NULL;
	  p_tbx_slist_t       slist        = NULL;

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
#if 0
      //#ifdef MARCEL
      p_mad_channel_t       channel       = NULL;
      p_mad_channel_t       vchannel      = NULL;
#endif // MARCEL
      char                 *channel_name  = NULL;
      char                 *vchannel_name = NULL;


      vchannel_name = mad_leonie_receive_string();
      if (tbx_streq(vchannel_name, "-"))
        {
          TBX_FREE(vchannel_name);

          break;
        }

      channel_name = mad_leonie_receive_string();
      lrank = mad_leonie_receive_int();
#if 0
//#ifdef MARCEL
      vchannel = tbx_htable_get(channel_htable, vchannel_name);
      if (!vchannel)
	TBX_FAILURE("channel not found");

      channel = tbx_htable_get(channel_htable, channel_name);
      if (!channel)
	TBX_FAILURE("channel not found");

      mad_forward_stop_reception(vchannel, channel, lrank);
#endif // MARCEL
      mad_leonie_send_int(-1);
      TBX_FREE(channel_name);
      TBX_FREE(vchannel_name);
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
      p_mad_channel_t  mad_channel  = NULL;
#endif // MARCEL
      char            *channel_name = NULL;

      channel_name = mad_leonie_receive_string();
      if (tbx_streq(channel_name, "-"))
        {
          TBX_FREE(channel_name);

          break;
        }

#ifdef MARCEL
      mad_channel = tbx_htable_extract(mad_channel_htable, channel_name);

      if (!mad_channel)
	TBX_FAILURE("vchannel not found");

      TRACE_STR("Vchannel", channel_name);
      common_channel_exit(mad_channel);

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
      TBX_FREE(channel_name);
    }
  LOG_OUT();
}

static
void
mad_dir_xchannel_disconnect(p_mad_madeleine_t madeleine)
{
  p_mad_directory_t dir                = NULL;
#if 0
  //#ifdef MARCEL
  p_tbx_slist_t     dir_xchannel_slist = NULL;
#endif // MARCEL
  p_tbx_htable_t    channel_htable     = NULL;

  LOG_IN();
  dir            = madeleine->dir;
  channel_htable = madeleine->channel_htable;

  // "Forward send" threads shutdown
#if 0
//#ifdef MARCEL
  dir_xchannel_slist = dir->xchannel_slist;

  if (!tbx_slist_is_nil(dir_xchannel_slist))
    {
      tbx_slist_ref_to_head(dir_xchannel_slist);
      do
	{
	  p_mad_dir_channel_t dir_xchannel = NULL;
	  p_mad_channel_t     mad_xchannel = NULL;
	  p_tbx_slist_t       slist        = NULL;

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
#if 0
      //#ifdef MARCEL
      p_mad_channel_t       channel       = NULL;
      p_mad_channel_t       xchannel      = NULL;
#endif // MARCEL
      char                 *channel_name  = NULL;
      char                 *xchannel_name = NULL;


      xchannel_name = mad_leonie_receive_string();
      if (tbx_streq(xchannel_name, "-"))
        {
          TBX_FREE(xchannel_name);

          break;
        }

      channel_name = mad_leonie_receive_string();
      lrank = mad_leonie_receive_int();
#if 0
//#ifdef MARCEL
      xchannel = tbx_htable_get(channel_htable, xchannel_name);
      if (!xchannel)
	TBX_FAILURE("channel not found");

      channel = tbx_htable_get(channel_htable, channel_name);
      if (!channel)
	TBX_FAILURE("channel not found");

      mad_mux_stop_reception(xchannel, channel, lrank);
#endif // MARCEL
      mad_leonie_send_int(-1);
      TBX_FREE(channel_name);
      TBX_FREE(xchannel_name);
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
#endif // MARCEL
      char                     *channel_name = NULL;

      channel_name = mad_leonie_receive_string();
      if (tbx_streq(channel_name, "-"))
        {
          TBX_FREE(channel_name);

          break;
        }

#ifdef MARCEL
      mad_channel = tbx_htable_extract(mad_channel_htable, channel_name);

      if (!mad_channel)
	TBX_FAILURE("xchannel not found");

      TRACE_STR("Xchannel", channel_name);

      common_channel_exit(mad_channel);

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
      TBX_FREE(channel_name);
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
      p_mad_channel_t  mad_channel  = NULL;
      char            *channel_name = NULL;

      channel_name = mad_leonie_receive_string();
      if (tbx_streq(channel_name, "-"))
        {
          TBX_FREE(channel_name);

          break;
        }

      mad_channel = tbx_htable_extract(mad_channel_htable, channel_name);
      if (!mad_channel)
	TBX_FAILURE("channel not found");

      TRACE_STR("Channel", channel_name);

      common_channel_exit(mad_channel);

      memset(mad_channel, 0, sizeof(mad_channel_t));
      TBX_FREE(mad_channel);
      mad_channel = NULL;

      mad_leonie_send_int(-1);
      TBX_FREE(channel_name);
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
  mad_dir_channel_exit(madeleine);

  // Regular channels
  mad_dir_channel_exit(madeleine);
  LOG_OUT();
}

void
mad_dir_driver_exit(p_mad_madeleine_t madeleine)
{
  p_mad_session_t      session             = NULL;
  p_mad_directory_t    dir                 = NULL;
  p_tbx_htable_t       mad_network_htable  = NULL;
  p_tbx_htable_t       mad_device_htable   = NULL;

  LOG_IN();
  session            = madeleine->session;
  dir                = madeleine->dir;
  mad_network_htable = madeleine->network_htable;
  mad_device_htable  = madeleine->device_htable;

  // Adapters
  while (1)
    {
      p_mad_driver_t            mad_driver         = NULL;
      p_mad_driver_interface_t  interface          = NULL;
      p_tbx_htable_t            mad_adapter_htable = NULL;
      char                     *driver_name        = NULL;

      driver_name = mad_leonie_receive_string();
      if (tbx_streq(driver_name, "-"))
        {
          TBX_FREE(driver_name);
          break;
        }

      mad_driver = tbx_htable_get(mad_network_htable, driver_name);
      if (!mad_driver)
	TBX_FAILURE("driver instance not found");

      TRACE_STR("Shutting down adapters of driver", driver_name);
      interface = mad_driver->interface;
      //mad_leonie_send_int(-1);

      mad_adapter_htable = mad_driver->adapter_htable;

      while (1)
	{
	  p_mad_adapter_t  mad_adapter  = NULL;
	  char            *adapter_name = NULL;

	  adapter_name = mad_leonie_receive_string();
	  if (tbx_streq(adapter_name, "-"))
            {
              TBX_FREE(adapter_name);
              break;
            }

	  mad_adapter =
	    tbx_htable_extract(mad_adapter_htable, adapter_name);
	  if (!mad_adapter)
	    TBX_FAILURE("adapter not found");

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

	  mad_leonie_send_int(-1);
          TBX_FREE(adapter_name);
	}
      TBX_FREE(driver_name);
    }
#if 0
//#ifdef MARCEL
  {
    p_mad_driver_t fwd_driver = NULL;

    fwd_driver = tbx_htable_get(mad_network_htable, "forward_network");
    fwd_driver->interface->driver_exit(fwd_driver);
  }

  {
    p_mad_driver_t mux_driver = NULL;

    mux_driver = tbx_htable_get(mad_network_htable, "mux_network");
    mux_driver->interface->driver_exit(mux_driver);
  }
#endif // MARCEL

  // Drivers
  while (1)
    {
      p_mad_driver_t            mad_driver  = NULL;
      p_mad_driver_interface_t  interface   = NULL;
      char                     *driver_name = NULL;

      driver_name = mad_leonie_receive_string();
      if (tbx_streq(driver_name, "-"))
        {
          TBX_FREE(driver_name);
          break;
        }

      mad_driver =
	tbx_htable_get(mad_network_htable, driver_name);
      if (!mad_driver)
	TBX_FAILURE("driver instance not found");

      TRACE_STR("Shutting down driver", driver_name);
      interface = mad_driver->interface;
      if (interface->driver_exit)
	interface->driver_exit(mad_driver);

      if (mad_driver->specific)
	{
	  TBX_FREE(mad_driver->specific);
	  mad_driver->specific = NULL;
	}

      mad_driver->interface = NULL;

      mad_leonie_send_int(-1);
      TBX_FREE(driver_name);
    }


 {
   p_tbx_slist_t mad_network_key_slist = NULL;

   mad_network_key_slist = tbx_htable_get_key_slist(mad_network_htable);

   while (!tbx_slist_is_nil(mad_network_key_slist))
     {
       p_mad_driver_t  mad_driver  = NULL;
       char           *network_name = NULL;

       network_name = tbx_slist_extract(mad_network_key_slist);
       mad_driver = tbx_htable_extract(mad_network_htable, network_name);


       /* may have already be done in mad_forward_driver_exit... */
       if (mad_driver->adapter_htable) {
         tbx_htable_free(mad_driver->adapter_htable);
         mad_driver->adapter_htable = NULL;
       }

       TBX_FREE(mad_driver->device_name);
       mad_driver->device_name = NULL;

       TBX_FREE(mad_driver->network_name);
       mad_driver->network_name = NULL;

       TBX_FREE(mad_driver);
       mad_driver = NULL;

       TBX_FREE(network_name);
     }

   tbx_slist_free(mad_network_key_slist);
 }

 {
   p_tbx_slist_t mad_device_key_slist = NULL;

   mad_device_key_slist = tbx_htable_get_key_slist(mad_device_htable);

   while (!tbx_slist_is_nil(mad_device_key_slist))
     {
       p_mad_driver_interface_t  interface   = NULL;
       char                     *device_name = NULL;

       device_name = tbx_slist_extract(mad_device_key_slist);
       interface   = tbx_htable_extract(mad_device_htable, device_name);

       TBX_FREE(interface);
       TBX_FREE(device_name);
     }

   tbx_slist_free(mad_device_key_slist);
 }

  LOG_OUT();
}

void
mad_leonie_sync(p_mad_madeleine_t madeleine)
{
  p_mad_session_t session   = NULL;
  p_mad_settings_t settings = NULL;
  p_ntbx_client_t client    = NULL;
  int             data      =    0;

  LOG_IN();
  TRACE("Termination sync");
  session  = madeleine->session;
  settings = madeleine->settings;
  client   = session->leonie_link;

  mad_ntbx_send_int(client, mad_leo_command_end);
  if (settings->leonie_dynamic_mode) {
#ifdef MARCEL
    mad_command_thread_exit(madeleine);
#endif // MARCEL
    } else {
    data = mad_ntbx_receive_int(client);
    if (data != 1)
      TBX_FAILURE("synchronization error");

  }
  LOG_OUT();
}

void
mad_leonie_link_exit(p_mad_madeleine_t madeleine)
{
  LOG_IN();
  ntbx_tcp_client_disconnect(madeleine->session->leonie_link);
  LOG_OUT();
}

static
void
session_exit(p_mad_session_t s)
{
  ntbx_client_dest(s->leonie_link);
  s->leonie_link  = NULL;
  s->process_rank =    0;
  TBX_FREE(s);
}

static
void
settings_exit(p_mad_settings_t s)
{
#ifndef LEO_IP
  TBX_FREE(s->leonie_server_host_name);
  s->leonie_server_host_name = NULL;
#endif /* LEO_IP */

  TBX_FREE(s->leonie_server_port);
  s->leonie_server_port = NULL;
  TBX_FREE(s);
}

void
mad_object_exit(p_mad_madeleine_t madeleine TBX_UNUSED)
{
  LOG_IN();
  TBX_FREE(madeleine->dir);
  session_exit(madeleine->session);

  settings_exit(madeleine->settings);
  madeleine->settings = NULL;

  while (!tbx_slist_is_nil(madeleine->public_channel_slist))
    {
      tbx_slist_extract(madeleine->public_channel_slist);
    }

  tbx_slist_free(madeleine->public_channel_slist);
  madeleine->public_channel_slist = NULL;

  tbx_htable_free(madeleine->channel_htable);
  madeleine->channel_htable = NULL;

  tbx_htable_free(madeleine->network_htable);
  madeleine->network_htable = NULL;

  tbx_htable_free(madeleine->device_htable);
  madeleine->device_htable = NULL;

  if (madeleine->dynamic)
    {
      TBX_FREE(madeleine->dynamic);
      madeleine->dynamic = NULL;
    }

#ifdef CONFIG_MULTI_RAIL
  tbx_darray_free(madeleine->cnx_darray);
#endif /* CONFIG_MULTI_RAIL */

  TBX_FREE(madeleine);
  LOG_OUT();
}

void
mad_exit(p_mad_madeleine_t madeleine) 
{
  mad_leonie_sync(madeleine);
  mad_dir_channels_exit(madeleine);
  mad_dir_driver_exit(madeleine);
  mad_directory_exit(madeleine);
  mad_leonie_link_exit(madeleine);
  mad_leonie_command_exit(madeleine);
  mad_object_exit(madeleine);
  madeleine = NULL;
  mad_memory_manager_exit();

  common_exit(NULL);
}


/*
 * Local variables:
 *  c-basic-offset: 2
 *  c-hanging-braces-alist: '((defun-open before after) (class-open before after) (inline-open before after) (block-open before after) (brace-list-open) (brace-entry-open) (substatement-open before after) (block-close . c-snug-do-while) (extern-lang-open before after) (inexpr-class-open before after) (inexpr-class-close before))
 * End:
 */
