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
 * Mad_forward.c
 * =============
 */

#include "madeleine.h"
#include "tbx.h"

/*#define ACCUMULATION 5*/

#ifdef MARCEL
/*
 * Macros
 * ------
 */
#define INITIAL_FBHEADER_NUM 64
#define INITIAL_MQENTRY_NUM  256

/*
 * Static variables
 * ----------------
 */
static p_tbx_memory_t mad_fbheader_memory = NULL;
static p_tbx_memory_t mad_mqentry_memory  = NULL;

/*
 * Functions
 * ---------
 */

void
mad_forward_memory_manager_init(int    argc TBX_UNUSED,
				char **argv TBX_UNUSED)
{
  LOG_IN();
  tbx_malloc_init(&mad_fbheader_memory,
		  sizeof(mad_fblock_header_t),
		  INITIAL_FBHEADER_NUM);
  tbx_malloc_init(&mad_mqentry_memory,
		  sizeof(mad_fmessage_queue_entry_t),
		  INITIAL_MQENTRY_NUM);
  LOG_OUT();
}

void
mad_forward_memory_manager_exit(void)
{
  LOG_IN();
  tbx_malloc_clean(mad_fbheader_memory);
  tbx_malloc_clean(mad_mqentry_memory);
  LOG_OUT();
}

static
void
mad_forward_write_block_header(p_mad_connection_t    out,
			       p_mad_fblock_header_t fbh)
{
  p_mad_buffer_t           fbh_buffer = NULL;
  p_mad_driver_interface_t interface  = NULL;
  p_mad_link_t             lnk        = NULL;
  p_mad_link_t             data_lnk   = NULL;

  LOG_IN();
  interface  = out->channel->adapter->driver->interface;
  fbh_buffer = mad_get_user_send_buffer(fbh->data,
					mad_fblock_fsize);
  if (interface->choice)
    {
      lnk = interface->choice(out, fbh_buffer->length,
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
      mad_copy_buffer(fbh_buffer, static_buffer);
      interface->send_buffer(lnk, static_buffer);
    }
  else if (lnk->buffer_mode == mad_buffer_mode_dynamic)
    {
      interface->send_buffer(lnk, fbh_buffer);
    }
  else
    TBX_FAILURE("invalid link mode");

  if (fbh->is_a_group || fbh->is_an_eof_msg
#ifdef MAD_FORWARD_FLOW_CONTROL
      || fbh->is_an_ack
      || fbh->is_a_new_msg
#endif // MAD_FORWARD_FLOW_CONTROL
      )
    goto end;

  if (interface->choice)
    {
      data_lnk = interface->choice(out, fbh->length,
				   mad_send_SAFER,
				   mad_receive_EXPRESS);
    }
  else
    {
      data_lnk = out->link_array[0];
    }

  if (data_lnk->buffer_mode == mad_buffer_mode_static)
    {
      if (fbh->type == mad_fblock_type_static_src)
	{
	  p_mad_buffer_t static_buffer = NULL;

	  static_buffer = interface->get_static_buffer(data_lnk);
	  mad_copy_buffer(fbh->block, static_buffer);
	  interface->send_buffer(data_lnk, static_buffer);

	  fbh->src_interface->
	    return_static_buffer(fbh->src_link, fbh->block);
	}
      else if (fbh->type == mad_fblock_type_static_dst)
	{
	  interface->send_buffer(data_lnk, fbh->block);
	}
      else
	TBX_FAILURE("invalid block type");
    }
  else if (data_lnk->buffer_mode == mad_buffer_mode_dynamic)
    {
      interface->send_buffer(data_lnk, fbh->block);

      if (fbh->type == mad_fblock_type_static_src)
	{
	  fbh->src_interface->
	    return_static_buffer(fbh->src_link, fbh->block);
	}
      else if (fbh->type == mad_fblock_type_dynamic)
	{
	  mad_free_buffer(fbh->block);
	}
      else
	TBX_FAILURE("invalid block type");
    }
  else
    TBX_FAILURE("invalid link mode");

 end:
  mad_free_buffer_struct(fbh_buffer);
  tbx_free(mad_fbheader_memory, fbh);
  LOG_OUT();
}

static
void *
mad_forward_direct_reemit_block(void *arg)
{
  p_mad_forward_reemit_block_arg_t  rarg               = NULL;
  p_mad_connection_t                vout               = NULL;
  p_mad_connection_t                out                = NULL;
  p_mad_channel_t                   vchannel           = NULL;
  p_mad_driver_interface_t          interface          = NULL;
  marcel_sem_t                     *message_to_forward = NULL;
  p_tbx_slist_t                     message_list       = NULL;
#ifdef ACCUMULATION
 int m = ACCUMULATION;
#endif
  LOG_IN();
  rarg               = arg;
  vout               = rarg->connection;
  out                = vout->regular;
  vchannel           = rarg->vchannel;
  TBX_FREE(arg);
  arg                = NULL;
  rarg               = NULL;
  interface          = out->channel->adapter->driver->interface;
  message_to_forward = &(out->something_to_forward);
  message_list       = out->message_queue;

  for (;;)
    {
      p_mad_forward_block_queue_t   block_queue      = NULL;
      p_tbx_slist_t                 block_slist      = NULL;
      p_mad_fmessage_queue_entry_t  entry            = NULL;
      marcel_sem_t                 *block_to_forward = NULL;
      ntbx_process_grank_t          source           =   -1;

      marcel_sem_P(message_to_forward);

      if (out->closing)
	break;

      marcel_mutex_lock(&(vout->lock_mutex));
      marcel_mutex_lock(&(out->lock_mutex));

      TBX_LOCK_SHARED(message_list);
      entry = tbx_slist_extract(message_list);
      TBX_UNLOCK_SHARED(message_list);

      source = entry->source;

      tbx_free(mad_mqentry_memory, entry);
      entry = NULL;

      if (interface->new_message)
	interface->new_message(out);

      block_queue      = tbx_darray_get(out->block_queues, source);
      block_to_forward = &(block_queue->block_to_forward);
      block_slist      = block_queue->queue;

      for (;;)
	{
	  p_mad_fblock_header_t fbh = NULL;

	  marcel_sem_P(block_to_forward);
	  TBX_LOCK_SHARED(block_slist);
#ifdef ACCUMULATION
	  {
	    int l = 0;

	    l = tbx_slist_get_length(block_slist);

	    if (l > m)
	      {
		m = l;
		LDISP_VAL("stored blocks", m);
	      }
	    else if (l <= ACCUMULATION)
	      {
		m = ACCUMULATION;
	      }
	  }
#endif // ACCUMULATION
	  fbh = tbx_slist_extract(block_slist);
	  TBX_UNLOCK_SHARED(block_slist);

	  if (fbh->is_an_eof_msg)
	    {
	      tbx_free(mad_fbheader_memory, fbh);
	      break;
	    }

	  if (fbh->is_a_group)
	    {
	      unsigned int group_len = 0;

	      group_len = fbh->length;
	      mad_forward_write_block_header(out, fbh);
	      fbh = NULL;

	      while (group_len--)
		{
		  marcel_sem_P(block_to_forward);
		  TBX_LOCK_SHARED(block_slist);
		  fbh = tbx_slist_extract(block_slist);
		  TBX_UNLOCK_SHARED(block_slist);

		  mad_forward_write_block_header(out, fbh);
		  fbh = NULL;
		}
	    }
	  else
	    {
	      mad_forward_write_block_header(out, fbh);
	    }
	}

      if (interface->finalize_message)
	interface->finalize_message(out);

      
      marcel_mutex_unlock(&(out->lock_mutex));
      marcel_mutex_unlock(&(vout->lock_mutex));
    }
  LOG_OUT();

  return NULL;
}

static
void *
mad_forward_indirect_reemit_block(void *arg)
{
  p_mad_forward_reemit_block_arg_t  rarg                 = NULL;
  p_mad_connection_t                out                  = NULL;
  p_mad_channel_t                   vchannel             = NULL;
  p_mad_driver_interface_t          interface            = NULL;
  marcel_sem_t                     *something_to_forward = NULL;
  p_tbx_slist_t                     block_slist          = NULL;

  LOG_IN();
  rarg                 = arg;
  out                  = rarg->connection;
  vchannel             = rarg->vchannel;
  TBX_FREE(arg);
  arg                  = NULL;
  rarg                 = NULL;
  interface            = out->channel->adapter->driver->interface;
  something_to_forward = &(out->something_to_forward);
  block_slist          = out->forwarding_block_list;

  for (;;)
    {
      p_mad_fblock_header_t fbh = NULL;

      marcel_sem_P(something_to_forward);

      if (out->closing)
	break;

      TBX_LOCK_SHARED(block_slist);
      fbh = tbx_slist_extract(block_slist);
      TBX_UNLOCK_SHARED(block_slist);

      marcel_mutex_lock(&(out->lock_mutex));
      if (interface->new_message)
	interface->new_message(out);

      if (fbh->is_a_group)
	{
	  unsigned int group_len = 0;

	  group_len = fbh->length;
	  mad_forward_write_block_header(out, fbh);
	  fbh = NULL;

	  while (group_len--)
	    {
	      marcel_sem_P(something_to_forward);

	      TBX_LOCK_SHARED(block_slist);
	      fbh = tbx_slist_extract(block_slist);
	      TBX_UNLOCK_SHARED(block_slist);

	      mad_forward_write_block_header(out, fbh);
	      fbh = NULL;
	    }
	}
      else
	{
	  mad_forward_write_block_header(out, fbh);
	}

      if (interface->finalize_message)
	interface->finalize_message(out);

      marcel_mutex_unlock(&(out->lock_mutex));
    }
  LOG_OUT();

  return NULL;
}

static
p_mad_fblock_header_t
mad_forward_read_block_header(p_mad_channel_t    mad_vchannel,
			      p_mad_connection_t in)
{
  p_mad_fblock_header_t     fbh        = NULL;
  p_mad_buffer_t            fbh_buffer = NULL;
  p_mad_driver_interface_t  interface  = NULL;
  p_mad_link_t              lnk        = NULL;
  char                     *data       = NULL;
  tbx_bool_t                no_data    = tbx_false;

  LOG_IN();
  interface  = in->channel->adapter->driver->interface;
  fbh        = tbx_malloc(mad_fbheader_memory);
  fbh_buffer = mad_get_user_receive_buffer(fbh->data,
					   mad_fblock_fsize);

  if (interface->choice)
    {
      lnk = interface->choice(in, fbh_buffer->length,
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
      mad_copy_buffer(static_buffer, fbh_buffer);
      interface->return_static_buffer(lnk, static_buffer);
    }
  else if (lnk->buffer_mode == mad_buffer_mode_dynamic)
    {
      interface->receive_buffer(lnk, &fbh_buffer);
    }
  else
    TBX_FAILURE("invalid link mode");

  data = fbh->data;
  fbh->length        =
      tbx_expand_field(data, mad_fblock_flength_0)
    | tbx_expand_field(data, mad_fblock_flength_1)
    | tbx_expand_field(data, mad_fblock_flength_2)
    | tbx_expand_field(data, mad_fblock_flength_3);
  fbh->source        =
      tbx_expand_field(data, mad_fblock_fsource_0)
    | tbx_expand_field(data, mad_fblock_fsource_1)
    | tbx_expand_field(data, mad_fblock_fsource_2)
    | tbx_expand_field(data, mad_fblock_fsource_3);
  fbh->destination   =
      tbx_expand_field(data, mad_fblock_fdestination_0)
    | tbx_expand_field(data, mad_fblock_fdestination_1)
    | tbx_expand_field(data, mad_fblock_fdestination_2)
    | tbx_expand_field(data, mad_fblock_fdestination_3);
  fbh->is_a_group    =
    tbx_expand_field(data, mad_fblock_fis_a_group);
  fbh->is_a_new_msg  =
    tbx_expand_field(data, mad_fblock_fis_a_new_msg);
  fbh->is_an_eof_msg =
    tbx_expand_field(data, mad_fblock_fis_an_eof_msg);
  fbh->closing       =
    tbx_expand_field(data, mad_fblock_fclosing);
#ifdef MAD_FORWARD_FLOW_CONTROL
  fbh->is_an_ack     =
    tbx_expand_field(data, mad_fblock_fis_an_ack);
#endif // MAD_FORWARD_FLOW_CONTROL
  fbh->src_interface = interface;
  fbh->vout          =
    tbx_darray_get(mad_vchannel->out_connection_darray,
		   fbh->destination);

  no_data =
    fbh->is_a_group
    || fbh->is_an_eof_msg
#ifdef MAD_FORWARD_FLOW_CONTROL
    || fbh->is_an_ack
    || fbh->is_a_new_msg
#endif // MAD_FORWARD_FLOW_CONTROL
    || fbh->closing;

  if (no_data)
    {
      fbh->type     = mad_fblock_type_empty;
      fbh->block    = NULL;
      fbh->src_link = NULL;
    }
  else
    {
      p_mad_link_t data_lnk = NULL;

      if (interface->choice)
	{
	  data_lnk = interface->choice(in, fbh->length,
				       mad_send_SAFER,
				       mad_receive_EXPRESS);
	}
      else
	{
	  data_lnk = in->link_array[0];
	}

      fbh->src_link = data_lnk;

      if (data_lnk->buffer_mode == mad_buffer_mode_static)
	{
	  interface->receive_buffer(data_lnk, &(fbh->block));
	  fbh->type = mad_fblock_type_static_src;
	}
      else if (data_lnk->buffer_mode == mad_buffer_mode_dynamic)
	{
	  p_mad_connection_t       vout           = NULL;
	  p_mad_connection_t       out            = NULL;
	  p_mad_driver_interface_t next_interface = NULL;
	  p_mad_link_t             next_data_lnk  = NULL;

	  vout           = fbh->vout;
	  out            = vout->regular;
	  next_interface =
	    out->channel->adapter->driver->interface;

	  if (next_interface->choice)
	    {
	      next_data_lnk =
		next_interface->choice(out, fbh->length,
				       mad_send_SAFER,
				       mad_receive_EXPRESS);
	    }
	  else
	    {
	      next_data_lnk = out->link_array[0];
	    }


	  if (next_data_lnk->buffer_mode == mad_buffer_mode_static)
	    {
	      fbh->block =
		next_interface->get_static_buffer(next_data_lnk);

	      fbh->type  = mad_fblock_type_static_dst;
	    }
	  else if (next_data_lnk->buffer_mode ==
		   mad_buffer_mode_dynamic)
	    {
	      fbh->block = mad_alloc_buffer(fbh->length);
	      fbh->type  = mad_fblock_type_dynamic;
	    }
	  else
	    TBX_FAILURE("invalid link mode");

	  if (fbh->block->length > fbh->length)
	    {
	      fbh->block->length = fbh->length;
	    }

	  interface->receive_buffer(data_lnk, &(fbh->block));
	}
      else
	TBX_FAILURE("invalid link mode");
    }

  mad_free_buffer_struct(fbh_buffer);
  LOG_OUT();

  return fbh;
}

static
void *
mad_forward_receive_block(void *arg)
{
  p_mad_forward_receive_block_arg_t farg         = NULL;
  p_mad_channel_t                   mad_channel  = NULL;
  p_mad_channel_t                   mad_vchannel = NULL;
  p_mad_driver_interface_t          interface    = NULL;

  LOG_IN();
  farg         = arg;
  mad_channel  = farg->channel;
  mad_vchannel = farg->vchannel;
  TBX_FREE(arg);
  arg          = NULL;
  farg         = NULL;
  interface    = mad_channel->adapter->driver->interface;

  for (;;)
    {
      p_mad_connection_t     in                   = NULL;
      p_mad_connection_t     vout                 = NULL;
      p_mad_connection_t     out                  = NULL;
      p_mad_fblock_header_t  fbh                  = NULL;
      p_mad_forward_block_queue_t block_queue     = NULL;
      p_tbx_slist_t          block_slist          = NULL;
      marcel_sem_t          *block_to_forward     = NULL;

      in  = interface->receive_message(mad_channel);
      fbh = mad_forward_read_block_header(mad_vchannel, in);

      if (fbh->closing)
	{
	  if (interface->message_received)
	    interface->message_received(in);

	  tbx_free(mad_fbheader_memory, fbh);
	  fbh = NULL;
	  break;
	}

#ifdef MAD_FORWARD_FLOW_CONTROL
      if (fbh->is_an_ack && (fbh->destination == mad_vchannel->process_lrank))
	{
	  p_mad_connection_t ack_vout = NULL;

	  if (interface->message_received)
	    interface->message_received(in);

	  ack_vout = tbx_darray_get(mad_vchannel->out_connection_darray,
				    fbh->source);

	  marcel_sem_V(&ack_vout->ack);

	  tbx_free(mad_fbheader_memory, fbh);
	  fbh = NULL;
	  continue;
	}
#endif // MAD_FORWARD_FLOW_CONTROL

      vout = fbh->vout;

#ifdef MAD_FORWARD_FLOW_CONTROL
      if (fbh->is_an_ack)
	{
	  p_mad_channel_t     fchannel    = NULL;
	  p_mad_dir_channel_t dir_channel = NULL;

	  dir_channel = vout->regular->channel->dir_channel;

	  {
	    char name [2 + strlen(dir_channel->name)];

	    name[0] = 'f';
	    strcpy(name + 1, dir_channel->name);

	    fchannel =
	      tbx_htable_get(vout->regular->channel->adapter->
			     channel_htable, name);
	  }

	  out = tbx_darray_get(fchannel->out_connection_darray,
			       vout->regular->remote_rank);
	}
      else
#endif // MAD_FORWARD_FLOW_CONTROL
	out = vout->regular;

      TBX_LOCK_SHARED(out);
      if ((vout->nature == mad_connection_nature_direct_virtual)
#ifdef MAD_FORWARD_FLOW_CONTROL
	  && (!fbh->is_an_ack)
#endif // MAD_FORWARD_FLOW_CONTROL
	  )
	{
	  p_tbx_slist_t        message_slist = NULL;
	  ntbx_process_grank_t source        =   -1;

	  if (!out->message_queue)
	    {
	      p_mad_forward_reemit_block_arg_t  rarg = NULL;

	      out->message_queue = tbx_slist_nil();
	      out->block_queues  = tbx_darray_init();
	      marcel_sem_init(&(out->something_to_forward), 0);

	      rarg =
		TBX_CALLOC(1,
			   sizeof(mad_forward_reemit_block_arg_t));

	      rarg->connection = vout;
	      rarg->vchannel   = mad_vchannel;

	      marcel_create(&(out->forwarding_thread),
			    NULL, mad_forward_direct_reemit_block,
			    (any_t)rarg);
	      rarg = NULL;
	    }

	  message_slist = out->message_queue;
	  source        = fbh->source;
	  block_queue   = tbx_darray_get(out->block_queues, source);

	  if (fbh->is_a_new_msg)
	    {
	      p_mad_fmessage_queue_entry_t entry = NULL;

	      entry = tbx_malloc(mad_mqentry_memory);
	      entry->source = source;

	      TBX_LOCK_SHARED(message_slist);
	      tbx_slist_append(message_slist, entry);
	      TBX_UNLOCK_SHARED(message_slist);

	      if (!block_queue)
		{
		  block_queue =
		    TBX_CALLOC(1, sizeof(mad_forward_block_queue_t));

		  block_to_forward = &(block_queue->block_to_forward);
		  marcel_sem_init(block_to_forward, 0);

		  block_slist = tbx_slist_nil();
		  block_queue->queue = block_slist;

		  tbx_darray_expand_and_set(out->block_queues,
					    source, block_queue);
		}
	      else
		{
		  block_slist = block_queue->queue;
		  block_to_forward = &(block_queue->block_to_forward);
		}

	      marcel_sem_V(&(out->something_to_forward));
	    }
	  else
	    {
	      block_slist = block_queue->queue;
	      block_to_forward = &(block_queue->block_to_forward);
	    }
	}
      else if (vout->nature == mad_connection_nature_indirect_virtual
#ifdef MAD_FORWARD_FLOW_CONTROL
	       || (fbh->is_an_ack)
#endif // MAD_FORWARD_FLOW_CONTROL
	       )
	{
	  if (!out->forwarding_block_list)
	    {
	      p_mad_forward_reemit_block_arg_t rarg = NULL;

	      out->forwarding_block_list = tbx_slist_nil();
	      marcel_sem_init(&(out->something_to_forward), 0);

	      rarg =
		TBX_CALLOC(1,
			   sizeof(mad_forward_reemit_block_arg_t));

	      rarg->connection = out;
	      rarg->vchannel   = mad_vchannel;

	      marcel_create(&(out->forwarding_thread),
			    NULL, mad_forward_indirect_reemit_block,
			    (any_t)rarg);
	      rarg = NULL;
	    }

	  block_slist      = out->forwarding_block_list;
	  block_to_forward = &(out->something_to_forward);
	}
      else
	TBX_FAILURE("invalid connection nature");

      if (fbh->is_a_group)
	{
	  unsigned int group_len = 0;

	  group_len = fbh->length;

	  TBX_LOCK_SHARED(block_slist);
	  tbx_slist_append(block_slist, fbh);
	  marcel_sem_V(block_to_forward);
	  TBX_UNLOCK_SHARED(block_slist);
	  fbh = NULL;

	  while (group_len--)
	    {
	      fbh = mad_forward_read_block_header(mad_vchannel, in);
	      TBX_LOCK_SHARED(block_slist);
	      tbx_slist_append(block_slist, fbh);
	      marcel_sem_V(block_to_forward);
	      TBX_UNLOCK_SHARED(block_slist);
	      fbh = NULL;
	    }
	}
      else
	{
	  TBX_LOCK_SHARED(block_slist);
	  tbx_slist_append(block_slist, fbh);
	  marcel_sem_V(block_to_forward);
	  TBX_UNLOCK_SHARED(block_slist);
	  fbh = NULL;
	}

      TBX_UNLOCK_SHARED(out);

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
mad_forward_fill_fbh_data(unsigned char *data,
			  unsigned int   src,
			  unsigned int   dst,
			  unsigned int   length,
			  tbx_bool_t     is_a_group,
			  tbx_bool_t     is_a_new_msg,
			  tbx_bool_t     is_an_eof_msg,
			  tbx_bool_t     is_direct,
			  tbx_bool_t     closing
#ifdef MAD_FORWARD_FLOW_CONTROL
			  , tbx_bool_t     is_an_ack
#endif // MAD_FORWARD_FLOW_CONTROL
			  )
{
  unsigned int uint_is_a_group    = 0;
  unsigned int uint_is_a_new_msg  = 0;
  unsigned int uint_is_an_eof_msg = 0;
  unsigned int uint_is_direct     = 0;
  unsigned int uint_closing       = 0;
#ifdef MAD_FORWARD_FLOW_CONTROL
  unsigned int uint_is_an_ack     = 0;
#endif // MAD_FORWARD_FLOW_CONTROL

  LOG_IN();
  uint_is_a_group    =    (is_a_group)?1:0;
  uint_is_a_new_msg  =  (is_a_new_msg)?1:0;
  uint_is_an_eof_msg = (is_an_eof_msg)?1:0;
  uint_is_direct     =     (is_direct)?1:0;
  uint_closing       =       (closing)?1:0;
#ifdef MAD_FORWARD_FLOW_CONTROL
  uint_is_an_ack     =     (is_an_ack)?1:0;
#endif // MAD_FORWARD_FLOW_CONTROL

  tbx_contract_field(data, length, mad_fblock_flength_0);
  tbx_contract_field(data, length, mad_fblock_flength_1);
  tbx_contract_field(data, length, mad_fblock_flength_2);
  tbx_contract_field(data, length, mad_fblock_flength_3);

  tbx_contract_field(data, src, mad_fblock_fsource_0);
  tbx_contract_field(data, src, mad_fblock_fsource_1);
  tbx_contract_field(data, src, mad_fblock_fsource_2);
  tbx_contract_field(data, src, mad_fblock_fsource_3);

  tbx_contract_field(data, dst, mad_fblock_fdestination_0);
  tbx_contract_field(data, dst, mad_fblock_fdestination_1);
  tbx_contract_field(data, dst, mad_fblock_fdestination_2);
  tbx_contract_field(data, dst, mad_fblock_fdestination_3);

  tbx_contract_field(data, uint_is_a_group,    mad_fblock_fis_a_group);
  tbx_contract_field(data, uint_is_a_new_msg,  mad_fblock_fis_a_new_msg);
  tbx_contract_field(data, uint_is_an_eof_msg, mad_fblock_fis_an_eof_msg);
  tbx_contract_field(data, uint_is_direct,     mad_fblock_fis_direct);
  tbx_contract_field(data, uint_closing,       mad_fblock_fclosing);

#ifdef MAD_FORWARD_FLOW_CONTROL
  tbx_contract_field(data, uint_is_an_ack, mad_fblock_fis_an_ack);
#endif // MAD_FORWARD_FLOW_CONTROL
  LOG_OUT();
}

static
void
mad_forward_send_bytes(p_mad_connection_t  out,
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
mad_forward_receive_bytes(p_mad_connection_t  in,
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

static
void *
mad_forward_poll_channel(void *arg)
{
  p_mad_forward_poll_channel_arg_t  parg             = NULL;
  p_mad_channel_t                   channel          = NULL;
  p_mad_channel_t                   vchannel         = NULL;
  p_mad_driver_interface_t          interface        = NULL;
  p_tbx_darray_t                    in_darray        = NULL;
  marcel_sem_t                     *ready_to_receive = NULL;
  marcel_sem_t                     *message_ready    = NULL;
  marcel_sem_t                     *message_received = NULL;

  LOG_IN();
  parg      = arg;
  channel   = parg->channel;
  vchannel  = parg->vchannel;
  in_darray = vchannel->in_connection_darray;
  interface = channel->adapter->driver->interface;

  ready_to_receive = &(vchannel->ready_to_receive);
  message_ready    = &(vchannel->message_ready);
  message_received = &(vchannel->message_received);

  for (;;)
    {
      p_mad_connection_t in        = NULL;
      unsigned int       is_direct =    0;
      unsigned int       closing   =    0;
      unsigned char      data[mad_fblock_fsize] = {0};

      in = interface->receive_message(channel);
      mad_forward_receive_bytes(in, data, mad_fblock_fsize);

      closing   = tbx_expand_field(data, mad_fblock_fclosing);
      if (closing)
	{
	  if (interface->message_received)
	    interface->message_received(in);

	  break;
	}

      is_direct = tbx_expand_field(data, mad_fblock_fis_direct);

      marcel_sem_P(ready_to_receive);

      if (is_direct)
	{
	  p_mad_connection_t vin = NULL;
	  unsigned int       src = 0;

	  src =
	    tbx_expand_field(data, mad_fblock_fsource_0)
	    | tbx_expand_field(data, mad_fblock_fsource_1)
	    | tbx_expand_field(data, mad_fblock_fsource_2)
	    | tbx_expand_field(data, mad_fblock_fsource_3);

	  vin = tbx_darray_get(in_darray, src);
	  vchannel->active_connection = vin;
	}
      else
	{
	  p_mad_connection_t vin = NULL;
	  unsigned int       src = 0;

	  src =
	    tbx_expand_field(data, mad_fblock_fsource_0)
	    | tbx_expand_field(data, mad_fblock_fsource_1)
	    | tbx_expand_field(data, mad_fblock_fsource_2)
	    | tbx_expand_field(data, mad_fblock_fsource_3);

	  vin = tbx_darray_get(in_darray, src);
	  vin->first_block_length     =
	    tbx_expand_field(data, mad_fblock_flength_0)
	    | tbx_expand_field(data, mad_fblock_flength_1)
	    | tbx_expand_field(data, mad_fblock_flength_2)
	    | tbx_expand_field(data, mad_fblock_flength_3);
	  vin->first_block_is_a_group =
	    tbx_expand_field(data, mad_fblock_fis_a_group);
	  vin->new_msg                = tbx_true;

	  vchannel->active_connection = vin;
	}

      marcel_sem_V(message_ready);
      marcel_sem_P(message_received);
    }
  LOG_OUT();

  return NULL;
}

void
mad_forward_register(p_mad_driver_t driver)
{
  p_mad_driver_interface_t interface;

  LOG_IN();
  TRACE("Registering FORWARD driver");
  interface = driver->interface;

  driver->connection_type  = mad_unidirectional_connection;
  driver->buffer_alignment = 64;
  driver->name             = tbx_strdup("forward");

  interface->driver_init                =
    mad_forward_driver_init;
  interface->adapter_init               = NULL;
  interface->channel_init               =
    mad_forward_channel_init;
  interface->before_open_channel        =
    mad_forward_before_open_channel;
  interface->connection_init            =
    mad_forward_connection_init;
  interface->link_init                  =
    mad_forward_link_init;
  interface->accept                     = NULL;
  interface->connect                    = NULL;
  interface->after_open_channel         =
    mad_forward_after_open_channel;
  interface->before_close_channel       =
    mad_forward_before_close_channel;
  interface->disconnect                 =
    mad_forward_disconnect;
  interface->after_close_channel        =
    mad_forward_after_close_channel;
  interface->link_exit                  =
    mad_forward_link_exit;
  interface->connection_exit            =
    mad_forward_connection_exit;
  interface->channel_exit               =
    mad_forward_channel_exit;
  interface->adapter_exit               = NULL;
  interface->driver_exit                = NULL;
  interface->choice                     =
    mad_forward_choice;
  interface->get_static_buffer          =
    mad_forward_get_static_buffer;
  interface->return_static_buffer       =
    mad_forward_return_static_buffer;
  interface->new_message                =
    mad_forward_new_message;
  interface->finalize_message           =
    mad_forward_finalize_message;
  interface->message_received           =
    mad_forward_message_received;
#ifdef MAD_MESSAGE_POLLING
  interface->poll_message               =
    mad_forward_poll_message;
#endif // MAD_MESSAGE_POLLING
  interface->receive_message            =
    mad_forward_receive_message;
  interface->send_buffer                =
    mad_forward_send_buffer;
  interface->receive_buffer             =
    mad_forward_receive_buffer;
  interface->send_buffer_group          =
    mad_forward_send_buffer_group;
  interface->receive_sub_buffer_group   =
    mad_forward_receive_sub_buffer_group;
  LOG_OUT();
}

void
mad_forward_driver_init(p_mad_driver_t driver)
{
  p_mad_adapter_t dummy_adapter = NULL;

  LOG_IN();
  dummy_adapter                 = mad_adapter_cons();
  dummy_adapter->driver         = driver;
  dummy_adapter->id             = -1;
  dummy_adapter->selector       = tbx_strdup("-");
  dummy_adapter->parameter      = tbx_strdup("-");
  dummy_adapter->channel_htable = tbx_htable_empty_table();

  tbx_htable_add(driver->adapter_htable, "forward",
		 dummy_adapter);
  LOG_OUT();
}


void
mad_forward_channel_init(p_mad_channel_t channel)
{
  LOG_IN();
  marcel_sem_init(&(channel->ready_to_receive), 0);
  marcel_sem_init(&(channel->message_ready), 0);
  marcel_sem_init(&(channel->message_received), 0);
  channel->can_receive        = tbx_false;
  channel->a_message_is_ready = tbx_false;
  channel->active_connection  = NULL;
  channel->specific           = NULL;
  LOG_OUT();
}

void
mad_forward_connection_init(p_mad_connection_t in,
			    p_mad_connection_t out)
{

  LOG_IN();
  if (in)
    {
#ifdef MAD_FORWARD_FLOW_CONTROL
      marcel_sem_init(&in->ack, 0);
#endif // MAD_FORWARD_FLOW_CONTROL

      in->specific = NULL;

      if (in->nature == mad_connection_nature_direct_virtual)
	{
	  in->nb_link = in->regular->nb_link;
	}
      else if (in->nature == mad_connection_nature_indirect_virtual)
	{
	  in->nb_link = 1;
	}
      else
	TBX_FAILURE("invalid connection nature");
    }

  if (out)
    {
#ifdef MAD_FORWARD_FLOW_CONTROL
      marcel_sem_init(&out->ack, 0);
#endif // MAD_FORWARD_FLOW_CONTROL

      out->specific = NULL;

      if (out->nature == mad_connection_nature_direct_virtual)
	{
	  out->nb_link = out->regular->nb_link;
	}
      else if (out->nature == mad_connection_nature_indirect_virtual)
	{
	  out->nb_link = 1;
	}
      else
	TBX_FAILURE("invalid connection nature");
    }
  LOG_OUT();
}

void
mad_forward_link_init(p_mad_link_t lnk)
{
  p_mad_connection_t connection = NULL;

  LOG_IN();
  connection = lnk->connection;

  if (connection->nature  == mad_connection_nature_direct_virtual)
    {
      p_mad_link_t regular_lnk = NULL;

      regular_lnk = connection->regular->link_array[lnk->id];

      lnk->link_mode   = regular_lnk->link_mode;
      lnk->buffer_mode = regular_lnk->buffer_mode;
      lnk->group_mode  = regular_lnk->group_mode;
    }
  else if (connection->nature ==
	   mad_connection_nature_indirect_virtual)
    {
      lnk->link_mode   = mad_link_mode_buffer;
      lnk->buffer_mode = mad_buffer_mode_dynamic;
      lnk->group_mode  = mad_group_mode_split;
    }
  else
    TBX_FAILURE("invalid connection nature");
  LOG_OUT();
}

void
mad_forward_before_open_channel(p_mad_channel_t channel)
{
  LOG_IN();
  /* nothing */
  LOG_OUT();
}

void
mad_forward_after_open_channel(p_mad_channel_t channel)
{
  p_tbx_slist_t slist = NULL;

  LOG_IN();
  slist = channel->channel_slist;

  tbx_slist_ref_to_head(slist);
  do
    {
      p_mad_channel_t                  regular_channel = NULL;
      p_mad_forward_poll_channel_arg_t parg            = NULL;

      regular_channel = tbx_slist_ref_get(slist);

      parg = TBX_CALLOC(1, sizeof(mad_forward_poll_channel_arg_t));
      parg->channel  = regular_channel;
      parg->vchannel = channel;

      marcel_create(&(regular_channel->polling_thread),
		    NULL, mad_forward_poll_channel,
		    (any_t)parg);
    }
  while (tbx_slist_ref_forward(slist));

  slist = channel->fchannel_slist;

  tbx_slist_ref_to_head(slist);
  do
    {
      p_mad_channel_t                   regular_channel = NULL;
      p_mad_forward_receive_block_arg_t farg            = NULL;

      regular_channel = tbx_slist_ref_get(slist);

      farg = TBX_CALLOC(1, sizeof(mad_forward_receive_block_arg_t));
      farg->channel  = regular_channel;
      farg->vchannel = channel;

      marcel_create(&(regular_channel->polling_thread),
		    NULL, mad_forward_receive_block,
		    (any_t)farg);
    }
  while (tbx_slist_ref_forward(slist));

  channel->specific = NULL;
  LOG_OUT();
}

void
mad_forward_disconnect(p_mad_connection_t connection)
{
  LOG_IN();
  /* currently unimplemented */
  LOG_OUT();
}

void
mad_forward_new_message(p_mad_connection_t connection)
{
  LOG_IN();
  if (connection->nature == mad_connection_nature_direct_virtual)
    {
      p_mad_connection_t       out       = NULL;
      p_mad_driver_interface_t interface = NULL;
      unsigned int             src       =    0;
      unsigned int             dst       =    0;
      unsigned char            data[mad_fblock_fsize] = {0};

      out = connection->regular;
      interface = out->channel->adapter->driver->interface;
      src = (unsigned int)connection->channel->process_lrank;
      dst = (unsigned int)connection->remote_rank;
      mad_forward_fill_fbh_data(data, src, dst, 0,
				tbx_false, tbx_false,
				tbx_false, tbx_true, tbx_false
#ifdef MAD_FORWARD_FLOW_CONTROL
				, tbx_false
#endif // MAD_FORWARD_FLOW_CONTROL
				);

      marcel_mutex_lock(&(out->lock_mutex));

      if (interface->new_message)
	interface->new_message(out);

      mad_forward_send_bytes(out, data, mad_fblock_fsize);
    }
  else if (connection->nature ==
	   mad_connection_nature_indirect_virtual)
    {
#ifdef MAD_FORWARD_FLOW_CONTROL
      {
	p_mad_connection_t out            = NULL;
	unsigned int       src            =    0;
	unsigned int       dst            =    0;
	p_mad_driver_interface_t interface = NULL;
	unsigned char      data[mad_fblock_fsize] = {0};

	out = connection->regular;
	interface = out->channel->adapter->driver->interface;
	src = (unsigned int)connection->channel->process_lrank;
	dst = (unsigned int)connection->remote_rank;
	mad_forward_fill_fbh_data(data, src, dst, 0,
				  tbx_false, tbx_true,
				  tbx_false, tbx_false, tbx_false,
				  tbx_false);

	marcel_mutex_lock(&(out->lock_mutex));
	if (interface->new_message)
	  interface->new_message(out);

	mad_forward_send_bytes(out, data, mad_fblock_fsize);

	if (interface->finalize_message)
	  interface->finalize_message(out);

	marcel_mutex_unlock(&(out->lock_mutex));
	marcel_sem_P(&connection->ack);
      }
#else // MAD_FORWARD_FLOW_CONTROL
      connection->new_msg = tbx_true;
#endif // MAD_FORWARD_FLOW_CONTROL
    }
  LOG_OUT();
}

void
mad_forward_finalize_message(p_mad_connection_t connection)
{
  LOG_IN();
  if (connection->nature == mad_connection_nature_direct_virtual)
    {
      p_mad_connection_t out = NULL;
      p_mad_driver_interface_t interface = NULL;

      out = connection->regular;
      interface = out->channel->adapter->driver->interface;

      if (interface->finalize_message)
	interface->finalize_message(out);

      marcel_mutex_unlock(&(out->lock_mutex));
    }
  else if (connection->nature ==
	   mad_connection_nature_indirect_virtual)
    {
      p_mad_connection_t out            = NULL;
      unsigned int       src            =    0;
      unsigned int       dst            =    0;
      p_mad_driver_interface_t interface = NULL;
      unsigned char      data[mad_fblock_fsize] = {0};

      out = connection->regular;
      interface = out->channel->adapter->driver->interface;
      src = (unsigned int)connection->channel->process_lrank;
      dst = (unsigned int)connection->remote_rank;
      mad_forward_fill_fbh_data(data, src, dst, 0,
				tbx_false, tbx_false,
				tbx_true, tbx_false, tbx_false
#ifdef MAD_FORWARD_FLOW_CONTROL
				, tbx_false
#endif // MAD_FORWARD_FLOW_CONTROL
				);

      marcel_mutex_lock(&(out->lock_mutex));
      if (interface->new_message)
	interface->new_message(out);

      mad_forward_send_bytes(out, data, mad_fblock_fsize);

      if (interface->finalize_message)
	interface->finalize_message(out);

      marcel_mutex_unlock(&(out->lock_mutex));
    }
  LOG_OUT();
}

#ifdef MAD_MESSAGE_POLLING
p_mad_connection_t
mad_forward_poll_message(p_mad_channel_t channel)
{
  p_mad_connection_t connection = NULL;

  LOG_IN();
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

#ifdef MAD_FORWARD_FLOW_CONTROL
  if (connection->nature == mad_connection_nature_indirect_virtual)
    {
      p_mad_connection_t vout = NULL;
      p_mad_connection_t out  = NULL;
      unsigned char      data[mad_fblock_fsize] = {0};
      p_mad_driver_interface_t interface = NULL;

      vout = connection->reverse;
      if (vout->nature == mad_connection_nature_direct_virtual)
	{
	  p_mad_channel_t     fchannel    = NULL;
	  p_mad_dir_channel_t dir_channel = NULL;

	  dir_channel = vout->regular->channel->dir_channel;

	  {
	    char name [2 + strlen(dir_channel->name)];

	    name[0] = 'f';
	    strcpy(name + 1, dir_channel->name);

	    fchannel =
	      tbx_htable_get(vout->regular->channel->adapter->channel_htable,
			     name);
	  }

	  out = tbx_darray_get(fchannel->out_connection_darray,
			       vout->regular->remote_rank);
	}
      else if (vout->nature == mad_connection_nature_indirect_virtual)
	{
	  out = vout->regular;
	}
      else
	TBX_FAILURE("invalid connection nature");

      interface = out->channel->adapter->driver->interface;

      marcel_mutex_lock(&(out->lock_mutex));

      if (interface->new_message)
	interface->new_message(out);

      mad_forward_fill_fbh_data(data, channel->process_lrank,
				vout->remote_rank, 0, tbx_false, tbx_false,
				tbx_false, tbx_false, tbx_false, tbx_true);

      mad_forward_send_bytes(out, data, mad_fblock_fsize);

      if (interface->finalize_message)
	interface->finalize_message(out);

      marcel_mutex_unlock(&(out->lock_mutex));
      connection->new_msg = tbx_false;
   }
#endif // MAD_FORWARD_FLOW_CONTROL
  LOG_OUT();

  return connection;
}
#endif // MAD_MESSAGE_POLLING

p_mad_connection_t
mad_forward_receive_message(p_mad_channel_t channel)
{
  p_mad_connection_t connection = NULL;

  LOG_IN();
  if (!channel->can_receive)
    {
      channel->can_receive = tbx_true;
      marcel_sem_V(&(channel->ready_to_receive));
    }
  marcel_sem_P(&(channel->message_ready));

  connection = channel->active_connection;
#ifdef MAD_FORWARD_FLOW_CONTROL
  if (connection->nature == mad_connection_nature_indirect_virtual)
    {
      p_mad_connection_t vout = NULL;
      p_mad_connection_t out  = NULL;
      unsigned char      data[mad_fblock_fsize] = {0};
      p_mad_driver_interface_t interface = NULL;

      vout = connection->reverse;
      if (vout->nature == mad_connection_nature_direct_virtual)
	{
	  p_mad_channel_t     fchannel    = NULL;
	  p_mad_dir_channel_t dir_channel = NULL;

	  dir_channel = vout->regular->channel->dir_channel;

	  {
	    char name [2 + strlen(dir_channel->name)];

	    name[0] = 'f';
	    strcpy(name + 1, dir_channel->name);

	    fchannel =
	      tbx_htable_get(vout->regular->channel->adapter->channel_htable,
			     name);
	  }

	  out = tbx_darray_get(fchannel->out_connection_darray,
			       vout->regular->remote_rank);
	}
      else if (vout->nature == mad_connection_nature_indirect_virtual)
	{
	  out = vout->regular;
	}
      else
	TBX_FAILURE("invalid connection nature");

      interface = out->channel->adapter->driver->interface;

      marcel_mutex_lock(&(out->lock_mutex));

      if (interface->new_message)
	interface->new_message(out);

      mad_forward_fill_fbh_data(data, channel->process_lrank,
				vout->remote_rank, 0, tbx_false, tbx_false,
				tbx_false, tbx_false, tbx_false, tbx_true);

      mad_forward_send_bytes(out, data, mad_fblock_fsize);

      if (interface->finalize_message)
	interface->finalize_message(out);

      marcel_mutex_unlock(&(out->lock_mutex));
      connection->new_msg = tbx_false;
   }
#endif // MAD_FORWARD_FLOW_CONTROL
  LOG_OUT();

  return connection;
}

void
mad_forward_message_received(p_mad_connection_t connection)
{
  p_mad_connection_t       regular   = NULL;
  p_mad_driver_interface_t interface = NULL;

  LOG_IN();
  regular = connection->regular;
  interface = regular->channel->adapter->driver->interface;

  if (interface->message_received)
    interface->message_received(regular);

  connection->channel->can_receive = tbx_false;
  marcel_sem_V(&(connection->channel->message_received));
  LOG_OUT();
}

void
mad_forward_before_close_channel(p_mad_channel_t channel)
{
  LOG_IN();
  /* unimplemented */
  LOG_OUT();
}

void
mad_forward_after_close_channel(p_mad_channel_t channel)
{
  LOG_IN();
  /* unimplemented */
  LOG_OUT();
}

void
mad_forward_link_exit(p_mad_link_t link)
{
  LOG_IN();
  /* unimplemented */
  LOG_OUT();
}

void
mad_forward_connection_exit(p_mad_connection_t in,
			    p_mad_connection_t out)
{
  LOG_IN();
  /* unimplemented */
  LOG_OUT();
}

void
mad_forward_channel_exit(p_mad_channel_t channel)
{
  LOG_IN();
  /* unimplemented */
  LOG_OUT();
}

p_mad_link_t
mad_forward_choice(p_mad_connection_t connection,
		   size_t             size         __attribute__ ((unused)),
		   mad_send_mode_t    send_mode    __attribute__ ((unused)),
		   mad_receive_mode_t receive_mode __attribute__ ((unused)))
{
  p_mad_link_t lnk = NULL;

  LOG_IN();
  if (connection->nature == mad_connection_nature_direct_virtual)
    {
      p_mad_connection_t       regular     = NULL;
      p_mad_driver_interface_t interface   = NULL;
      p_mad_link_t             regular_lnk = NULL;

      regular   = connection->regular;
      interface = regular->channel->adapter->driver->interface;
      if (interface->choice)
	{
	  regular_lnk = interface->choice(regular, size,
					  send_mode, receive_mode);
	  lnk = connection->link_array[regular_lnk->id];
	}
      else
	{
	  lnk = connection->link_array[0];
	}
    }
  else if (connection->nature ==
	   mad_connection_nature_indirect_virtual)
    {
      lnk = connection->link_array[0];
    }
  else
    TBX_FAILURE("invalid connection nature");

  LOG_OUT();

  return lnk;
}

void
mad_forward_return_static_buffer(p_mad_link_t   lnk,
				 p_mad_buffer_t buffer)
{
  p_mad_connection_t       vin       = NULL;
  p_mad_connection_t       in        = NULL;
  p_mad_driver_interface_t interface = NULL;

  LOG_IN();
  vin       = lnk->connection;
  in        = vin->regular;
  interface = in->channel->adapter->driver->interface;

  if (vin->nature == mad_connection_nature_direct_virtual)
    {
      p_mad_link_t regular_lnk = NULL;

      regular_lnk = in->link_array[lnk->id];
      interface->return_static_buffer(regular_lnk, buffer);
    }
  else if (vin->nature == mad_connection_nature_indirect_virtual)
    TBX_FAILURE("invalid function call");

  LOG_OUT();
}

p_mad_buffer_t
mad_forward_get_static_buffer(p_mad_link_t lnk)
{
  p_mad_buffer_t           buffer    = NULL;
  p_mad_connection_t       vout      = NULL;
  p_mad_connection_t       out       = NULL;
  p_mad_driver_interface_t interface = NULL;

  LOG_IN();
  vout      = lnk->connection;
  out       = vout->regular;
  interface = out->channel->adapter->driver->interface;

  if (vout->nature == mad_connection_nature_direct_virtual)
    {
      p_mad_link_t regular_lnk = NULL;

      regular_lnk = out->link_array[lnk->id];
      buffer = interface->get_static_buffer(regular_lnk);
    }
  else if (vout->nature == mad_connection_nature_indirect_virtual)
    TBX_FAILURE("invalid function call");

  LOG_OUT();

  return buffer;
}


void
mad_forward_send_buffer(p_mad_link_t   lnk,
			p_mad_buffer_t buffer)
{
  p_mad_connection_t       vout      = NULL;
  p_mad_connection_t       out       = NULL;
  p_mad_driver_interface_t interface = NULL;

  LOG_IN();
  vout      = lnk->connection;
  out       = vout->regular;
  interface = out->channel->adapter->driver->interface;

  if (vout->nature == mad_connection_nature_direct_virtual)
    {
      p_mad_link_t regular_lnk = NULL;

      regular_lnk = out->link_array[lnk->id];
      interface->send_buffer(regular_lnk, buffer);
    }
  else if (vout->nature == mad_connection_nature_indirect_virtual)
    {
      unsigned int   mtu            =    0;
      unsigned int   nb_block       =    0;
      unsigned int   last_block_len =    0;
      unsigned int   src            =    0;
      unsigned int   dst            =    0;
      tbx_bool_t     is_a_new_msg   = tbx_false;
      unsigned char *ptr            = NULL;
      unsigned int   bytes_read     =    0;
      unsigned char  data[mad_fblock_fsize] = {0};

      mtu            = vout->mtu;
      nb_block       =
	(buffer->bytes_written - buffer->bytes_read) / mtu;
      last_block_len =
	(buffer->bytes_written - buffer->bytes_read) % mtu;
      bytes_read     = buffer->bytes_read;
      ptr            = buffer->buffer;
      src            = (unsigned int)vout->channel->process_lrank;
      dst            = (unsigned int)vout->remote_rank;
      is_a_new_msg   = vout->new_msg;
      vout->new_msg  = tbx_false;

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
	  mad_forward_fill_fbh_data(data, src, dst, nb_block + 1,
				    tbx_true, is_a_new_msg,
				    tbx_false, tbx_false, tbx_false
#ifdef MAD_FORWARD_FLOW_CONTROL
				, tbx_false
#endif // MAD_FORWARD_FLOW_CONTROL
				);
	  is_a_new_msg = tbx_false;
	  mad_forward_send_bytes(out, data, mad_fblock_fsize);

	  do
	    {
	      mad_forward_fill_fbh_data(data, src, dst, mtu,
					tbx_false, tbx_false,
					tbx_false, tbx_false, tbx_false
#ifdef MAD_FORWARD_FLOW_CONTROL
				, tbx_false
#endif // MAD_FORWARD_FLOW_CONTROL
				);
	      mad_forward_send_bytes(out, data, mad_fblock_fsize);
	      mad_forward_send_bytes(out, ptr + bytes_read, mtu);
	      bytes_read += mtu;
	    }
	  while (--nb_block);
	}

      if (!last_block_len)
	TBX_FAILURE("no data");

      mad_forward_fill_fbh_data(data, src, dst, last_block_len,
				tbx_false, is_a_new_msg,
				tbx_false, tbx_false, tbx_false
#ifdef MAD_FORWARD_FLOW_CONTROL
				, tbx_false
#endif // MAD_FORWARD_FLOW_CONTROL
				);
      mad_forward_send_bytes(out, data, mad_fblock_fsize);
      mad_forward_send_bytes(out, ptr + bytes_read, last_block_len);
      buffer->bytes_read = (size_t)(bytes_read + last_block_len);

      if (interface->finalize_message)
	interface->finalize_message(out);

      marcel_mutex_unlock(&(out->lock_mutex));
    }
  else
    TBX_FAILURE("invalid connection nature");

  LOG_OUT();
}

void
mad_forward_receive_buffer(p_mad_link_t    lnk,
			   p_mad_buffer_t *buffer)
{
  p_mad_connection_t       vin       = NULL;
  p_mad_connection_t       in        = NULL;
  p_mad_driver_interface_t interface = NULL;

  LOG_IN();
  vin = lnk->connection;
  in  = vin->regular;
  interface = in->channel->adapter->driver->interface;

  if (vin->nature == mad_connection_nature_direct_virtual)
    {
      p_mad_link_t regular_lnk = NULL;

      regular_lnk = in->link_array[lnk->id];
      interface->receive_buffer(regular_lnk, buffer);
    }
  else if (vin->nature == mad_connection_nature_indirect_virtual)
    {
      p_mad_buffer_t      buf           = NULL;
      unsigned int        nb_block      =    0;
      unsigned int        block_len     =    0;
      unsigned char      *ptr           = NULL;
      unsigned int        bytes_written =    0;
      unsigned char       data[mad_fblock_fsize] = {0};

      buf = *buffer;
      ptr = buf->buffer + buf->bytes_written;
      bytes_written = (unsigned int)buf->bytes_written;

      if (vin->new_msg)
	{
	  vin->new_msg = tbx_false;

	  if (vin->first_block_is_a_group)
	    {
	      nb_block = (unsigned int)vin->first_block_length;
	    }
	  else
	    {
	      block_len = (unsigned int)vin->first_block_length;
	      nb_block = 1;
	    }
	}
      else
	{
	  unsigned int length     = 0;
	  unsigned int is_a_group = 0;

	  mad_forward_receive_bytes(in, data, mad_fblock_fsize);
	  length =
	    tbx_expand_field(data, mad_fblock_flength_0)
	    | tbx_expand_field(data, mad_fblock_flength_1)
	    | tbx_expand_field(data, mad_fblock_flength_2)
	    | tbx_expand_field(data, mad_fblock_flength_3);

	  is_a_group = tbx_expand_field(data, mad_fblock_fis_a_group);

	  if (is_a_group)
	    {
	      nb_block = length;
	    }
	  else
	    {
	      block_len = length;
	      nb_block  = 1;
	    }
	}

      if (nb_block > 1)
	{
	  while (nb_block--)
	    {
	      mad_forward_receive_bytes(in, data, mad_fblock_fsize);
	      block_len =
		tbx_expand_field(data, mad_fblock_flength_0)
		| tbx_expand_field(data, mad_fblock_flength_1)
		| tbx_expand_field(data, mad_fblock_flength_2)
		| tbx_expand_field(data, mad_fblock_flength_3);

	      mad_forward_receive_bytes(in, ptr + bytes_written,
					block_len);
	      bytes_written += block_len;
	    }
	}
      else
	{
	  mad_forward_receive_bytes(in, ptr + bytes_written,
				    block_len);
	  bytes_written += block_len;
	}

      buf->bytes_written = (size_t)bytes_written;
    }
  else
    TBX_FAILURE("invalid connection nature");

  LOG_OUT();
}

void
mad_forward_send_buffer_group(p_mad_link_t         lnk,
			      p_mad_buffer_group_t buffer_group)
{
  p_mad_connection_t vout = NULL;

  LOG_IN();
  vout = lnk->connection;

  if (vout->nature == mad_connection_nature_direct_virtual)
    {
      p_mad_connection_t       out         = NULL;
      p_mad_driver_interface_t interface   = NULL;
      p_mad_link_t             regular_lnk = NULL;

      out       = vout->regular;
      interface = out->channel->adapter->driver->interface;

      regular_lnk = out->link_array[lnk->id];
      interface->send_buffer_group(regular_lnk, buffer_group);
    }
  else if (vout->nature == mad_connection_nature_indirect_virtual)
    {
      if (!tbx_empty_list(&(buffer_group->buffer_list)))
	{
	  tbx_list_reference_t ref;

	  tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
	  do
	    {
	      p_mad_buffer_t buffer = NULL;

	      buffer = tbx_get_list_reference_object(&ref);
	      mad_forward_send_buffer(lnk, buffer);
	    }
	  while(tbx_forward_list_reference(&ref));
	}
    }
  else
    TBX_FAILURE("invalid connection nature");

  LOG_OUT();
}

void
mad_forward_receive_sub_buffer_group(p_mad_link_t         lnk,
				     tbx_bool_t           first_sub_group
				     __attribute__ ((unused)),
				     p_mad_buffer_group_t buffer_group)
{
p_mad_connection_t vin = NULL;

  LOG_IN();
  vin = lnk->connection;

  if (vin->nature == mad_connection_nature_direct_virtual)
    {
      p_mad_connection_t       in         = NULL;
      p_mad_driver_interface_t interface   = NULL;
      p_mad_link_t             regular_lnk = NULL;

      in       = vin->regular;
      interface = in->channel->adapter->driver->interface;

      regular_lnk = in->link_array[lnk->id];
      interface->receive_sub_buffer_group(regular_lnk,
					  first_sub_group,
					  buffer_group);
    }
  else if (vin->nature == mad_connection_nature_indirect_virtual)
    {
      if (!tbx_empty_list(&(buffer_group->buffer_list)))
	{
	  tbx_list_reference_t ref;

	  tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
	  do
	    {
	      p_mad_buffer_t buffer = NULL;

	      buffer = tbx_get_list_reference_object(&ref);
	      mad_forward_receive_buffer(lnk, &buffer);
	    }
	  while(tbx_forward_list_reference(&ref));
	}
    }
  else
    TBX_FAILURE("invalid connection nature");

  LOG_OUT();
}

// Specific clean-up functions
void
mad_forward_stop_direct_retransmit(p_mad_channel_t channel)
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

	  if (out->message_queue)
	    {
	      any_t              status = NULL;
	      size_t             bq_len =    0;
	      tbx_darray_index_t bq_idx =    0;

	      marcel_sem_V(&(out->something_to_forward));
	      marcel_join(out->forwarding_thread, &status);
	      out->forwarding_thread = NULL;

	      tbx_slist_free(out->message_queue);
	      out->message_queue = NULL;

	      bq_len = tbx_darray_length(out->block_queues);

	      while (bq_idx < bq_len)
		{
		  p_mad_forward_block_queue_t fbq = NULL;

		  fbq = tbx_darray_get(out->block_queues, bq_idx);

		  if (fbq)
		    {
		      tbx_slist_free(fbq->queue);
		      fbq->queue = NULL;

		      TBX_FREE(fbq);
		      tbx_darray_set(out->block_queues, bq_idx, NULL);
		    }

		  bq_idx++;
		}

	      tbx_darray_free(out->block_queues);
	      out->block_queues = NULL;
	    }
	}

      idx++;
    }
  LOG_OUT();
}

void
mad_forward_stop_indirect_retransmit(p_mad_channel_t channel)
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
mad_forward_stop_reception(p_mad_channel_t      vchannel,
			   p_mad_channel_t      channel,
			   ntbx_process_lrank_t lrank)
{
  LOG_IN();

  if (lrank == -1)
    {
      // Receiver
      any_t status = NULL;

      if (channel->type == mad_channel_type_regular)
	{
	  if (!vchannel->can_receive)
	    {
	      vchannel->can_receive = tbx_true;
	      marcel_sem_V(&(vchannel->ready_to_receive));
	    }
	}

      marcel_join(channel->polling_thread, &status);
      channel->polling_thread = NULL;
    }
  else
    {
      // Sender
      p_mad_connection_t       out       = NULL;
      p_mad_driver_interface_t interface = NULL;
      unsigned char            data[mad_fblock_fsize] = {0};

      out = tbx_darray_get(channel->out_connection_darray, lrank);

      mad_forward_fill_fbh_data(data, 0, 0, 0, tbx_false, tbx_false,
				tbx_false, tbx_false, tbx_true
#ifdef MAD_FORWARD_FLOW_CONTROL
				, tbx_false
#endif // MAD_FORWARD_FLOW_CONTROL
				);
      interface = out->channel->adapter->driver->interface;
      if (interface->new_message)
	interface->new_message(out);

      mad_forward_send_bytes(out, data, mad_fblock_fsize);

      if (interface->finalize_message)
	interface->finalize_message(out);
    }
  LOG_OUT();
}
#endif // MARCEL

