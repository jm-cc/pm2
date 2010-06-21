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
 * Mad_channel.c
 * =============
 */
/* #define DEBUG */
/* #define TRACING */

/*
 * Headers
 * -------
 */
#include "madeleine.h"

/*
 * Functions
 * ---------
 */

p_mad_channel_t
mad_get_channel(p_mad_madeleine_t  madeleine,
		char              *name)
{
  p_mad_channel_t channel = NULL;

  LOG_IN();
  channel = tbx_htable_get(madeleine->channel_htable, name);
  if (!channel)
    TBX_FAILUREF("channel <%s> not found", name);

  if (!channel->not_private)
    TBX_FAILUREF("invalid channel <%s>", name);

  LOG_OUT();

  return channel;
}

p_mad_channel_t
mad_get_sub_channel(p_mad_channel_t channel,
		    unsigned int    sub)
{
  p_mad_channel_t sub_channel = NULL;
  p_tbx_darray_t  darray  = NULL;

  LOG_IN();
  darray = channel->sub_channel_darray;

  if (sub >= channel->max_sub)
    TBX_FAILURE("not enough resources to allocate anonymous sub channel");

  sub_channel = tbx_darray_expand_and_get(darray, sub);

  if (!sub_channel)
    {
      p_mad_driver_interface_t interface = NULL;

      interface = channel->adapter->driver->interface;

      if (interface->get_sub_channel)
	{
	  sub_channel = interface->get_sub_channel(channel, sub);
	}
      else
	TBX_FAILURE("anonymous sub channels unsupported by the driver");

      tbx_darray_set(darray, sub, sub_channel);
    }

  LOG_OUT();

  return sub_channel;
}

#ifdef MARCEL
tbx_bool_t
    channel_reopen(p_mad_madeleine_t  madeleine)
{
   ntbx_process_grank_t        g =   -1;
   p_mad_dir_channel_t         dir_channel  = NULL;
   p_mad_channel_t             old_channel  = NULL;
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

   old_channel = tbx_htable_extract(madeleine->channel_htable, channel_name);
     if (!old_channel)
       TBX_FAILURE("Old channel not found");

   mad_driver =
     tbx_htable_get(madeleine->network_htable, dir_channel->driver->network_name);
      if (!mad_driver)
     TBX_FAILURE("driver not found");


   dir_driver = mad_driver->dir_driver;
   interface  = mad_driver->interface;
     {
	p_mad_dir_connection_t dir_connection = NULL;

	dir_connection = ntbx_pc_get_global_specific(dir_channel->pc, g);
	mad_adapter = tbx_htable_get(mad_driver->adapter_htable,
				     dir_connection->adapter_name);
	if (!mad_adapter)
	  TBX_FAILURE("adapter not found");
     }

   mad_channel                = mad_channel_cons();
   mad_channel->process_lrank = ntbx_pc_global_to_local(dir_channel->pc, g);
   mad_channel->type          = mad_channel_type_regular;
   mad_channel->id            = old_channel->id;
   mad_channel->name          = tbx_strdup(dir_channel->name);
   mad_channel->pc            = dir_channel->pc;
   mad_channel->not_private   = dir_channel->not_private;
   mad_channel->mergeable     = dir_channel->mergeable;
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

   tbx_htable_extract(mad_adapter->channel_htable, channel_name);
   tbx_htable_add(mad_adapter->channel_htable, dir_channel->name, mad_channel);

   tbx_htable_add(madeleine->channel_htable,   dir_channel->name, mad_channel);

   TRACE("Pass 3 - sending -1 sync");
   mad_leonie_send_int(-1);
   TBX_FREE(channel_name);
   LOG_OUT();

   return tbx_true;
}

void
  mad_channel_merge_done(p_mad_madeleine_t madeleine)
{
   LOG_IN();
   madeleine->dynamic->merge_done = tbx_true ;
   LOG_OUT();
}

tbx_bool_t
  mad_channel_is_merged(p_mad_madeleine_t madeleine)
{
   tbx_bool_t res = tbx_false;

   LOG_IN();
   if ( madeleine->dynamic->merge_done == tbx_true )
     {
	madeleine->dynamic->merge_done =  tbx_false;
	res                            =  tbx_true;
     }

   LOG_OUT();
   return res;
}

tbx_bool_t
    mad_expand_channel(p_mad_madeleine_t madeleine,
		       char *name)
{
   tbx_bool_t res  = tbx_false;

   if (madeleine->dynamic->mergeable == tbx_true)
     {
	do
	  {
	     mad_directory_update( madeleine );
	  }
	while(! mad_directory_is_updated( madeleine));

	mad_leonie_send_int(mad_leo_command_merge_channel);
	mad_leonie_send_string(name);

	while(! mad_channel_is_merged( madeleine ))
	  marcel_yield();

	madeleine->dynamic->mergeable = tbx_true;
	res = tbx_true ;
     }
   return res;
}

void
  mad_channel_split_done(p_mad_madeleine_t madeleine)
{
   LOG_IN();
   madeleine->dynamic->split_done = tbx_true ;
   LOG_OUT();
}

tbx_bool_t
  mad_channel_is_split(p_mad_madeleine_t madeleine)
{
   tbx_bool_t res = tbx_false;

   LOG_IN();
   if ( madeleine->dynamic->split_done == tbx_true )
     {
	madeleine->dynamic->split_done =  tbx_false;
	res                            =  tbx_true;
     }

   LOG_OUT();
   return res;
}

tbx_bool_t
  mad_shrink_channel(p_mad_madeleine_t madeleine,
		     char *name)
{
   tbx_bool_t          res          = tbx_false;

   if (madeleine->dynamic->mergeable == tbx_true)
     {
	mad_directory_rollback( madeleine );

	mad_leonie_send_int(mad_leo_command_split_channel);
	mad_leonie_send_string(name);

	while(! mad_channel_is_split( madeleine ))
	  marcel_yield();

	madeleine->dynamic->mergeable = tbx_true;
	res = tbx_true ;
     }
   return res;
}
#endif /* MARCEL */
