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
 * Mad_mux.c
 * =============
 */

#include "madeleine.h"
#include "tbx.h"

#ifdef MARCEL
/*
 * Macros
 * ------
 */
#define INITIAL_XBHEADER_NUM 64
#define INITIAL_MQENTRY_NUM  256

/*
 * Static variables
 * ----------------
 */
static p_tbx_memory_t mad_xbheader_memory = NULL;
static p_tbx_memory_t mad_mqentry_memory  = NULL;

/*
 * Functions
 * ---------
 */

void
mad_mux_memory_manager_init(int    argc TBX_UNUSED,
				char **argv TBX_UNUSED)
{
  LOG_IN();
  tbx_malloc_init(&mad_xbheader_memory,
		  sizeof(mad_xblock_header_t),
		  INITIAL_XBHEADER_NUM,
                  "madeleine/xblock_headers");
  tbx_malloc_init(&mad_mqentry_memory,
		  sizeof(mad_xmessage_queue_entry_t),
		  INITIAL_MQENTRY_NUM,
                  "madeleine/xmessage_queue_entries");
  LOG_OUT();
}

void
mad_mux_memory_manager_exit(void)
{
  LOG_IN();
  tbx_malloc_clean(mad_xbheader_memory);
  tbx_malloc_clean(mad_mqentry_memory);
  LOG_OUT();
}

static
void
mad_mux_write_block_header(p_mad_connection_t    out,
			   p_mad_xblock_header_t xbh)
{
  p_mad_buffer_t           xbh_buffer = NULL;
  p_mad_driver_interface_t interface  = NULL;
  p_mad_link_t             lnk        = NULL;
  p_mad_link_t             data_lnk   = NULL;

  LOG_IN();
  interface  = out->channel->adapter->driver->interface;
  xbh_buffer = mad_get_user_send_buffer(xbh->data,
					mad_xblock_fsize);
  if (interface->choice)
    {
      lnk = interface->choice(out, xbh_buffer->length,
			      mad_send_SAFER,
			      mad_receive_EXPRESS);
    }
  else
    {
      lnk = out->link_array[0];
    }

  if (lnk->buffer_mode == mad_buffer_mode_static)
    {
      p_mad_buffer_t static_buffer = NULL;

      static_buffer = interface->get_static_buffer(lnk);
      mad_copy_buffer(xbh_buffer, static_buffer);
      interface->send_buffer(lnk, static_buffer);
    }
  else if (lnk->buffer_mode == mad_buffer_mode_dynamic)
    {
      interface->send_buffer(lnk, xbh_buffer);
    }
  else
    TBX_FAILURE("invalid link mode");

  if (!xbh->data_available)
    goto end;

  if (interface->choice)
    {
      data_lnk = interface->choice(out, xbh->length,
				   mad_send_SAFER,
				   mad_receive_EXPRESS);
    }
  else
    {
      data_lnk = out->link_array[0];
    }

  if (data_lnk->buffer_mode == mad_buffer_mode_static)
    {
      if (xbh->buffer_type == mad_xblock_buffer_type_static_src)
	{
	  p_mad_buffer_t static_buffer = NULL;

	  static_buffer = interface->get_static_buffer(data_lnk);
	  mad_copy_buffer(xbh->block, static_buffer);
	  interface->send_buffer(data_lnk, static_buffer);

	  xbh->src_interface->
	    return_static_buffer(xbh->src_link, xbh->block);
	}
      else if (xbh->buffer_type == mad_xblock_buffer_type_static_dst)
	{
	  interface->send_buffer(data_lnk, xbh->block);
	}
      else
	TBX_FAILURE("invalid block type");
    }
  else if (data_lnk->buffer_mode == mad_buffer_mode_dynamic)
    {
      interface->send_buffer(data_lnk, xbh->block);

      if (xbh->buffer_type == mad_xblock_buffer_type_static_src)
	{
	  xbh->src_interface->
	    return_static_buffer(xbh->src_link, xbh->block);
	}
      else if (xbh->buffer_type == mad_xblock_buffer_type_dynamic)
	{
	  mad_free_buffer(xbh->block);
	}
      else
	TBX_FAILURE("invalid block type");
    }
  else
    TBX_FAILURE("invalid link mode");

 end:
  mad_free_buffer_struct(xbh_buffer);
  tbx_free(mad_xbheader_memory, xbh);
  LOG_OUT();
}

static
void *
mad_mux_reemit_block(void *arg)
{
  p_mad_mux_reemit_block_arg_t  rarg                 = NULL;
  p_mad_connection_t            out                  = NULL;
  p_mad_channel_t               xchannel             = NULL;
  p_mad_driver_interface_t      interface            = NULL;
  marcel_sem_t                 *something_to_forward = NULL;
  p_tbx_slist_t                 block_slist          = NULL;

  LOG_IN();
  rarg                 = arg;
  out                  = rarg->connection;
  xchannel             = rarg->xchannel;
  TBX_FREE(arg);
  arg                  = NULL;
  rarg                 = NULL;
  interface            = out->channel->adapter->driver->interface;
  something_to_forward = &(out->something_to_forward);
  block_slist          = out->forwarding_block_list;

  for (;;)
    {
      p_mad_xblock_header_t xbh = NULL;

      TRACE("mad_mux_reemit_block: waiting for something to forward");
      marcel_sem_P(something_to_forward);
      TRACE("mad_mux_reemit_block: got something to forward");

      if (out->closing)
	break;

      TRACE("mad_mux_reemit_block: locking bock_slist");
      TBX_LOCK_SHARED(block_slist);
      xbh = tbx_slist_extract(block_slist);
      TBX_UNLOCK_SHARED(block_slist);
      TRACE("mad_mux_reemit_block: unlocking bock_slist");

      TRACE("mad_mux_reemit_block: locking out->lock_mutex");
      marcel_mutex_lock(&(out->lock_mutex));
      if (interface->new_message)
	interface->new_message(out);

      if (xbh->is_a_group)
	{
	  unsigned int group_len = 0;

	  group_len = xbh->length;
	  mad_mux_write_block_header(out, xbh);
	  xbh = NULL;

	  while (group_len--)
	    {
	      TRACE("mad_mux_reemit_block: waiting again for something to forward");
	      marcel_sem_P(something_to_forward);
	      TRACE("mad_mux_reemit_block: got something to forward again");

	      TRACE("mad_mux_reemit_block: locking bock_slist again");
	      TBX_LOCK_SHARED(block_slist);
	      xbh = tbx_slist_extract(block_slist);
	      TBX_UNLOCK_SHARED(block_slist);
	      TRACE("mad_mux_reemit_block: unlocking bock_slist again");

	      mad_mux_write_block_header(out, xbh);
	      xbh = NULL;
	    }
	}
      else
	{
	  mad_mux_write_block_header(out, xbh);
	}

      if (interface->finalize_message)
	interface->finalize_message(out);

      TRACE("mad_mux_reemit_block: unlocking out->lock_mutex");
      marcel_mutex_unlock(&(out->lock_mutex));
    }
  LOG_OUT();

  return NULL;
}

static
p_mad_xblock_header_t
mad_mux_read_block_header(p_mad_channel_t    mad_xchannel,
			  p_mad_connection_t in)
{
  p_mad_xblock_header_t     xbh        = NULL;
  p_mad_buffer_t            xbh_buffer = NULL;
  p_mad_driver_interface_t  interface  = NULL;
  p_mad_link_t              lnk        = NULL;
  unsigned char            *data       = NULL;
  tbx_bool_t                no_data    = tbx_false;

  LOG_IN();
  interface  = in->channel->adapter->driver->interface;
  xbh        = tbx_malloc(mad_xbheader_memory);
  xbh_buffer = mad_get_user_receive_buffer(xbh->data,
					   mad_xblock_fsize);

  if (interface->choice)
    {
      lnk = interface->choice(in, xbh_buffer->length,
			      mad_send_SAFER,
			      mad_receive_EXPRESS);
    }
  else
    {
      lnk = in->link_array[0];
    }

  if (lnk->buffer_mode == mad_buffer_mode_static)
    {
      p_mad_buffer_t static_buffer = NULL;

      interface->receive_buffer(lnk, &static_buffer);
      mad_copy_buffer(static_buffer, xbh_buffer);
      interface->return_static_buffer(lnk, static_buffer);
    }
  else if (lnk->buffer_mode == mad_buffer_mode_dynamic)
    {
      interface->receive_buffer(lnk, &xbh_buffer);
    }
  else
    TBX_FAILURE("invalid link mode");

  data = xbh->data;
  xbh->length        =
      tbx_expand_field(data, mad_xblock_flength_0)
    | tbx_expand_field(data, mad_xblock_flength_1)
    | tbx_expand_field(data, mad_xblock_flength_2)
    | tbx_expand_field(data, mad_xblock_flength_3);
  xbh->source        =
      tbx_expand_field(data, mad_xblock_fsource_0)
    | tbx_expand_field(data, mad_xblock_fsource_1)
    | tbx_expand_field(data, mad_xblock_fsource_2)
    | tbx_expand_field(data, mad_xblock_fsource_3);
  xbh->destination   =
      tbx_expand_field(data, mad_xblock_fdestination_0)
    | tbx_expand_field(data, mad_xblock_fdestination_1)
    | tbx_expand_field(data, mad_xblock_fdestination_2)
    | tbx_expand_field(data, mad_xblock_fdestination_3);
  xbh->mux           =
    tbx_expand_field(data, mad_xblock_fmux);
  xbh->sub           =
    tbx_expand_field(data, mad_xblock_fsub);
  xbh->is_a_group    =
    tbx_expand_field(data, mad_xblock_fis_a_group);
  xbh->is_a_new_msg  =
    tbx_expand_field(data, mad_xblock_fis_a_new_msg);
  xbh->closing       =
    tbx_expand_field(data, mad_xblock_fclosing);
#ifdef MAD_MUX_FLOW_CONTROL
  xbh->is_an_ack     =
    tbx_expand_field(data, mad_xblock_fis_an_ack);
#endif // MAD_MUX_FLOW_CONTROL

  xbh->src_interface = interface;
  if (xbh->destination != mad_xchannel->process_lrank)
    {
      xbh->xout          =
	tbx_darray_get(mad_xchannel->out_connection_darray,
		       xbh->destination);
    }
  else
    {
      xbh->xout = NULL;
    }

  no_data =
    xbh->is_a_group
#ifdef MAD_MUX_FLOW_CONTROL
    || xbh->is_an_ack
    || xbh->is_a_new_msg
#endif // MAD_MUX_FLOW_CONTROL
    || xbh->closing;

  xbh->data_available = !no_data;
  if (no_data)
    {
      xbh->buffer_type = mad_xblock_buffer_type_none;
    }
  mad_free_buffer_struct(xbh_buffer);
  LOG_OUT();

  return xbh;
}

static
void
mad_mux_read_block_data(p_mad_channel_t       mad_xchannel,
			p_mad_connection_t    in,
			p_mad_xblock_header_t xbh,
			p_mad_buffer_t        buffer)
{
  p_mad_driver_interface_t interface = NULL;

  LOG_IN();
  interface  = in->channel->adapter->driver->interface;

  if (xbh->data_available)
    {
      p_mad_link_t data_lnk = NULL;

      if (interface->choice)
	{
	  data_lnk = interface->choice(in, xbh->length,
				       mad_send_SAFER,
				       mad_receive_EXPRESS);
	}
      else
	{
	  data_lnk = in->link_array[0];
	}

      xbh->src_link = data_lnk;

      if (data_lnk->buffer_mode == mad_buffer_mode_static)
	{
	  if (buffer)
	    {
	      p_mad_buffer_t static_buffer = NULL;

	      interface->receive_buffer(data_lnk, &static_buffer);
	      mad_copy_buffer(static_buffer, buffer);
	      interface->return_static_buffer(data_lnk, static_buffer);

	      xbh->block       = buffer;
	      xbh->buffer_type = mad_xblock_buffer_type_dest;
	    }
	  else
	    {
	      interface->receive_buffer(data_lnk, &(xbh->block));
	      xbh->buffer_type = mad_xblock_buffer_type_static_src;
	    }
	}
      else if (data_lnk->buffer_mode == mad_buffer_mode_dynamic)
	{
	  if (buffer)
	    {
	      xbh->block = buffer;
	      xbh->buffer_type = mad_xblock_buffer_type_dest;
	    }
	  else
	    {
	      p_mad_connection_t xout = NULL;
	      xout = xbh->xout;

	      if (xout)
		{
		  p_mad_connection_t       out            = NULL;
		  p_mad_driver_interface_t next_interface = NULL;
		  p_mad_link_t             next_data_lnk  = NULL;

		  out            = xout->regular;
		  next_interface =
		    out->channel->adapter->driver->interface;

		  if (next_interface->choice)
		    {
		      next_data_lnk =
			next_interface->choice(out, xbh->length,
					       mad_send_SAFER,
					       mad_receive_EXPRESS);
		    }
		  else
		    {
		      next_data_lnk = out->link_array[0];
		    }


		  if (next_data_lnk->buffer_mode == mad_buffer_mode_static)
		    {
		      xbh->block       =
			next_interface->get_static_buffer(next_data_lnk);

		      xbh->buffer_type = mad_xblock_buffer_type_static_dst;
		    }
		  else if (next_data_lnk->buffer_mode ==
			   mad_buffer_mode_dynamic)
		    {
		      xbh->block       = mad_alloc_buffer(xbh->length);
		      xbh->buffer_type = mad_xblock_buffer_type_dynamic;
		    }
		  else
		    TBX_FAILURE("invalid link mode");
		}
	      else
		{
		  xbh->block       = mad_alloc_buffer(xbh->length);
		  xbh->buffer_type = mad_xblock_buffer_type_dynamic;
		}
	    }

	  if (xbh->block->length > xbh->length)
	    {
	      xbh->block->length = xbh->length;
	    }

	  interface->receive_buffer(data_lnk, &(xbh->block));
	}
      else
	TBX_FAILURE("invalid link mode");
    }
  else
    TBX_FAILURE("no data");

  LOG_OUT();
}

static
void *
mad_mux_receive_block(void *arg)
{
  p_mad_mux_receive_block_arg_t farg         = NULL;
  p_mad_channel_t               mad_channel  = NULL;
  p_mad_channel_t               mad_xchannel = NULL;
  p_mad_driver_interface_t      interface    = NULL;

  LOG_IN();
  farg         = arg;
  mad_channel  = farg->channel;
  mad_xchannel = farg->xchannel;
  TBX_FREE(arg);
  arg          = NULL;
  farg         = NULL;
  interface    = mad_channel->adapter->driver->interface;

  for (;;)
    {
      p_mad_connection_t       in               = NULL;
      p_mad_connection_t       xout             = NULL;
      p_mad_xblock_header_t    xbh              = NULL;

      TRACE("mad_mux_receive_block: waiting for a new block");
      in  = interface->receive_message(mad_channel);
      xbh = mad_mux_read_block_header(mad_xchannel, in);
      TRACE_VAL("mad_mux_receive_block: got a block on mux", xbh->mux);

      if (xbh->closing)
	{
	  if (interface->message_received)
	    interface->message_received(in);

	  tbx_free(mad_xbheader_memory, xbh);
	  xbh = NULL;
	  break;
	}

#ifdef MAD_MUX_FLOW_CONTROL
      if (xbh->is_an_ack && (xbh->destination == mad_xchannel->process_lrank))
	{
	  p_mad_connection_t ack_xout = NULL;

	  if (interface->message_received)
	    interface->message_received(in);

	  ack_xout = tbx_darray_get(mad_xchannel->out_connection_darray,
				    xbh->source);

	  marcel_sem_V(&ack_xout->ack);

	  tbx_free(mad_xbheader_memory, xbh);
	  xbh = NULL;
	  continue;
	}
#endif // MAD_MUX_FLOW_CONTROL

      xout = xbh->xout;

      if (xout)
	{
	  p_tbx_slist_t            block_slist      = NULL;
	  marcel_sem_t            *block_to_forward = NULL;
	  p_mad_connection_t       out              = NULL;

	  TRACE("mad_mux_receive_block: block is just passing by");

#ifdef MAD_MUX_FLOW_CONTROL
	  if (xbh->is_an_ack)
	    {
	      p_mad_channel_t     fchannel    = NULL;
	      p_mad_dir_channel_t dir_channel = NULL;

	      dir_channel = xout->regular->channel->dir_channel;

	      {
		char name [2 + strlen(dir_channel->name)];

		name[0] = 'f';
		strcpy(name + 1, dir_channel->name);

		fchannel =
		  tbx_htable_get(xout->regular->channel->adapter->
				 channel_htable, name);
	      }

	      out = tbx_darray_get(fchannel->out_connection_darray,
				   xout->regular->remote_rank);
	    }
	  else
#endif // MAD_MUX_FLOW_CONTROL
	    out = xout->regular;

	  TBX_LOCK_SHARED(out);
	  if (xout->nature == mad_connection_nature_mux
#ifdef MAD_MUX_FLOW_CONTROL
	      || (xbh->is_an_ack)
#endif // MAD_MUX_FLOW_CONTROL
	      )
	    {
	      if (!out->forwarding_block_list)
		{
		  p_mad_mux_reemit_block_arg_t rarg = NULL;

		  out->forwarding_block_list = tbx_slist_nil();
		  marcel_sem_init(&(out->something_to_forward), 0);

		  rarg =
		    TBX_CALLOC(1,
			       sizeof(mad_mux_reemit_block_arg_t));

		  rarg->connection = out;
		  rarg->xchannel   = mad_xchannel;

		  marcel_create(&(out->forwarding_thread),
				NULL, mad_mux_reemit_block,
				(any_t)rarg);
		  rarg = NULL;
		}

	      block_slist      = out->forwarding_block_list;
	      block_to_forward = &(out->something_to_forward);
	    }
	  else
	    TBX_FAILURE("invalid connection nature");

	  if (xbh->is_a_group)
	    {
	      unsigned int group_len = 0;

	      group_len = xbh->length;

	      TBX_LOCK_SHARED(block_slist);
	      tbx_slist_append(block_slist, xbh);
	      marcel_sem_V(block_to_forward);
	      TBX_UNLOCK_SHARED(block_slist);
	      xbh = NULL;

	      while (group_len--)
		{
		  xbh = mad_mux_read_block_header(mad_xchannel, in);
		  if (xbh->data_available)
		    {
		      mad_mux_read_block_data(mad_xchannel, in, xbh, NULL);
		    }
		  TBX_LOCK_SHARED(block_slist);
		  tbx_slist_append(block_slist, xbh);
		  marcel_sem_V(block_to_forward);
		  TBX_UNLOCK_SHARED(block_slist);
		  xbh = NULL;
		}
	    }
	  else
	    {
	      if (xbh->data_available)
		{
		  mad_mux_read_block_data(mad_xchannel, in, xbh, NULL);
		}
	      TBX_LOCK_SHARED(block_slist);
	      tbx_slist_append(block_slist, xbh);
	      marcel_sem_V(block_to_forward);
	      TBX_UNLOCK_SHARED(block_slist);
	      xbh = NULL;
	    }

	  TBX_UNLOCK_SHARED(out);
	}
      else
	{
	  /* At destination */
	  p_tbx_slist_t            message_slist = NULL;
	  ntbx_process_grank_t     source        =   -1;
	  p_tbx_darray_t           darray        = NULL;
	  p_mad_mux_darray_lane_t  lane          = NULL;
	  unsigned int             mux           =    0;
	  unsigned int             sub           =    0;
	  p_tbx_slist_t            buffer_slist  = NULL;
	  p_mad_mux_block_queue_t  block_queue      = NULL;
	  p_tbx_slist_t            block_slist      = NULL;
	  marcel_sem_t            *block_to_forward = NULL;

	  TRACE("mad_mux_receive_block: block is for me");
	  mux = xbh->mux;
	  {
	    darray = mad_xchannel->mux_list_darray;
	    darray = tbx_darray_get(darray, mux);
	  }
	  sub = xbh->sub;

	  TRACE("mad_mux_receive_block: locking mux_list_darray");
	  TBX_LOCK_SHARED(darray);
	  lane = tbx_darray_expand_and_get(darray, sub);

	  if (!lane)
	    {
	      lane = TBX_CALLOC(1, sizeof(mad_mux_darray_lane_t));

	      lane->message_queue = tbx_slist_nil();
	      lane->block_queues  = tbx_darray_init();
	      marcel_sem_init(&(lane->something_to_forward), 0);

	      TBX_INIT_SHARED(lane);

	      tbx_darray_set(darray, sub, lane);
	    }

	  TRACE("mad_mux_receive_block: locking lane");
	  TBX_LOCK_SHARED(lane);
	  TBX_UNLOCK_SHARED(darray);
	  TRACE("mad_mux_receive_block: unlocking mux_list_darray");

	  message_slist = lane->message_queue;
	  source        = xbh->source;
	  block_queue   = tbx_darray_get(lane->block_queues, source);

	  if (xbh->is_a_new_msg)
	    {
	      p_mad_xmessage_queue_entry_t entry = NULL;

	      entry = tbx_malloc(mad_mqentry_memory);
	      entry->source = source;

	      TRACE("mad_mux_receive_block: locking message list");
	      TBX_LOCK_SHARED(message_slist);
	      tbx_slist_append(message_slist, entry);
	      TBX_UNLOCK_SHARED(message_slist);
	      TRACE("mad_mux_receive_block: unlocking message list");

	      if (!block_queue)
		{
		  block_queue =
		    TBX_CALLOC(1, sizeof(mad_mux_block_queue_t));

		  block_to_forward = &(block_queue->block_to_forward);
		  marcel_sem_init(block_to_forward, 0);

		  block_slist  = tbx_slist_nil();
		  buffer_slist = tbx_slist_nil();
		  block_queue->queue        = block_slist;
		  block_queue->buffer_slist = buffer_slist;
		  tbx_darray_expand_and_set(lane->block_queues,
					    source, block_queue);
		}
	      else
		{
		  block_slist      = block_queue->queue;
		  buffer_slist     = block_queue->buffer_slist;
		  block_to_forward = &(block_queue->block_to_forward);
		}

	      marcel_sem_V(&(lane->something_to_forward));
	    }
	  else
	    {
	      block_slist      = block_queue->queue;
	      buffer_slist     = block_queue->buffer_slist;
	      block_to_forward = &(block_queue->block_to_forward);
	    }


	  if (xbh->is_a_group)
	    {
	      unsigned int   group_len = 0;
	      p_mad_buffer_t buffer    = NULL;

	      group_len = xbh->length;

	      TBX_LOCK_SHARED(block_slist);
	      tbx_slist_append(block_slist, xbh);
	      if (!tbx_slist_is_nil(buffer_slist))
		{
		  buffer = tbx_slist_dequeue(buffer_slist);
		}
	      marcel_sem_V(block_to_forward);
	      TBX_UNLOCK_SHARED(block_slist);
	      xbh = NULL;

	      while (group_len--)
		{
		  TBX_LOCK_SHARED(block_slist);
		  xbh = mad_mux_read_block_header(mad_xchannel, in);
		  if (xbh->data_available)
		    {
		      if (buffer)
			{
			  p_mad_buffer_t tmp_buffer = NULL;

			  tmp_buffer =
			    mad_get_user_receive_buffer(buffer->buffer
							+ buffer->bytes_written,
							xbh->length);

			  mad_mux_read_block_data(mad_xchannel, in, xbh,
						  tmp_buffer);
			  buffer->bytes_written += xbh->block->bytes_written;
			}
		      else
			{
			  mad_mux_read_block_data(mad_xchannel, in, xbh, NULL);
			}
		    }
		  tbx_slist_append(block_slist, xbh);
		  marcel_sem_V(block_to_forward);
		  TBX_UNLOCK_SHARED(block_slist);
		  xbh = NULL;
		}
	    }
	  else
	    {
	      p_mad_buffer_t buffer = NULL;

	      TBX_LOCK_SHARED(block_slist);
	      if (!tbx_slist_is_nil(buffer_slist))
		{
		  buffer = tbx_slist_dequeue(buffer_slist);
		}
	      if (xbh->data_available)
		{
		  if (buffer)
		    {
		      p_mad_buffer_t tmp_buffer = NULL;

		      tmp_buffer =
			mad_get_user_receive_buffer(buffer->buffer
						    + buffer->bytes_written,
						    xbh->length);

		      xbh->block = tmp_buffer;
		      mad_mux_read_block_data(mad_xchannel, in, xbh,
					      tmp_buffer);
		      buffer->bytes_written += xbh->block->bytes_written;
		    }
		  else
		    {
		      mad_mux_read_block_data(mad_xchannel, in, xbh, NULL);
		    }
		}
	      tbx_slist_append(block_slist, xbh);
	      marcel_sem_V(block_to_forward);
	      TBX_UNLOCK_SHARED(block_slist);
	      xbh = NULL;
	    }

	  TBX_UNLOCK_SHARED(lane);
	  TRACE("mad_mux_receive_block: unlocking lane");
	}

      if (interface->message_received)
	interface->message_received(in);
    }
  LOG_OUT();

  return NULL;
}


/*
 * Generic forwarding transmission module
 * --------------------------------------
 */
static
void
mad_mux_fill_xbh_data(unsigned char *data,
		      unsigned int   src,
		      unsigned int   dst,
		      unsigned int   length,
		      unsigned int   mux,
		      unsigned int   sub,
		      tbx_bool_t     is_a_group,
		      tbx_bool_t     is_a_new_msg,
		      tbx_bool_t     closing
#ifdef MAD_MUX_FLOW_CONTROL
		      , tbx_bool_t     is_an_ack
#endif // MAD_MUX_FLOW_CONTROL
			  )
{
  unsigned int uint_is_a_group    = 0;
  unsigned int uint_is_a_new_msg  = 0;
  unsigned int uint_closing       = 0;
#ifdef MAD_MUX_FLOW_CONTROL
  unsigned int uint_is_an_ack     = 0;
#endif // MAD_MUX_FLOW_CONTROL

  LOG_IN();
  uint_is_a_group    =    (is_a_group)?1:0;
  uint_is_a_new_msg  =  (is_a_new_msg)?1:0;
  uint_closing       =       (closing)?1:0;
#ifdef MAD_MUX_FLOW_CONTROL
  uint_is_an_ack     =     (is_an_ack)?1:0;
#endif // MAD_MUX_FLOW_CONTROL

  tbx_contract_field(data, length, mad_xblock_flength_0);
  tbx_contract_field(data, length, mad_xblock_flength_1);
  tbx_contract_field(data, length, mad_xblock_flength_2);
  tbx_contract_field(data, length, mad_xblock_flength_3);

  tbx_contract_field(data, src, mad_xblock_fsource_0);
  tbx_contract_field(data, src, mad_xblock_fsource_1);
  tbx_contract_field(data, src, mad_xblock_fsource_2);
  tbx_contract_field(data, src, mad_xblock_fsource_3);

  tbx_contract_field(data, dst, mad_xblock_fdestination_0);
  tbx_contract_field(data, dst, mad_xblock_fdestination_1);
  tbx_contract_field(data, dst, mad_xblock_fdestination_2);
  tbx_contract_field(data, dst, mad_xblock_fdestination_3);

  tbx_contract_field(data, mux, mad_xblock_fmux);
  tbx_contract_field(data, sub, mad_xblock_fsub);

  tbx_contract_field(data, uint_is_a_group,    mad_xblock_fis_a_group);
  tbx_contract_field(data, uint_is_a_new_msg,  mad_xblock_fis_a_new_msg);
  tbx_contract_field(data, uint_closing,       mad_xblock_fclosing);

#ifdef MAD_MUX_FLOW_CONTROL
  tbx_contract_field(data, uint_is_an_ack, mad_xblock_fis_an_ack);
#endif // MAD_MUX_FLOW_CONTROL
  LOG_OUT();
}

#ifdef MAD_MUX_FLOW_CONTROL
#define FILL_XBH_DATA() \
    mad_mux_fill_xbh_data(_data, _src, _dst, _length, _mux, _sub,\
			  _is_a_group, _is_a_new_msg, \
			  _closing, _is_an_ack)
#else
#define FILL_XBH_DATA() \
    mad_mux_fill_xbh_data(_data, _src, _dst, _length, _mux, _sub,\
			  _is_a_group, _is_a_new_msg, \
			  _closing)
#endif /* MAD_MUX_FLOW_CONTROL */

static
void
mad_mux_send_bytes(p_mad_connection_t  out,
		   void               *ptr,
		   unsigned int        length)
{
  p_mad_driver_interface_t interface = NULL;
  p_mad_link_t             lnk       = NULL;
  p_mad_buffer_t           buffer    = NULL;

  LOG_IN();
  buffer = mad_get_user_send_buffer(ptr, length);

  interface = out->channel->adapter->driver->interface;

  if (interface->choice)
    {
      lnk = interface->choice(out, length,
			      mad_send_SAFER, mad_receive_EXPRESS);
    }
  else
    {
      lnk = out->link_array[0];
    }

  if (lnk->buffer_mode == mad_buffer_mode_static)
    {
      p_mad_buffer_t static_buffer = NULL;

      static_buffer = interface->get_static_buffer(lnk);
      mad_copy_buffer(buffer, static_buffer);
      interface->send_buffer(lnk, static_buffer);
    }
  else if (lnk->buffer_mode == mad_buffer_mode_dynamic)
    {
      interface->send_buffer(lnk, buffer);
    }
  else
    TBX_FAILURE("invalid link mode");

  mad_free_buffer_struct(buffer);
  LOG_OUT();
}

static
void
mad_mux_receive_bytes(p_mad_connection_t  in,
		      void               *ptr,
		      unsigned int        length) TBX_UNUSED;

static
void
mad_mux_receive_bytes(p_mad_connection_t  in,
		      void               *ptr,
		      unsigned int        length)
{
  p_mad_driver_interface_t interface = NULL;
  p_mad_link_t             lnk       = NULL;
  p_mad_buffer_t           buffer    = NULL;

  LOG_IN();
  buffer = mad_get_user_receive_buffer(ptr, length);

  interface = in->channel->adapter->driver->interface;

  if (interface->choice)
    {
      lnk = interface->choice(in, buffer->length,
			      mad_send_SAFER, mad_receive_EXPRESS);
    }
  else
    {
      lnk = in->link_array[0];
    }

  if (lnk->buffer_mode == mad_buffer_mode_static)
    {
      p_mad_buffer_t static_buffer = NULL;

      interface->receive_buffer(lnk, &static_buffer);
      mad_copy_buffer(static_buffer, buffer);
      interface->return_static_buffer(lnk, static_buffer);
    }
  else if (lnk->buffer_mode == mad_buffer_mode_dynamic)
    {
      interface->receive_buffer(lnk, &buffer);
    }
  else
    TBX_FAILURE("invalid link mode");

  mad_free_buffer_struct(buffer);
  LOG_OUT();
}

char *
mad_mux_register(p_mad_driver_interface_t interface)
{
  LOG_IN();
  TRACE("Registering MUX driver");
  interface->driver_init                =
    mad_mux_driver_init;
  interface->adapter_init               = NULL;
  interface->channel_init               =
    mad_mux_channel_init;
  interface->before_open_channel        =
    mad_mux_before_open_channel;
  interface->connection_init            =
    mad_mux_connection_init;
  interface->link_init                  =
    mad_mux_link_init;
  interface->accept                     = NULL;
  interface->connect                    = NULL;
  interface->after_open_channel         =
    mad_mux_after_open_channel;
  interface->before_close_channel       =
    mad_mux_before_close_channel;
  interface->disconnect                 =
    mad_mux_disconnect;
  interface->after_close_channel        =
    mad_mux_after_close_channel;
  interface->link_exit                  =
    mad_mux_link_exit;
  interface->connection_exit            =
    mad_mux_connection_exit;
  interface->channel_exit               =
    mad_mux_channel_exit;
  interface->adapter_exit               = NULL;
  interface->driver_exit                =
    mad_mux_driver_exit;
  interface->choice                     =
    mad_mux_choice;
  interface->get_static_buffer          =
    mad_mux_get_static_buffer;
  interface->return_static_buffer       =
    mad_mux_return_static_buffer;
  interface->new_message                =
    mad_mux_new_message;
  interface->finalize_message           =
    mad_mux_finalize_message;
  interface->message_received           =
    mad_mux_message_received;
#ifdef MAD_MESSAGE_POLLING
  interface->poll_message               =
    mad_mux_poll_message;
#endif // MAD_MESSAGE_POLLING
  interface->receive_message            =
    mad_mux_receive_message;
  interface->send_buffer                =
    mad_mux_send_buffer;
  interface->receive_buffer             =
    mad_mux_receive_buffer;
  interface->send_buffer_group          =
    mad_mux_send_buffer_group;
  interface->receive_sub_buffer_group   =
    mad_mux_receive_sub_buffer_group;
  interface->get_sub_channel            =
    mad_mux_get_sub_channel;
  LOG_OUT();

  return "mux";
}

void
mad_mux_driver_init(p_mad_driver_t driver, int *argc, char ***argv)
{
  p_mad_adapter_t dummy_adapter = NULL;

  LOG_IN();
  driver->connection_type  = mad_unidirectional_connection;
  driver->buffer_alignment = 64;

  dummy_adapter                 = mad_adapter_cons();
  dummy_adapter->driver         = driver;
  dummy_adapter->id             = -1;
  dummy_adapter->selector       = tbx_strdup("-");
  dummy_adapter->parameter      = tbx_strdup("-");
  dummy_adapter->channel_htable = tbx_htable_empty_table();

  tbx_htable_add(driver->adapter_htable, "mux",
		 dummy_adapter);
  LOG_OUT();
}


void
mad_mux_channel_init(p_mad_channel_t channel)
{
  LOG_IN();
  channel->specific = NULL;

  channel->max_mux = MAD_MUX_MAX_MUX;
  channel->max_sub = MAD_MUX_MAX_SUB;
  LOG_OUT();
}

void
mad_mux_connection_init(p_mad_connection_t in,
			    p_mad_connection_t out)
{

  LOG_IN();
  if (in)
    {
#ifdef MAD_MUX_FLOW_CONTROL
      marcel_sem_init(&in->ack, 0);
#endif // MAD_MUX_FLOW_CONTROL

      in->specific = NULL;
      in->nb_link  = 1;
    }

  if (out)
    {
#ifdef MAD_MUX_FLOW_CONTROL
      marcel_sem_init(&out->ack, 0);
#endif // MAD_MUX_FLOW_CONTROL

      out->specific = NULL;
      out->nb_link  = 1;
    }
  LOG_OUT();
}

void
mad_mux_link_init(p_mad_link_t lnk)
{
  p_mad_connection_t connection = NULL;

  LOG_IN();
  connection = lnk->connection;
  /*  lnk->link_mode   = mad_link_mode_buffer; */
  lnk->link_mode   = mad_link_mode_buffer_group;
  lnk->buffer_mode = mad_buffer_mode_dynamic;
  lnk->group_mode  = mad_group_mode_split;
  LOG_OUT();
}

void
mad_mux_before_open_channel(p_mad_channel_t channel)
{
  LOG_IN();
  /* nothing */
  LOG_OUT();
}

void
mad_mux_after_open_channel(p_mad_channel_t channel)
{
  p_tbx_slist_t slist = NULL;

  LOG_IN();
  slist = channel->channel_slist;

  tbx_slist_ref_to_head(slist);
  do
    {
      p_mad_channel_t               regular_channel = NULL;
      p_mad_mux_receive_block_arg_t farg            = NULL;

      regular_channel = tbx_slist_ref_get(slist);

      farg = TBX_CALLOC(1, sizeof(mad_mux_receive_block_arg_t));
      farg->channel  = regular_channel;
      farg->xchannel = channel;

      marcel_create(&(regular_channel->polling_thread),
		    NULL, mad_mux_receive_block,
		    (any_t)farg);
    }
  while (tbx_slist_ref_forward(slist));

  channel->specific = NULL;
  LOG_OUT();
}

void
mad_mux_disconnect(p_mad_connection_t connection)
{
  LOG_IN();
  /* currently unimplemented */
  LOG_OUT();
}

void
mad_mux_new_message(p_mad_connection_t xout)
{
  p_mad_channel_t channel = NULL;
  unsigned int    mux     =    0;
  unsigned int    sub     =    0;

  LOG_IN();
  channel = xout->channel;
  mux     = channel->mux;
  sub     = channel->sub;

#ifdef MAD_MUX_FLOW_CONTROL
  {
    p_mad_connection_t out            = NULL;
    unsigned int       src            =    0;
    unsigned int       dst            =    0;
    p_mad_driver_interface_t interface = NULL;
    unsigned char      data[mad_xblock_fsize] = {0};

    out = xout->regular;
    interface = out->channel->adapter->driver->interface;
    src = (unsigned int)xout->channel->process_lrank;
    dst = (unsigned int)xout->remote_rank;

    {
      unsigned char *_data          = data;
      unsigned int   _src           = src;
      unsigned int   _dst           = dst;
      unsigned int   _length        = 0;
      unsigned int   _mux           = mux;
      unsigned int   _sub           = sub;
      tbx_bool_t     _is_a_group    = tbx_false;
      tbx_bool_t     _is_a_new_msg  = tbx_true;
      tbx_bool_t     _closing       = tbx_false;
      tbx_bool_t     _is_an_ack     = tbx_false;

      FILL_XBH_DATA();
    }

    marcel_mutex_lock(&(out->lock_mutex));
    if (interface->new_message)
      interface->new_message(out);

    mad_mux_send_bytes(out, data, mad_xblock_fsize);

    if (interface->finalize_message)
      interface->finalize_message(out);

    marcel_mutex_unlock(&(out->lock_mutex));
    marcel_sem_P(&xout->ack);
  }
#else // MAD_MUX_FLOW_CONTROL
  xout->new_msg = tbx_true;
#endif // MAD_MUX_FLOW_CONTROL
  LOG_OUT();
}

void
mad_mux_finalize_message(p_mad_connection_t xout)
{
#if 0
  p_mad_channel_t          channel   = NULL;
  unsigned int             mux       =    0;
  unsigned int             sub       =    0;
  p_mad_connection_t       out       = NULL;
  unsigned int             src       =    0;
  unsigned int             dst       =    0;
  p_mad_driver_interface_t interface = NULL;
  unsigned char            data[mad_xblock_fsize] = {0};

  LOG_IN();
  channel   = xout->channel;
  mux       = channel->mux;
  sub       = channel->sub;
  out       = xout->regular;
  interface = out->channel->adapter->driver->interface;
  src       = (unsigned int)xout->channel->process_lrank;
  dst       = (unsigned int)xout->remote_rank;

  {
    unsigned char *_data          = data;
    unsigned int   _src           = src;
    unsigned int   _dst           = dst;
    unsigned int   _length        = 0;
    unsigned int   _mux           = mux;
    unsigned int   _sub           = sub;
    tbx_bool_t     _is_a_group    = tbx_false;
    tbx_bool_t     _is_a_new_msg  = tbx_false;
    tbx_bool_t     _closing       = tbx_false;
#ifdef MAD_MUX_FLOW_CONTROL
    tbx_bool_t     _is_an_ack     = tbx_false;
#endif // MAD_MUX_FLOW_CONTROL

    FILL_XBH_DATA();
  }

  marcel_mutex_lock(&(out->lock_mutex));
  if (interface->new_message)
    interface->new_message(out);

  mad_mux_send_bytes(out, data, mad_xblock_fsize);

  if (interface->finalize_message)
    interface->finalize_message(out);

  marcel_mutex_unlock(&(out->lock_mutex));
  LOG_OUT();
#endif // 0
}

#ifdef MAD_MESSAGE_POLLING
p_mad_connection_t
mad_mux_poll_message(p_mad_channel_t channel)
{
  p_mad_connection_t connection = NULL;
  unsigned int       mux        =    0;
  unsigned int       sub        =    0;

  LOG_IN();
  mux = channel->mux;
  sub = channel->sub;

#error revoir
  if (!channel->can_receive)
    {
      channel->a_message_is_ready = tbx_false;
      channel->can_receive        = tbx_true;
      marcel_sem_V(&(channel->ready_to_receive));
    }

  if (channel->a_message_is_ready)
    {
      marcel_sem_P(&(channel->message_ready));

      connection = channel->active_connection;
    }

#ifdef MAD_MUX_FLOW_CONTROL
    {
      p_mad_connection_t xout = NULL;
      p_mad_connection_t out  = NULL;
      unsigned char      data[mad_xblock_fsize] = {0};
      p_mad_driver_interface_t interface = NULL;

      xout = connection->reverse;
      out = xout->regular;

      interface = out->channel->adapter->driver->interface;

      marcel_mutex_lock(&(out->lock_mutex));

      if (interface->new_message)
	interface->new_message(out);

      {
	unsigned char *_data          = data;
	unsigned int   _src           = channel->process_lrank;
	unsigned int   _dst           = xout->remote_rank;
	unsigned int   _length        = 0;
	unsigned int   _mux           = mux;
	unsigned int   _sub           = sub;
	tbx_bool_t     _is_a_group    = tbx_false;
	tbx_bool_t     _is_a_new_msg  = tbx_false;
	tbx_bool_t     _closing       = tbx_false;
	tbx_bool_t     _is_an_ack     = tbx_true;

	FILL_XBH_DATA();
      }

      mad_mux_send_bytes(out, data, mad_xblock_fsize);

      if (interface->finalize_message)
	interface->finalize_message(out);

      marcel_mutex_unlock(&(out->lock_mutex));
      connection->new_msg = tbx_false;
   }
#endif // MAD_MUX_FLOW_CONTROL
  LOG_OUT();

  return connection;
}
#endif // MAD_MESSAGE_POLLING

p_mad_connection_t
mad_mux_receive_message(p_mad_channel_t channel)
{
  p_tbx_slist_t            message_slist      = NULL;
  marcel_sem_t            *message_to_forward = NULL;
  unsigned int             mux                =    0;
  unsigned int             sub                =    0;
  p_mad_mux_darray_lane_t  lane               = NULL;
  p_tbx_darray_t           darray             = NULL;
  p_mad_connection_t       xin                = NULL;

  LOG_IN();
  mux    = channel->mux;
  sub    = channel->sub;
  darray = channel->sub_list_darray;

  TBX_LOCK_SHARED(darray);
  lane = tbx_darray_expand_and_get(darray, sub);

  if (!lane)
    {
      lane = TBX_CALLOC(1, sizeof(mad_mux_darray_lane_t));

      lane->message_queue = tbx_slist_nil();
      lane->block_queues  = tbx_darray_init();
      marcel_sem_init(&(lane->something_to_forward), 0);

      TBX_INIT_SHARED(lane);

      tbx_darray_set(darray, sub, lane);
    }
  TBX_UNLOCK_SHARED(darray);

  message_slist      =  lane->message_queue;
  message_to_forward = &(lane->something_to_forward);

  {
    p_mad_xmessage_queue_entry_t entry  = NULL;
    ntbx_process_grank_t         source =   -1;

    marcel_sem_P(message_to_forward);

    TBX_LOCK_SHARED(message_slist);
    entry = tbx_slist_extract(message_slist);
    TBX_UNLOCK_SHARED(message_slist);

    source = entry->source;

    tbx_free(mad_mqentry_memory, entry);
    entry = NULL;

    xin = tbx_darray_get(channel->in_connection_darray, source);
  }

#ifdef MAD_MUX_FLOW_CONTROL
    {
      p_mad_connection_t xout = NULL;
      p_mad_connection_t out  = NULL;
      unsigned char      data[mad_xblock_fsize] = {0};
      p_mad_driver_interface_t interface = NULL;

      xout = xin->reverse;
      out  = xout->regular;

      interface = out->channel->adapter->driver->interface;

      marcel_mutex_lock(&(out->lock_mutex));

      if (interface->new_message)
	interface->new_message(out);

      {
	unsigned char *_data          = data;
	unsigned int   _src           = channel->process_lrank;
	unsigned int   _dst           = xout->remote_rank;
	unsigned int   _length        = 0;
	unsigned int   _mux           = mux;
	unsigned int   _sub           = sub;
	tbx_bool_t     _is_a_group    = tbx_false;
	tbx_bool_t     _is_a_new_msg  = tbx_false;
	tbx_bool_t     _closing       = tbx_false;
	tbx_bool_t     _is_an_ack     = tbx_true;

	FILL_XBH_DATA();
      }

      mad_mux_send_bytes(out, data, mad_xblock_fsize);

      if (interface->finalize_message)
	interface->finalize_message(out);

      marcel_mutex_unlock(&(out->lock_mutex));
      xin->new_msg = tbx_false;
   }
#endif // MAD_MUX_FLOW_CONTROL
  LOG_OUT();

  return xin;
}

void
mad_mux_message_received(p_mad_connection_t xin)
{
  LOG_IN();
  /* unimplemented */
  LOG_OUT();
}

void
mad_mux_before_close_channel(p_mad_channel_t channel)
{
  LOG_IN();
  /* unimplemented */
  LOG_OUT();
}

void
mad_mux_after_close_channel(p_mad_channel_t channel)
{
  LOG_IN();
  /* unimplemented */
  LOG_OUT();
}

void
mad_mux_link_exit(p_mad_link_t link)
{
  LOG_IN();
  /* unimplemented */
  LOG_OUT();
}

void
mad_mux_connection_exit(p_mad_connection_t in,
			    p_mad_connection_t out)
{
  LOG_IN();
  /* unimplemented */
  LOG_OUT();
}

void
mad_mux_channel_exit(p_mad_channel_t channel)
{
  LOG_IN();
  /* unimplemented */
  LOG_OUT();
}

void
mad_mux_driver_exit(p_mad_driver_t driver)
{
  p_mad_adapter_t dummy_adapter = NULL;

  LOG_IN();
  dummy_adapter =  tbx_htable_extract(driver->adapter_htable, "mux");
  TBX_FREE(dummy_adapter->selector);
  dummy_adapter->selector	= NULL;

  TBX_FREE(dummy_adapter->parameter);
  dummy_adapter->parameter	= NULL;

  tbx_htable_free(dummy_adapter->channel_htable);
  dummy_adapter->channel_htable	= NULL;

  TBX_FREE(dummy_adapter);

  tbx_htable_free(driver->adapter_htable);
  driver->adapter_htable	= NULL;
  LOG_OUT();
}

p_mad_link_t
mad_mux_choice(p_mad_connection_t connection,
		   size_t             size         __attribute__ ((unused)),
		   mad_send_mode_t    send_mode    __attribute__ ((unused)),
		   mad_receive_mode_t receive_mode __attribute__ ((unused)))
{
  p_mad_link_t lnk = NULL;

  LOG_IN();
  lnk = connection->link_array[0];
  LOG_OUT();

  return lnk;
}

void
mad_mux_return_static_buffer(p_mad_link_t   lnk,
				 p_mad_buffer_t buffer)
{
  LOG_IN();
  TBX_FAILURE("invalid function call");
  LOG_OUT();
}

p_mad_buffer_t
mad_mux_get_static_buffer(p_mad_link_t lnk)
{
  p_mad_buffer_t buffer = NULL;

  LOG_IN();
  TBX_FAILURE("invalid function call");
  LOG_OUT();

  return buffer;
}


void
mad_mux_send_buffer(p_mad_link_t   lnk,
		    p_mad_buffer_t buffer)
{
  p_mad_channel_t          channel   = NULL;
  unsigned int             mux       =    0;
  unsigned int             sub       =    0;
  p_mad_connection_t       xout      = NULL;
  p_mad_connection_t       out       = NULL;
  p_mad_driver_interface_t interface = NULL;

  LOG_IN();
  xout      = lnk->connection;
  channel   = xout->channel;
  mux       = channel->mux;
  sub       = channel->sub;
  out       = xout->regular;
  interface = out->channel->adapter->driver->interface;

  {
    unsigned int   mtu            =    0;
    unsigned int   nb_block       =    0;
    unsigned int   last_block_len =    0;
    unsigned int   src            =    0;
    unsigned int   dst            =    0;
    tbx_bool_t     is_a_new_msg   = tbx_false;
    unsigned char *ptr            = NULL;
    unsigned int   bytes_read     =    0;
    unsigned char  data[mad_xblock_fsize] = {0};

    mtu            = xout->mtu;
    nb_block       =
      (buffer->bytes_written - buffer->bytes_read) / mtu;
    last_block_len =
      (buffer->bytes_written - buffer->bytes_read) % mtu;
    bytes_read     = buffer->bytes_read;
    ptr            = buffer->buffer;
    src            = (unsigned int)xout->channel->process_lrank;
    dst            = (unsigned int)xout->remote_rank;
    is_a_new_msg   = xout->new_msg;
    xout->new_msg  = tbx_false;

    if (!last_block_len)
      {
	nb_block --;
	last_block_len = mtu;
      }

    marcel_mutex_lock(&(out->lock_mutex));

    if (interface->new_message)
      interface->new_message(out);

    if (nb_block)
      {
	{
	  unsigned char *_data          = data;
	  unsigned int   _src           = src;
	  unsigned int   _dst           = dst;
	  unsigned int   _length        = nb_block + 1;
	  unsigned int   _mux           = mux;
	  unsigned int   _sub           = sub;
	  tbx_bool_t     _is_a_group    = tbx_true;
	  tbx_bool_t     _is_a_new_msg  = is_a_new_msg;
	  tbx_bool_t     _closing       = tbx_false;
#ifdef MAD_MUX_FLOW_CONTROL
	  tbx_bool_t     _is_an_ack     = tbx_false;
#endif // MAD_MUX_FLOW_CONTROL

	  FILL_XBH_DATA();
	}
	is_a_new_msg = tbx_false;
	mad_mux_send_bytes(out, data, mad_xblock_fsize);

	do
	  {
	    {
	      unsigned char *_data          = data;
	      unsigned int   _src           = src;
	      unsigned int   _dst           = dst;
	      unsigned int   _length        = mtu;
	      unsigned int   _mux           = mux;
	      unsigned int   _sub           = sub;
	      tbx_bool_t     _is_a_group    = tbx_false;
	      tbx_bool_t     _is_a_new_msg  = tbx_false;
	      tbx_bool_t     _closing       = tbx_false;
#ifdef MAD_MUX_FLOW_CONTROL
	      tbx_bool_t     _is_an_ack     = tbx_false;
#endif // MAD_MUX_FLOW_CONTROL

	      FILL_XBH_DATA();
	    }
	    mad_mux_send_bytes(out, data, mad_xblock_fsize);
	    mad_mux_send_bytes(out, ptr + bytes_read, mtu);
	    bytes_read += mtu;
	  }
	while (--nb_block);
      }

    if (!last_block_len)
      TBX_FAILURE("no data");

    {
      unsigned char *_data          = data;
      unsigned int   _src           = src;
      unsigned int   _dst           = dst;
      unsigned int   _length        = last_block_len;
      unsigned int   _mux           = mux;
      unsigned int   _sub           = sub;
      tbx_bool_t     _is_a_group    = tbx_false;
      tbx_bool_t     _is_a_new_msg  = is_a_new_msg;
      tbx_bool_t     _closing       = tbx_false;
#ifdef MAD_MUX_FLOW_CONTROL
      tbx_bool_t     _is_an_ack     = tbx_false;
#endif // MAD_MUX_FLOW_CONTROL

      FILL_XBH_DATA();
    }
    mad_mux_send_bytes(out, data, mad_xblock_fsize);
    mad_mux_send_bytes(out, ptr + bytes_read, last_block_len);
    buffer->bytes_read = (size_t)(bytes_read + last_block_len);

    if (interface->finalize_message)
      interface->finalize_message(out);

    marcel_mutex_unlock(&(out->lock_mutex));
  }
  LOG_OUT();
}


static
void
mad_mux_extract_buffer(p_mad_link_t            lnk,
		       p_mad_mux_block_queue_t block_queue,
		       p_mad_buffer_t          buffer)
{
  p_mad_connection_t       xin              = NULL;
  p_mad_xblock_header_t    xbh              = NULL;
  p_tbx_slist_t            block_slist      = NULL;
  marcel_sem_t            *block_to_forward = NULL;
  unsigned int             nb_block         =    0;
  unsigned int             block_len        =    0;

  LOG_IN();
  xin     = lnk->connection;

  block_to_forward = &(block_queue->block_to_forward);
  block_slist      = block_queue->queue;

  marcel_sem_P(block_to_forward);
  TBX_LOCK_SHARED(block_slist);
  xbh = tbx_slist_extract(block_slist);
  TBX_UNLOCK_SHARED(block_slist);

  if (xin->new_msg)
    {
      xin->new_msg = tbx_false;
    }

  if (xbh->is_a_group)
    {
      nb_block = xbh->length;
    }
  else
    {
      block_len = xbh->length;
      nb_block  = 1;
    }

  if (nb_block > 1)
    {
      tbx_free(mad_xbheader_memory, xbh);
      xbh = NULL;

      while (nb_block--)
	{
	  marcel_sem_P(block_to_forward);
	  TBX_LOCK_SHARED(block_slist);
	  xbh = tbx_slist_extract(block_slist);
	  TBX_UNLOCK_SHARED(block_slist);

	  if (xbh->buffer_type == mad_xblock_buffer_type_dest)
	    {
	      mad_free_buffer(xbh->block);
	    }
	  else if (xbh->buffer_type == mad_xblock_buffer_type_static_src)
	    {
	      mad_copy_buffer(xbh->block, buffer);
	      xbh->src_interface->
		return_static_buffer(xbh->src_link, xbh->block);
	    }
	  else if (xbh->buffer_type == mad_xblock_buffer_type_dynamic)
	    {
	      mad_copy_buffer(xbh->block, buffer);
	      mad_free_buffer(xbh->block);
	    }
	  else
	    TBX_FAILURE("invalid block type");

	  tbx_free(mad_xbheader_memory, xbh);
	  xbh = NULL;
	}
    }
  else
    {
      if (xbh->buffer_type == mad_xblock_buffer_type_dest)
	{
	  mad_free_buffer(xbh->block);
	}
      else if (xbh->buffer_type == mad_xblock_buffer_type_static_src)
	{
	  mad_copy_buffer(xbh->block, buffer);
	  xbh->src_interface->
	    return_static_buffer(xbh->src_link, xbh->block);
	}
      else if (xbh->buffer_type == mad_xblock_buffer_type_dynamic)
	{
	  mad_copy_buffer(xbh->block, buffer);
	  mad_free_buffer(xbh->block);
	}
      else
	TBX_FAILURE("invalid block type");

      tbx_free(mad_xbheader_memory, xbh);
      xbh = NULL;
    }
  LOG_OUT();
}


void
mad_mux_receive_buffer(p_mad_link_t    lnk,
		       p_mad_buffer_t *buffer)
{
  p_mad_channel_t          channel          = NULL;
  p_mad_connection_t       xin              = NULL;
  ntbx_process_grank_t     source           =   -1;
  unsigned int             mux              =    0;
  unsigned int             sub              =    0;
  p_mad_mux_darray_lane_t  lane             = NULL;
  p_tbx_darray_t           darray           = NULL;
  p_mad_mux_block_queue_t  block_queue      = NULL;
  p_tbx_slist_t            block_slist      = NULL;
  p_mad_buffer_t           buf              = NULL;
  p_tbx_slist_t            buffer_slist     = NULL;

  LOG_IN();
  xin     = lnk->connection;
  source  = xin->remote_rank;
  channel = xin->channel;

  mux = channel->mux;
  sub = channel->sub;
  darray = channel->sub_list_darray;

  TBX_LOCK_SHARED(darray);
  lane = tbx_darray_get(darray, sub);
  TBX_UNLOCK_SHARED(darray);

  block_queue      = tbx_darray_get(lane->block_queues, source);
  block_slist      = block_queue->queue;
  buffer_slist     = block_queue->buffer_slist;

  buf = *buffer;

  TBX_LOCK_SHARED(block_slist);
  if (tbx_slist_is_nil(block_slist))
    {
      TRACE("%s: Preposting a block!", __FUNCTION__);
      tbx_slist_enqueue(buffer_slist, buf);
    }
  else
    {
      TRACE("Data already here!");
    }
  TBX_UNLOCK_SHARED(block_slist);

  mad_mux_extract_buffer(lnk, block_queue, buf);
  LOG_OUT();
}

void
mad_mux_send_buffer_group(p_mad_link_t         lnk,
			      p_mad_buffer_group_t buffer_group)
{
  p_mad_connection_t xout = NULL;

  LOG_IN();
  xout = lnk->connection;

  if (!tbx_empty_list(&(buffer_group->buffer_list)))
    {
      tbx_list_reference_t ref;

      tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
      do
	{
	  p_mad_buffer_t buffer = NULL;

	  buffer = tbx_get_list_reference_object(&ref);
	  mad_mux_send_buffer(lnk, buffer);
	}
      while(tbx_forward_list_reference(&ref));
    }
  LOG_OUT();
}

void
mad_mux_receive_sub_buffer_group(p_mad_link_t         lnk,
				 tbx_bool_t           first_sub_group
				 __attribute__ ((unused)),
				 p_mad_buffer_group_t buffer_group)
{
  p_mad_channel_t          channel          = NULL;
  p_mad_connection_t       xin              = NULL;
  ntbx_process_grank_t     source           =   -1;
  unsigned int             mux              =    0;
  unsigned int             sub              =    0;
  p_mad_mux_darray_lane_t  lane             = NULL;
  p_tbx_darray_t           darray           = NULL;
  p_mad_mux_block_queue_t  block_queue      = NULL;
  p_tbx_slist_t            block_slist      = NULL;
  p_tbx_slist_t            buffer_slist     = NULL;

  LOG_IN();
  xin     = lnk->connection;
  source  = xin->remote_rank;
  channel = xin->channel;

  mux = channel->mux;
  sub = channel->sub;
  darray = channel->sub_list_darray;

  TBX_LOCK_SHARED(darray);
  lane = tbx_darray_get(darray, sub);
  TBX_UNLOCK_SHARED(darray);

  block_queue      = tbx_darray_get(lane->block_queues, source);
  block_slist      = block_queue->queue;
  buffer_slist     = block_queue->buffer_slist;

  if (!tbx_empty_list(&(buffer_group->buffer_list)))
    {
      p_tbx_slist_t        slist = NULL;
      tbx_list_reference_t ref;

      tbx_list_reference_init(&ref, &(buffer_group->buffer_list));

      do
	{
	  TBX_LOCK_SHARED(block_slist);
	  if (tbx_slist_is_nil(block_slist))
	    {
	      slist = tbx_slist_nil();

	      do
		{
		  p_mad_buffer_t buffer = NULL;

		  TRACE("%s: Preposting a block!", __FUNCTION__);
		  buffer = tbx_get_list_reference_object(&ref);
		  tbx_slist_append(slist, buffer);
		  tbx_slist_enqueue(buffer_slist, buffer);
		}
	      while(tbx_forward_list_reference(&ref));

	      TBX_UNLOCK_SHARED(block_slist);

	      break;
	    }
	  else
	    {
	      p_mad_buffer_t buffer = NULL;

	      TRACE("Data already here!");
	      TBX_UNLOCK_SHARED(block_slist);
	      buffer = tbx_get_list_reference_object(&ref);
	      mad_mux_extract_buffer(lnk, block_queue, buffer);
	    }
	}
      while(tbx_forward_list_reference(&ref));

      if (slist)
	{
	  do
	    {
	      p_mad_buffer_t buffer = NULL;

	      buffer = tbx_slist_extract(slist);
	      mad_mux_extract_buffer(lnk, block_queue, buffer);
	    }
	  while (!tbx_slist_is_nil(slist));

	  tbx_slist_free(slist);
	  slist = NULL;
	}
    }
  LOG_OUT();
}

// Specific clean-up functions
void
mad_mux_stop_indirect_retransmit(p_mad_channel_t channel)
{
  p_tbx_darray_t     out_darray  = NULL;
  size_t             len         =    0;
  tbx_darray_index_t idx         =    0;

  LOG_IN();
  out_darray  = channel->out_connection_darray;
  len         = tbx_darray_length(out_darray);

  while (idx < len)
    {
      p_mad_connection_t out = NULL;

      out = tbx_darray_get(out_darray, idx);

      if (out)
	{
	  out->closing = tbx_true;

	  if (out->forwarding_block_list)
	    {
	      any_t status = NULL;

	      marcel_sem_V(&(out->something_to_forward));
	      marcel_join(out->forwarding_thread, &status);
	      out->forwarding_thread = NULL;

	      tbx_slist_free(out->forwarding_block_list);
	      out->forwarding_block_list = NULL;
	    }
	}

      idx++;
    }
  LOG_OUT();
}

void
mad_mux_stop_reception(p_mad_channel_t      xchannel,
		       p_mad_channel_t      channel,
		       ntbx_process_lrank_t lrank)
{
  LOG_IN();

  if (lrank == -1)
    {
      // Receiver
      any_t status = NULL;

      marcel_join(channel->polling_thread, &status);
      channel->polling_thread = NULL;
    }
  else
    {
      // Sender
      p_mad_connection_t       out       = NULL;
      p_mad_driver_interface_t interface = NULL;
      unsigned char            data[mad_xblock_fsize] = {0};

      out = tbx_darray_get(channel->out_connection_darray, lrank);

	{
	  unsigned char *_data          = data;
	  unsigned int   _src           = 0;
	  unsigned int   _dst           = 0;
	  unsigned int   _length        = 0;
	  unsigned int   _mux           = 0;
	  unsigned int   _sub           = 0;
	  tbx_bool_t     _is_a_group    = tbx_false;
	  tbx_bool_t     _is_a_new_msg  = tbx_false;
	  tbx_bool_t     _closing       = tbx_true;
#ifdef MAD_MUX_FLOW_CONTROL
	  tbx_bool_t     _is_an_ack     = tbx_false;
#endif // MAD_MUX_FLOW_CONTROL

	  FILL_XBH_DATA();
	}

      interface = out->channel->adapter->driver->interface;
      if (interface->new_message)
	interface->new_message(out);

      mad_mux_send_bytes(out, data, mad_xblock_fsize);

      if (interface->finalize_message)
	interface->finalize_message(out);
    }
  LOG_OUT();
}

static
p_mad_channel_t
__mad_mux_generate_sub_channel_skel(p_mad_channel_t xchannel)
{
  p_mad_channel_t channel = NULL;

  LOG_IN();
  channel = mad_channel_cons();
  channel->process_lrank = xchannel->process_lrank;
  channel->type          = mad_channel_type_mux;
  channel->id            = xchannel->id;
  channel->pc            = xchannel->pc;
  channel->not_private   = xchannel->not_private;
  channel->dir_channel   = xchannel->dir_channel;
  channel->adapter       = xchannel->adapter;

  // in+out
  {
    p_tbx_darray_t     s_in_darray  = NULL;
    p_tbx_darray_t     s_out_darray = NULL;
    p_tbx_darray_t     in_darray    = NULL;
    p_tbx_darray_t     out_darray   = NULL;
    tbx_darray_index_t idx          =    0;
    p_mad_connection_t src          = NULL;


    s_in_darray  = xchannel->in_connection_darray;
    s_out_darray = xchannel->out_connection_darray;

    in_darray  = tbx_darray_init();
    out_darray = tbx_darray_init();

    if ((src = tbx_darray_first_idx(s_in_darray, &idx)))
      {
	do
	  {
	    p_mad_connection_t dst = NULL;

	    dst = mad_connection_cons();
	    tbx_darray_expand_and_set(in_darray, idx, dst);
	  }
	while ((src = tbx_darray_next_idx(s_in_darray, &idx)));
      }

    if ((src = tbx_darray_first_idx(s_out_darray, &idx)))
      {
	do
	  {
	    p_mad_connection_t dst = NULL;

	    dst = mad_connection_cons();
	    tbx_darray_expand_and_set(out_darray, idx, dst);
	  }
	while ((src = tbx_darray_next_idx(s_out_darray, &idx)));
      }

    if ((src = tbx_darray_first_idx(s_in_darray, &idx)))
      {
	do
	  {
	    p_mad_connection_t dst = NULL;

	    dst = tbx_darray_get(in_darray, idx);
	    dst->nature                = src->nature;
	    dst->remote_rank           = src->remote_rank;
	    dst->channel               = channel;
	    dst->reverse               =
	      tbx_darray_expand_and_get(out_darray, idx);
	    dst->way                   = src->way;
	    dst->connected             = src->connected;
	    dst->regular               = src->regular;
	    dst->parameter             = src->parameter;
	    dst->mtu                   = src->mtu;
	    dst->nb_link               = src->nb_link;
	    dst->specific              = src->specific;
	    dst->link_array            =
	      TBX_CALLOC(dst->nb_link, sizeof(p_mad_link_t));

	    {
	      mad_link_id_t link_id = -1;

	      for (link_id = 0; link_id < dst->nb_link; link_id++)
		{
		  p_mad_link_t src_lnk = NULL;
		  p_mad_link_t dst_lnk = NULL;

		  src_lnk = src->link_array[link_id];
		  dst_lnk = mad_link_cons();

		  dst_lnk->connection  = dst;
		  dst_lnk->id          = src_lnk->id;
		  dst_lnk->link_mode   = src_lnk->link_mode;
		  dst_lnk->buffer_mode = src_lnk->buffer_mode;
		  dst_lnk->group_mode  = src_lnk->group_mode;
		  dst_lnk->specific    = src_lnk->specific;

		  dst->link_array[link_id] = dst_lnk;
		}
	    }
	  }
	while ((src = tbx_darray_next_idx(s_in_darray, &idx)));
      }

    if ((src = tbx_darray_first_idx(s_out_darray, &idx)))
      {
	do
	  {
	    p_mad_connection_t dst = NULL;

	    dst = tbx_darray_get(out_darray, idx);
	    dst->nature                = src->nature;
	    dst->remote_rank           = src->remote_rank;
	    dst->channel               = channel;
	    dst->reverse               =
	      tbx_darray_expand_and_get(in_darray, idx);
	    dst->way                   = src->way;
	    dst->connected             = src->connected;
	    dst->regular               = src->regular;
	    dst->parameter             = src->parameter;
	    dst->mtu                   = src->mtu;
	    dst->nb_link               = src->nb_link;
	    dst->specific              = src->specific;
	    dst->link_array            =
	      TBX_CALLOC(dst->nb_link, sizeof(p_mad_link_t));

	    {
	      mad_link_id_t link_id = -1;

	      for (link_id = 0; link_id < dst->nb_link; link_id++)
		{
		  p_mad_link_t src_lnk = NULL;
		  p_mad_link_t dst_lnk = NULL;

		  src_lnk = src->link_array[link_id];
		  dst_lnk = mad_link_cons();

		  dst_lnk->connection  = dst;
		  dst_lnk->id          = src_lnk->id;
		  dst_lnk->link_mode   = src_lnk->link_mode;
		  dst_lnk->buffer_mode = src_lnk->buffer_mode;
		  dst_lnk->group_mode  = src_lnk->group_mode;
		  dst_lnk->specific    = src_lnk->specific;

		  dst->link_array[link_id] = dst_lnk;
		}
	    }
	  }
	while ((src = tbx_darray_next_idx(s_out_darray, &idx)));
      }


    channel->in_connection_darray  = in_darray;
    channel->out_connection_darray = out_darray;
  }

  channel->parameter = xchannel->parameter;

  channel->max_sub            = xchannel->max_sub;
  channel->max_mux            = xchannel->max_mux;
  channel->mux_list_darray    = xchannel->mux_list_darray;
  channel->mux_channel_darray = xchannel->mux_channel_darray;
  channel->channel_slist      = xchannel->channel_slist;
  channel->specific           = xchannel->specific;
  LOG_OUT();

  return channel;
}

p_mad_channel_t
mad_mux_get_sub_channel(p_mad_channel_t xchannel,
			unsigned int    sub)
{
  p_mad_channel_t channel = NULL;

  LOG_IN();
  channel = __mad_mux_generate_sub_channel_skel(xchannel);

  {
    p_tbx_string_t s = NULL;

    s = tbx_string_init_to_cstring(xchannel->name);
    tbx_string_append_int(s, sub);
    channel->name = tbx_string_to_cstring_and_free(s);
  }

  channel->sub_list_darray    = xchannel->sub_list_darray;
  channel->sub                = sub;
  channel->mux                = xchannel->mux;
  channel->sub_channel_darray = xchannel->sub_channel_darray;
  LOG_OUT();

  return channel;
}

void
mad_mux_add_named_sub_channels(p_mad_channel_t xchannel)
{
  p_mad_madeleine_t   madeleine    = NULL;
  p_mad_adapter_t     mux_adapter  = NULL;
  p_mad_dir_channel_t dir_xchannel = NULL;
  unsigned int        mux          =    0;
  p_tbx_darray_t      darray       = NULL;
  p_tbx_slist_t       slist        = NULL;

  LOG_IN();
  mux_adapter  = xchannel->adapter;
  madeleine    = mux_adapter->driver->madeleine;
  dir_xchannel = xchannel->dir_channel;
  slist        = dir_xchannel->sub_channel_name_slist;
  darray       = xchannel->mux_channel_darray;

  if (!tbx_slist_is_nil(slist))
    {
      tbx_slist_ref_to_head(slist);

      do
	{
	  p_mad_channel_t  channel = NULL;
	  char            *name    = NULL;

	  name = tbx_slist_ref_get(slist);
	  mux++;
	  if (mux >= xchannel->max_mux)
	    TBX_FAILURE("not enough resources to  allocate named sub channel");

	  channel = __mad_mux_generate_sub_channel_skel(xchannel);

	  channel->name               = tbx_strdup(name);
	  channel->sub                = 0;
	  channel->mux                = mux;
	  channel->sub_list_darray    = tbx_darray_init();
	  tbx_darray_expand_and_set(xchannel->mux_list_darray, mux,
				    channel->sub_list_darray);
	  channel->sub_channel_darray = tbx_darray_init();
	  tbx_darray_expand_and_set(channel->sub_channel_darray, 0, channel);

	  tbx_htable_add(mux_adapter->channel_htable, channel->name, channel);
	  tbx_htable_add(madeleine->channel_htable,   channel->name, channel);

	  if (channel->not_private)
	    {
	      tbx_slist_append(madeleine->public_channel_slist, channel->name);
	    }

	  tbx_darray_expand_and_set(darray, mux, channel);
	}
      while (tbx_slist_ref_forward(slist));
    }
  LOG_OUT();
}

#endif // MARCEL
