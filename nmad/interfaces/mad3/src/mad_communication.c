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
 * Mad_communication.c
 * ===================
 */
#include "madeleine.h"

#include <stdarg.h>

#if !defined CONFIG_SCHED_OPT
void
mad_free_parameter_slist(p_tbx_slist_t parameter_slist)
{
  if (!parameter_slist)
    return;

  while (!tbx_slist_is_nil(parameter_slist))
    {
      p_mad_buffer_slice_parameter_t param = NULL;

      param = tbx_slist_extract(parameter_slist);
      mad_free_slice_parameter(param);
    }

  tbx_slist_free(parameter_slist);
}

p_mad_connection_t
mad_begin_packing(p_mad_channel_t      channel,
		  ntbx_process_lrank_t remote_rank)
{
  p_mad_driver_interface_t interface  = NULL;
  p_mad_connection_t       connection = NULL;
  mad_link_id_t            link_id    =  -1;

  LOG_IN();
  TRACE("New emission request");
  if (!channel->not_private)
    return NULL;

  interface = channel->adapter->driver->interface;

  connection = tbx_darray_get(channel->out_connection_darray, remote_rank);

  if (!connection)
    {
      LOG_OUT();

      return NULL;
    }

#ifdef MARCEL
  marcel_mutex_lock(&(connection->lock_mutex));
#else /* MARCEL */
  if (connection->lock == tbx_true)
    TBX_FAILURE("mad_begin_packing: connection dead lock");
  connection->lock = tbx_true;
#endif /* MARCEL */

  if (interface->new_message)
    interface->new_message(connection);

  /* structure initialisation */
  tbx_list_init(connection->buffer_list);
  tbx_list_init(connection->buffer_group_list);

  for(link_id = 0;
      link_id < connection->nb_link;
      link_id++)
    {
      tbx_list_init(connection->link_array[link_id]->buffer_list);
    }

  connection->pair_list_used = tbx_false;
  connection->delayed_send   = tbx_false;
  connection->flushed        = tbx_true;
  connection->last_link      = NULL;
  TRACE("Emission request initiated");
  LOG_OUT();
  return connection;
}

#ifdef MAD_MESSAGE_POLLING
p_mad_connection_t
mad_message_ready(p_mad_channel_t channel)
{
  p_mad_driver_interface_t   interface  = NULL;
  p_mad_connection_t         connection = NULL;
  mad_link_id_t              link_id    =   -1;

  LOG_IN();
  TRACE("New reception request");

  if (!channel->not_private)
    return NULL;

  interface = channel->adapter->driver->interface;

#ifdef MARCEL
  if (!marcel_mutex_trylock(&(channel->reception_lock_mutex)))
    {
      LOG_OUT();

      return NULL;
    }
#else /* MARCEL */
  if (channel->reception_lock == tbx_true)
    TBX_FAILURE("mad_begin_unpacking: reception dead lock");
  channel->reception_lock = tbx_true;
#endif /* MARCEL */

  if (!(connection = interface->poll_message(channel)))
    {
#ifdef MARCEL
      marcel_mutex_unlock(&(channel->reception_lock_mutex));
#else // MARCEL
      channel->reception_lock = tbx_false;
#endif // MARCEL
      LOG_OUT();

      return NULL;
    }

   tbx_list_init(connection->buffer_list);
   tbx_list_init(connection->buffer_group_list);
   tbx_list_init(connection->user_buffer_list);
   tbx_list_reference_init(connection->user_buffer_list_reference,
			   connection->user_buffer_list);

  for(link_id = 0;
      link_id < connection->nb_link;
      link_id++)
    {
      tbx_list_init(connection->link_array[link_id]->buffer_list);
      tbx_list_init(connection->link_array[link_id]->user_buffer_list);
    }

  connection->pair_list_used  = tbx_false;
  connection->delayed_send    = tbx_false;
  connection->flushed         = tbx_true;
  connection->last_link       = NULL;
  connection->first_sub_buffer_group = tbx_true;
  connection->more_data       = tbx_false;
  connection->parameter_slist = NULL;
  LOG_OUT();
  return connection;
}
#endif /* MAD_MESSAGE_POLLING */

p_mad_connection_t
mad_begin_unpacking(p_mad_channel_t channel)
{
  p_mad_driver_interface_t interface  = NULL;
  p_mad_connection_t       connection = NULL;
  mad_link_id_t            link_id    =   -1;

  LOG_IN();
  TRACE("New reception request");

  if (!channel->not_private)
    return NULL;

  interface = channel->adapter->driver->interface;

#ifdef MARCEL
  marcel_mutex_lock(&(channel->reception_lock_mutex));
#else /* MARCEL */
  if (channel->reception_lock == tbx_true)
    TBX_FAILURE("mad_begin_unpacking: reception dead lock");
  channel->reception_lock = tbx_true;
#endif /* MARCEL */

  TRACE("Receiving message");
  connection = interface->receive_message(channel);
  if (!connection)
    TBX_FAILURE("message reception failed");

  TRACE("Message received");

  tbx_list_init(connection->buffer_list);
  tbx_list_init(connection->buffer_group_list);
  tbx_list_init(connection->user_buffer_list);
  tbx_list_reference_init(connection->user_buffer_list_reference,
			  connection->user_buffer_list);

  for(link_id = 0;
      link_id < connection->nb_link;
      link_id++)
    {
      tbx_list_init(connection->link_array[link_id]->buffer_list);
      tbx_list_init(connection->link_array[link_id]->user_buffer_list);
    }

  connection->pair_list_used  = tbx_false;
  connection->delayed_send    = tbx_false;
  connection->flushed         = tbx_true;
  connection->last_link       = NULL;
  connection->first_sub_buffer_group = tbx_true;
  connection->more_data       = tbx_false;
  connection->parameter_slist = NULL;

  TRACE("Reception request initiated");
  LOG_OUT();
  return connection;
}

void
mad_end_packing(p_mad_connection_t connection)
{
  mad_link_id_t            link_id;
  p_mad_driver_interface_t interface =
    connection->channel->adapter->driver->interface;

  LOG_IN();
  if (connection->flushed == tbx_false)
    {
      p_mad_link_t last_link   = connection->last_link;
      p_tbx_list_t buffer_list = connection->buffer_list;

     if (connection->delayed_send == tbx_false)
	{
	  if (connection->last_link_mode == mad_link_mode_buffer)
	    {
	      p_mad_buffer_t buffer;

	      buffer = tbx_get_list_object(buffer_list);
	      interface->send_buffer(last_link, buffer);
	    }
	  else if (connection->last_link_mode == mad_link_mode_buffer_group)
	    {
	      mad_buffer_group_t buffer_group;

	      mad_make_buffer_group(&buffer_group,
				    buffer_list,
				    last_link);
	      if (last_link->group_mode == mad_group_mode_split
		  && buffer_group.buffer_list.length == 1)
		{
		  interface->
		    send_buffer(last_link,
				buffer_group.buffer_list.first->object);
		}
	      else
		{
		  interface->send_buffer_group(last_link, &buffer_group);
		}
	    }
	  else
	    TBX_FAILURE("invalid link mode");
	}
      else
	{
	  if (connection->last_link_mode == mad_link_mode_buffer_group)
	    {
	      p_mad_buffer_group_t buffer_group;

	      buffer_group = mad_alloc_buffer_group_struct();
	      mad_make_buffer_group(buffer_group, buffer_list, last_link);
	      tbx_append_list(connection->buffer_group_list, buffer_group);
	    }
	  else
	    TBX_FAILURE("invalid link mode");
	}

      connection->flushed = tbx_true;
    }

  /* buffer pair handling */
  if (connection->pair_list_used)
    {
      if (!tbx_empty_list(connection->pair_list))
	{
	  tbx_list_reference_t ref;

	  tbx_list_reference_init(&ref, connection->pair_list);

	  do
	    {
	      p_mad_buffer_pair_t pair ;

	      pair = tbx_get_list_reference_object(&ref);
	      mad_copy_buffer(&(pair->dynamic_buffer),
			      &(pair->static_buffer));
	    }
	  while (tbx_forward_list_reference(&ref));

	  tbx_foreach_destroy_list(connection->pair_list,
				   mad_foreach_free_buffer_pair_struct);
	}
    }

  /* residual buffer_group flushing */
  if (!tbx_empty_list(connection->buffer_group_list))
    {
      tbx_list_reference_t list_ref;

      tbx_list_reference_init(&list_ref, connection->buffer_group_list);

      do
	{
	  p_mad_buffer_group_t buffer_group ;

	  buffer_group = tbx_get_list_reference_object(&list_ref);
	  if (   buffer_group->link->group_mode == mad_group_mode_split
	      && buffer_group->buffer_list.length == 1)
	    {
	      interface->send_buffer(buffer_group->link,
				     buffer_group->buffer_list.first->object);
	    }
	  else
	    {
	      interface->send_buffer_group(buffer_group->link, buffer_group);
	    }
	}
      while (tbx_forward_list_reference(&list_ref));
    }

  /* link data groups flushing */
  for (link_id = 0;
       link_id < connection->nb_link;
       link_id++)
    {
      p_mad_link_t lnk = connection->link_array[link_id];

      if (!tbx_empty_list(lnk->buffer_list))
	{
	  mad_buffer_group_t buffer_group;

	  mad_make_buffer_group(&buffer_group, lnk->buffer_list, lnk);
	  if (   lnk->group_mode == mad_group_mode_split
	      && buffer_group.buffer_list.length == 1)
	    {
	      interface->send_buffer(lnk,
				     buffer_group.buffer_list.first->object);
	    }
	  else
	    {
	      interface->send_buffer_group(lnk, &buffer_group);
	    }
	}
    }

  tbx_foreach_destroy_list(connection->buffer_group_list,
			   mad_foreach_free_buffer_group_struct);
  tbx_foreach_destroy_list(connection->buffer_list, mad_foreach_free_buffer);
  for (link_id = 0; link_id < connection->nb_link; link_id++)
    {
	tbx_foreach_destroy_list(connection->link_array[link_id]->buffer_list,
				 mad_foreach_free_buffer);
    }

  if (interface->finalize_message)
    interface->finalize_message(connection);

#ifdef MARCEL
  marcel_mutex_unlock(&(connection->lock_mutex));
#else // MARCEL
  connection->lock = tbx_false;
#endif // MARCEL
  TRACE("Emission request completed");
  LOG_OUT();
}

p_tbx_slist_t
mad_end_unpacking_ext(p_mad_connection_t connection)
{
  p_mad_driver_interface_t interface       =
    connection->channel->adapter->driver->interface;
  p_tbx_list_t             src_list        = NULL;
  p_tbx_list_t             dest_list       = NULL;
  p_tbx_list_reference_t   dest_ref        = NULL;
  p_mad_buffer_t           source          = NULL;
  p_mad_buffer_t           destination     = NULL;
  p_tbx_slist_t            parameter_slist = NULL;
  mad_link_id_t            link_id         =   -1;

  LOG_IN();
  if (connection->flushed == tbx_false)
    {
      p_mad_link_t      last_link        = connection->last_link;
      mad_buffer_mode_t last_buffer_mode = last_link->buffer_mode;

      src_list  = connection->buffer_list;
      dest_list = connection->user_buffer_list;
      dest_ref  = connection->user_buffer_list_reference;

      if (connection->last_link_mode == mad_link_mode_buffer_group)
	{
	  if (last_buffer_mode == mad_buffer_mode_static)
	    {
	      if (!tbx_empty_list(dest_list)
		  && !tbx_reference_after_end_of_list(dest_ref))
		{
		  if (tbx_empty_list(src_list)
		      || !connection->more_data)
		    {
		      interface->receive_buffer(last_link, &source);
		      tbx_append_list(src_list, source);
		    }
		  else
		    {
		      source = tbx_get_list_object(src_list);
		    }

		  do
		    {
		      destination = tbx_get_list_reference_object(dest_ref);

		      do
			{
			  if (!mad_more_data(source))
			    {
			      interface->
				return_static_buffer(last_link, source);
			      interface->
				receive_buffer(last_link, &source);
			      tbx_append_list(src_list, source);
			    }
			  mad_copy_buffer(source, destination);
			}
		      while (!mad_buffer_full(destination));
		    }
		  while (tbx_forward_list_reference(dest_ref));

		  interface->return_static_buffer(last_link, source);
		}
	      else
		{
		  if (connection->more_data)
		    {
		      source = tbx_get_list_object(src_list);
		      interface->return_static_buffer(last_link, source);
		    }
		}
	    }
	  else
	    {
	      mad_buffer_group_t buffer_group;

	      mad_make_buffer_group(&buffer_group,
				    src_list,
				    last_link);
	      if (last_link->group_mode == mad_group_mode_split
		  && buffer_group.buffer_list.length == 1)
		{
                  p_mad_buffer_t buffer = buffer_group.
                    buffer_list.first->object;

		  interface->receive_buffer(last_link, &buffer);
		}
	      else
		{
		  interface->
		    receive_sub_buffer_group(last_link,
					     connection->
					     first_sub_buffer_group,
					     &buffer_group);
		  /*
		    if (interface->finalize_sub_buffer_group_reception)
		    {
		    interface->
		    finalize_sub_buffer_group_reception(last_link);
		    }
		  */
		}
	    }
	}
      else if (connection->last_link_mode == mad_link_mode_buffer)
	{
	  interface->return_static_buffer(last_link,
					  tbx_get_list_object(src_list));
	}
      else
	TBX_FAILURE("invalid link mode");

      tbx_mark_list(src_list);
      connection->more_data              = tbx_false;
      connection->flushed                = tbx_true;
      connection->first_sub_buffer_group = tbx_false;
    }

  /* link data groups reception */
  for (link_id = 0;
       link_id < connection->nb_link;
       link_id++)
    {
      /* A buffer_group must be created from the buffers */
      p_mad_link_t lnk = connection->link_array[link_id];

      src_list = lnk->buffer_list;

      if (lnk->buffer_mode == mad_buffer_mode_static)
	{
	  tbx_list_reference_t destination_list_reference;

	  dest_list = lnk->user_buffer_list;
	  dest_ref  = &destination_list_reference;
	  tbx_list_reference_init(dest_ref, dest_list);

	  if (!tbx_empty_list(dest_list))
	    {
	      interface->receive_buffer(lnk, &source);
	      tbx_append_list(src_list, source);

	      do
		{
		  destination = tbx_get_list_reference_object(dest_ref);

		  while (!mad_buffer_full(destination))
		    {
		      if (!mad_more_data(source))
			{
			  interface->return_static_buffer(lnk, source);
			  interface->receive_buffer(lnk, &source);
			  tbx_append_list(src_list, source);
			}
		      mad_copy_buffer(source, destination);
		    }
		}
	      while(tbx_forward_list_reference(dest_ref));

	      interface->return_static_buffer(lnk, source);
	    }
	}
      else
	{
	  if (!tbx_empty_list(src_list))
	    {
	      mad_buffer_group_t buffer_group;

	      mad_make_buffer_group(&buffer_group, src_list, lnk);

	      if (lnk->group_mode == mad_group_mode_split
		  && buffer_group.buffer_list.length == 1)
		{
                  p_mad_buffer_t buffer = buffer_group.buffer_list.first->object;

		  interface->receive_buffer(lnk, &buffer);
		}
	      else
		{
		  interface->
		    receive_sub_buffer_group(lnk,
					     connection->
					     first_sub_buffer_group,
					     &buffer_group);
		}
	    }
	}
    }

  // final parameter processing
  {
    tbx_list_reference_t ref;

    if (!tbx_empty_list(connection->buffer_list))
      {
        tbx_list_reference_init(&ref, connection->buffer_list);
        do
          {
            p_mad_buffer_t buffer = NULL;

            buffer = tbx_get_list_reference_object(&ref);
            if (buffer->type == mad_user_buffer && buffer->parameter_slist)
              {
                if (!connection->parameter_slist)
                  {
                    connection->parameter_slist = buffer->parameter_slist;
                  }
                else
                  {
                    tbx_slist_merge_after(connection->parameter_slist,
                                          buffer->parameter_slist);
                    tbx_slist_clear_and_free(buffer->parameter_slist);
                  }

                buffer->parameter_slist = NULL;
              }
          }
        while (tbx_forward_list_reference(&ref));
      }

    if (!tbx_empty_list(connection->user_buffer_list))
      {
        tbx_list_reference_init(&ref, connection->user_buffer_list);
        do
          {
            p_mad_buffer_t buffer = NULL;

            buffer = tbx_get_list_reference_object(&ref);
            if (buffer->type == mad_user_buffer && buffer->parameter_slist)
              {
                if (!connection->parameter_slist)
                  {
                    connection->parameter_slist = buffer->parameter_slist;
                  }
                else
                  {
                    tbx_slist_merge_after(connection->parameter_slist,
                                          buffer->parameter_slist);
                    tbx_slist_clear_and_free(buffer->parameter_slist);
                  }

                buffer->parameter_slist = NULL;
              }
          }
        while (tbx_forward_list_reference(&ref));
      }

    for (link_id = 0; link_id < connection->nb_link; link_id++)
      {
        if (!tbx_empty_list(connection->link_array[link_id]->buffer_list))
          {
            tbx_list_reference_init(&ref, connection->link_array[link_id]->buffer_list);
            do
              {
                p_mad_buffer_t buffer = NULL;

                buffer = tbx_get_list_reference_object(&ref);
                if (buffer->type == mad_user_buffer && buffer->parameter_slist)
                  {
                    if (!connection->parameter_slist)
                      {
                        connection->parameter_slist = buffer->parameter_slist;
                      }
                    else
                      {
                        tbx_slist_merge_after(connection->parameter_slist,
                                              buffer->parameter_slist);
                        tbx_slist_clear_and_free(buffer->parameter_slist);
                      }

                    buffer->parameter_slist = NULL;
                  }
              }
            while (tbx_forward_list_reference(&ref));
          }

        if (!tbx_empty_list(connection->link_array[link_id]->user_buffer_list))
          {
            tbx_list_reference_init(&ref, connection->link_array[link_id]->user_buffer_list);
            do
              {
                p_mad_buffer_t buffer = NULL;

                buffer = tbx_get_list_reference_object(&ref);
                if (buffer->type == mad_user_buffer && buffer->parameter_slist)
                  {
                    if (!connection->parameter_slist)
                      {
                        connection->parameter_slist = buffer->parameter_slist;
                      }
                    else
                      {
                        tbx_slist_merge_after(connection->parameter_slist,
                                              buffer->parameter_slist);
                        tbx_slist_clear_and_free(buffer->parameter_slist);
                      }

                    buffer->parameter_slist = NULL;
                  }
              }
            while (tbx_forward_list_reference(&ref));
          }

        tbx_foreach_destroy_list(connection->link_array[link_id]->user_buffer_list,
                                 mad_foreach_free_buffer);
      }
  }

  tbx_foreach_destroy_list(connection->buffer_list, mad_foreach_free_buffer);
  tbx_foreach_destroy_list(connection->user_buffer_list,
			   mad_foreach_free_buffer);

  for (link_id = 0; link_id < connection->nb_link; link_id++)
    {
      tbx_foreach_destroy_list(connection->link_array[link_id]->buffer_list,
			       mad_foreach_free_buffer);
      tbx_foreach_destroy_list(connection->link_array[link_id]->user_buffer_list,
			       mad_foreach_free_buffer);
    }

  if (interface->message_received)
    interface->message_received(connection);

  parameter_slist = connection->parameter_slist;
  connection->parameter_slist = NULL;

#ifdef MARCEL
  marcel_mutex_unlock(&(connection->channel->reception_lock_mutex));
#else // MARCEL
  connection->channel->reception_lock = tbx_false;
#endif // MARCEL

  TRACE("Reception request completed");
  LOG_OUT();

  return parameter_slist;
}


void
mad_end_unpacking(p_mad_connection_t connection)
{
  p_tbx_slist_t parameter_slist = NULL;

  parameter_slist = mad_end_unpacking_ext(connection);
  mad_free_parameter_slist(parameter_slist);
}


/*
 * mad_pack
 * --------
 */
void
mad_pack_ext(p_mad_connection_t   connection,
             void                *user_buffer,
             size_t               user_buffer_length,
             mad_send_mode_t      send_mode,
             mad_receive_mode_t   receive_mode, ...)
{
  p_mad_driver_interface_t   interface         =
    connection->channel->adapter->driver->interface;
  p_mad_link_t               lnk               = NULL;
  mad_link_id_t              link_id           =   -1;
  mad_link_mode_t            link_mode         = mad_link_mode_unknown;
  mad_buffer_mode_t          buffer_mode       = mad_buffer_mode_unknown;
  mad_group_mode_t           group_mode        = mad_group_mode_unknown;
  tbx_bool_t                 last_delayed_send = tbx_false;
  p_mad_buffer_t             source            = NULL;
  p_mad_buffer_t             destination       = NULL;
  p_tbx_list_t               dest_list         = connection->buffer_list;
  p_tbx_list_t               buffer_group_list = connection->buffer_group_list;
  tbx_bool_t                 free_source       = tbx_false;
  va_list                    arg_list;

  LOG_IN();
  if (!user_buffer_length)
    {
      LOG_OUT();
      return;
    }

#if 0
  if ((unsigned long)user_buffer & (MAD_ALIGNMENT - 1))
    TBX_FAILURE("buffer is not aligned on MAD_ALIGNMENT bytes");

  if (user_buffer_length & (MAD_LENGTH_ALIGNMENT - 1))
    TBX_FAILURE("buffer length is not aligned on MAD_LENGTH_ALIGNMENT bytes");
#endif

  if (interface->choice)
    {
      lnk = interface->choice(connection,
			      user_buffer_length,
			      send_mode,
			      receive_mode);
    }
  else
    {
      lnk = connection->link_array[0];
    }
  TRACE("Link chosen : %d", (int)lnk->id);

  link_id               = lnk->id;
  link_mode             = lnk->link_mode;
  buffer_mode           = lnk->buffer_mode;
  group_mode            = lnk->group_mode;
  last_delayed_send     = connection->delayed_send;

  if (      (send_mode == mad_send_LATER)
      && (receive_mode == mad_receive_EXPRESS))
    {
      connection->delayed_send = tbx_true;
    }

  if (connection->delayed_send)
    {
      if (receive_mode == mad_receive_EXPRESS)
	{
	  link_mode = mad_link_mode_buffer_group;
	}
      else
	{
	  link_mode = mad_link_mode_link_group;
	}
    }
  else
    {
      if (   (send_mode    == mad_send_LATER)
 	  && (receive_mode == mad_receive_CHEAPER))
	{
	  link_mode = mad_link_mode_link_group;
	}
      else
	{
	  link_mode = lnk->link_mode;

	  if (   (receive_mode == mad_receive_EXPRESS)
	      && (link_mode    == mad_link_mode_link_group))
	    {
	      link_mode = mad_link_mode_buffer_group;
	    }
	}
    }

  if (   (connection->last_link != NULL)
      && (link_mode != mad_link_mode_link_group)
      && (   (lnk != connection->last_link)
	  || (    (last_delayed_send != connection->delayed_send)
	       && (connection->last_link_mode == mad_link_mode_buffer)))
      && (connection->flushed == tbx_false))
    {
      p_mad_link_t last_link = connection->last_link;

      if (last_delayed_send == tbx_false)
	{
	  if (connection->last_link_mode == mad_link_mode_buffer)
	    {
	      interface->
		send_buffer(last_link, tbx_get_list_object(dest_list));
	    }
	  else if (connection->last_link_mode ==
		   mad_link_mode_buffer_group)
	    {
	      mad_buffer_group_t buffer_group;

	      mad_make_buffer_group(&buffer_group, dest_list, last_link);
	      if (last_link->group_mode == mad_group_mode_split
		  && buffer_group.buffer_list.length == 1)
		{
		  interface->send_buffer(last_link,
					 buffer_group.buffer_list.first->object);
		}
	      else
		{
		  interface->send_buffer_group(last_link, &buffer_group);
		}
	    }
	  else
	    TBX_FAILURE("invalid link mode");
	}
      else
	{
	  if (connection->last_link_mode == mad_link_mode_buffer_group)
	    {
	      p_mad_buffer_group_t buffer_group;

	      buffer_group = mad_alloc_buffer_group_struct();
	      mad_make_buffer_group(buffer_group, dest_list, last_link);
	      tbx_append_list(buffer_group_list, buffer_group);
	    }
	  else
	    TBX_FAILURE("invalid link mode");
	}

      connection->flushed = tbx_true;
      tbx_mark_list(dest_list);
    }

  source = mad_get_user_send_buffer(user_buffer, user_buffer_length);

  va_start(arg_list, receive_mode);
  {
    p_mad_buffer_slice_parameter_t param = NULL;
    p_tbx_slist_t                  slist = NULL;

    while ((param = va_arg(arg_list, void *)))
      {
        if (!slist)
          {
            slist = tbx_slist_nil();
          }

        if (!param->length)
          {
            param->length = source->length - param->offset;
          }

        TRACE_VAL("source->length", source->length);
        TRACE_VAL("param->offset", param->offset);
        TRACE_VAL("param->length", param->length);
        TRACE_VAL("param->opcode", param->opcode);

        if (param->offset >= source->length)
          TBX_FAILURE("invalid buffer slice parameter offset");

        if (param->length+param->offset > source->length)
          TBX_FAILURE("invalid buffer slice parameter length");

        param->base = source->buffer;
        tbx_slist_append(slist, param);
      }

    source->parameter_slist = slist;
  }
  va_end(arg_list);

  if (link_mode == mad_link_mode_buffer)
    {
      /* B U F F E R   mode
	 -----------        */
      if (connection->delayed_send == tbx_true)
	TBX_FAILURE("cannot send data in buffer mode when delayed send is on");

      if (   (receive_mode == mad_receive_EXPRESS)
	  || (receive_mode == mad_receive_CHEAPER))
	{
	  if (   (send_mode == mad_send_SAFER)
	      || (send_mode == mad_send_CHEAPER))
	    {
	      if (buffer_mode == mad_buffer_mode_dynamic)
		{
		  interface->send_buffer(lnk, source);
                  free_source = tbx_true;
		  connection->flushed = tbx_true;
		}
	      else if (buffer_mode == mad_buffer_mode_static)
		{
		  if (tbx_empty_list(dest_list) || connection->flushed)
		    {
		      destination = interface->get_static_buffer(lnk);

		      tbx_append_list(dest_list, destination);
		    }
		  else
		    {
		      destination = tbx_get_list_object(dest_list);
		    }

		  do
		    {
		      if (mad_buffer_full(destination))
			{
			  interface->send_buffer(lnk, destination);
			  destination = interface->get_static_buffer(lnk);
			  tbx_append_list(dest_list, destination);
			}

		      mad_copy_buffer(source, destination);
		    }
		  while(mad_more_data(source));

                  free_source = tbx_true;

		  if (mad_buffer_full(destination))
		    {
		      interface->send_buffer(lnk, destination);
		      connection->flushed = tbx_true;
		    }
		  else
		    {
		      connection->flushed = tbx_false;
		    }

		  tbx_mark_list(dest_list);
		}
	      else
		TBX_FAILURE("unknown buffer mode");
	    }
	  else if (send_mode == mad_send_LATER)
	    TBX_FAILURE("send_LATER data cannot be sent in buffer mode");
	  else
	    TBX_FAILURE("unknown send mode");
	}
      else
	TBX_FAILURE("unknown receive mode");

      connection->last_link      = lnk;
      connection->last_link_mode = link_mode;
    }
  else if (link_mode == mad_link_mode_buffer_group)
    {
      /* C H U N K   mode
	 ---------        */
      if (   (receive_mode == mad_receive_EXPRESS)
	  || (receive_mode == mad_receive_CHEAPER))
	{
	  if (buffer_mode == mad_buffer_mode_dynamic)
	    {
	      if (send_mode == mad_send_SAFER)
		{
		  tbx_append_list(dest_list, mad_duplicate_buffer(source));
                  free_source = tbx_true;
		}
	      else if (   (send_mode == mad_send_LATER)
		       || (send_mode == mad_send_CHEAPER))
		{
		  tbx_append_list(dest_list, source);
		}
	      else
		TBX_FAILURE("unknown send mode");
	    }
	  else if (buffer_mode == mad_buffer_mode_static)
	    {
	      if (   (send_mode == mad_send_SAFER)
                  || (send_mode == mad_send_CHEAPER))
		{
		  if (tbx_empty_list(dest_list) || connection->flushed)
		    {
		      destination = interface->get_static_buffer(lnk);
		      tbx_append_list(dest_list, destination);
		    }
		  else
		    {
		      destination = tbx_get_list_object(dest_list);
		    }

		  do
		    {
		      if (mad_buffer_full(destination))
			{
			  destination = interface->get_static_buffer(lnk);
			  tbx_append_list(dest_list, destination);
			}

		      mad_copy_buffer(source, destination);
		    }
		  while(mad_more_data(source));

                  free_source = tbx_true;
		}
	      else if (send_mode == mad_send_LATER)
		{
		  if (tbx_empty_list(dest_list) || connection->flushed)
		    {
		      destination = interface->get_static_buffer(lnk);
		      tbx_append_list(dest_list, destination);
		    }
		  else
		    {
		      destination = tbx_get_list_object(dest_list);
		    }

		  do
		    {
		      if (mad_buffer_full(destination))
			{
			  destination = interface->get_static_buffer(lnk);
			  tbx_append_list(dest_list, destination);
			}

		      tbx_append_list(connection->pair_list,
				      mad_make_sub_buffer_pair(source,
							       destination));
		      mad_pseudo_copy_buffer(source, destination);
		    }
		  while(mad_more_data(source));

                  free_source = tbx_true;
		  connection->pair_list_used = tbx_true;
		}
	      else
		TBX_FAILURE("unknown send mode");
	    }
	  else
	    TBX_FAILURE("unknown buffer mode");
	}
      else
	TBX_FAILURE("unknown receive mode");

      connection->flushed        = tbx_false;
      connection->last_link      = lnk;
      connection->last_link_mode = link_mode;

      if (   (group_mode == mad_group_mode_split)
	  && (receive_mode == mad_receive_EXPRESS))
	{
	  if (connection->delayed_send == tbx_false)
	    {
	      mad_buffer_group_t buffer_group;

	      mad_make_buffer_group(&buffer_group, dest_list, lnk);
	      if (buffer_group.buffer_list.length == 1)
		{
		  interface->send_buffer(lnk,
					 buffer_group.buffer_list.first->object);
		}
	      else
		{
		  interface->send_buffer_group(lnk, &buffer_group);
		}
	    }
	  else
	    {
	      p_mad_buffer_group_t buffer_group;

	      buffer_group = mad_alloc_buffer_group_struct();
	      mad_make_buffer_group(buffer_group, dest_list, lnk);
	      tbx_append_list(buffer_group_list, buffer_group);
	    }

	  tbx_mark_list(dest_list);
	  connection->flushed = tbx_true;
	}
    }
  else if (link_mode == mad_link_mode_link_group)
    {
      /* G R O U P   mode
         ---------        */
      dest_list = lnk->buffer_list;

      if (receive_mode == mad_receive_CHEAPER)
	{
	  if (buffer_mode == mad_buffer_mode_static)
	    {
	      if (   (send_mode == mad_send_SAFER)
		  || (send_mode == mad_send_CHEAPER))
		{
		  if (tbx_empty_list(dest_list))
		    {
		      destination = interface->get_static_buffer(lnk);
		      tbx_append_list(dest_list, destination);
		    }
		  else
		    {
		      destination = tbx_get_list_object(dest_list);
		    }

		  do
		    {
		      if (mad_buffer_full(destination))
			{
			  destination = interface->get_static_buffer(lnk);
			  tbx_append_list(dest_list, destination);
			}
		      mad_copy_buffer(source, destination);
		    }
		  while(mad_more_data(source));

                  free_source = tbx_true;
		}
	      else if (send_mode == mad_send_LATER)
		{
		  if (tbx_empty_list(dest_list) || connection->flushed)
		    {
		      destination = interface->get_static_buffer(lnk);
		      tbx_append_list(dest_list, destination);
		    }
		  else
		    {
		      destination = tbx_get_list_object(dest_list);
		    }

		  do
		    {
		      if (mad_buffer_full(destination))
			{
			  destination = interface->get_static_buffer(lnk);
			  tbx_append_list(dest_list, destination);
			}

		      tbx_append_list(connection->pair_list,
				      mad_make_sub_buffer_pair(source,
							       destination));
		      mad_pseudo_copy_buffer(source, destination);
		    }
		  while(mad_more_data(source));

                  free_source = tbx_true;
		  connection->pair_list_used = tbx_true;
		}
	      else
		TBX_FAILURE("unknown send mode");
	    }
	  else if (buffer_mode == mad_buffer_mode_dynamic)
	    {
	      if (send_mode == mad_send_SAFER)
		{
		  tbx_append_list(dest_list, mad_duplicate_buffer(source));
                  free_source = tbx_true;
		}
	      else if (   (send_mode == mad_send_LATER)
		       || (send_mode == mad_send_CHEAPER))
		{
		  tbx_append_list(dest_list, source);
		}
	      else
		TBX_FAILURE("unknown send mode");
	    }
	  else
	    TBX_FAILURE("unknown buffer mode");
	}
      else if (receive_mode == mad_receive_EXPRESS)
	TBX_FAILURE("Express data cannot be sent in group mode");
      else
	TBX_FAILURE("unknown receive mode");
    }
  else
    TBX_FAILURE("unknown link mode");

  if (free_source) {
          mad_free_buffer_struct(source);
          source = NULL;
  }

  LOG_OUT();
}

void
mad_pack(p_mad_connection_t   connection,
	 void                *user_buffer,
	 size_t               user_buffer_length,
	 mad_send_mode_t      send_mode,
	 mad_receive_mode_t   receive_mode)
{
  mad_pack_ext(connection,
               user_buffer,
               user_buffer_length,
               send_mode,
               receive_mode, NULL);
}


/*
 * mad_unpack
 * ----------
 */
p_tbx_slist_t
mad_unpack_ext(p_mad_connection_t   connection,
               void                *user_buffer,
               size_t               user_buffer_length,
               mad_send_mode_t      send_mode,
               mad_receive_mode_t   receive_mode)
{
  p_mad_driver_interface_t   interface =
    connection->channel->adapter->driver->interface;
  p_mad_link_t               lnk               = NULL;
  mad_link_id_t              link_id           =   -1;
  mad_link_mode_t            link_mode         = mad_link_mode_unknown;
  mad_buffer_mode_t          buffer_mode       = mad_buffer_mode_unknown;
  mad_group_mode_t           group_mode        = mad_group_mode_unknown;
  tbx_bool_t                 last_delayed_send = tbx_false;
  p_mad_buffer_t             source            = NULL;
  p_mad_buffer_t             destination       = NULL;
  p_tbx_list_t               src_list          = connection->buffer_list;
  p_tbx_list_t               dest_list         = connection->user_buffer_list;
  p_tbx_list_reference_t     dest_ref  =
    connection->user_buffer_list_reference;
  p_tbx_slist_t              parameter_slist   = NULL;

  LOG_IN();
  if (!user_buffer_length)
    {
      LOG_OUT();

      return NULL;
    }

#if 0
  if ((unsigned long)user_buffer & (MAD_ALIGNMENT - 1))
    TBX_FAILURE("buffer is not aligned on MAD_ALIGNMENT bytes");

  if (user_buffer_length & (MAD_LENGTH_ALIGNMENT - 1))
    TBX_FAILURE("buffer length is not aligned on MAD_LENGTH_ALIGNMENT bytes");
#endif

  if (interface->choice)
    {
      lnk = interface->choice(connection,
			      user_buffer_length,
			      send_mode,
			      receive_mode);
    }
  else
    {
      lnk = connection->link_array[0];
    }

  link_id               = lnk->id;
  link_mode             = lnk->link_mode;
  buffer_mode           = lnk->buffer_mode;
  group_mode            = lnk->group_mode;
  last_delayed_send     = connection->delayed_send;

  if (   (send_mode    == mad_send_LATER)
      && (receive_mode == mad_receive_EXPRESS))
    {
      connection->delayed_send = tbx_true;
    }

  if (connection->delayed_send)
    {
      if (receive_mode == mad_receive_EXPRESS)
	{
	  link_mode = mad_link_mode_buffer_group;
	}
      else
	{
	  link_mode = mad_link_mode_link_group;
	}
    }
  else
    {
      if (   (send_mode    == mad_send_LATER)
	  && (receive_mode == mad_receive_CHEAPER))
	{
	  link_mode = mad_link_mode_link_group;
	}
      else
	{
	  link_mode = lnk->link_mode;

	  if ((receive_mode == mad_receive_EXPRESS)
	      && (link_mode == mad_link_mode_link_group))
	    {
	      link_mode = mad_link_mode_buffer_group;
	    }
	}
    }

  if (   (connection->last_link != NULL)
      && (link_mode != mad_link_mode_link_group)
      && (   (lnk != connection->last_link)
	  || (    (last_delayed_send != connection->delayed_send)
	       && (connection->last_link_mode == mad_link_mode_buffer))))
    {
      p_mad_link_t        last_link        = connection->last_link;
      mad_buffer_mode_t   last_buffer_mode = last_link->buffer_mode;

      if (connection->flushed == tbx_false)
	{
	  if (connection->last_link_mode == mad_link_mode_buffer_group)
	    {
	      if (last_buffer_mode == mad_buffer_mode_static)
		{
		  if (   !tbx_empty_list(dest_list)
		      && !tbx_reference_after_end_of_list(dest_ref))
		    {
		      if (   tbx_empty_list(src_list)
			  || !connection->more_data)
			{
			  interface->receive_buffer(lnk, &source);
			  tbx_append_list(src_list, source);
			}
		      else
			{
			  source = tbx_get_list_object(src_list);
			}

		      do
			{
			  destination =
			    tbx_get_list_reference_object(dest_ref);

			  do
			    {
			      if (!mad_more_data(source))
				{
				  interface->
				    return_static_buffer(lnk, source);
				  interface->
				    receive_buffer(last_link, &source);
				  tbx_append_list(src_list, source);
				}
			      mad_copy_buffer(source, destination);
			    }
			  while (!mad_buffer_full(destination));
			}
		      while (tbx_forward_list_reference(dest_ref));

		      interface->return_static_buffer(lnk, source);
		    }
		  else
		    {
		      if (connection->more_data)
			{
			  source = tbx_get_list_object(src_list);
			  interface->return_static_buffer(lnk, source);
			}
		    }
		}
	      else
		{
		  mad_buffer_group_t buffer_group;

		  mad_make_buffer_group(&buffer_group,
					src_list,
					last_link);

		  if (last_link->group_mode == mad_group_mode_split
		      && buffer_group.buffer_list.length == 1)
		    {
                      p_mad_buffer_t buffer = buffer_group.buffer_list.first->object;

		      interface->receive_buffer(last_link, &buffer);
		    }
		  else
		    {
		      interface->
			receive_sub_buffer_group(last_link,
						 connection->first_sub_buffer_group,
						 &buffer_group);
		    }
		}
	    }
	  else if (connection->last_link_mode == mad_link_mode_buffer)
	    {
	      interface->return_static_buffer(last_link,
					   tbx_get_list_object(src_list));
	    }
	  else
	    TBX_FAILURE("invalid link mode");

	  tbx_mark_list(src_list);
	  connection->more_data = tbx_false;
	  connection->flushed   = tbx_true ;
	}

      connection->first_sub_buffer_group = tbx_true;
    }

  destination = mad_get_user_receive_buffer(user_buffer, user_buffer_length);

  if (link_mode == mad_link_mode_buffer)
    {
      /* B U F F E R   mode
	 -----------        */
      if (connection->delayed_send == tbx_true)
	TBX_FAILURE("cannot receive data in buffer mode when delayed send is on");

      if (   (receive_mode == mad_receive_EXPRESS)
          || (receive_mode == mad_receive_CHEAPER))
	{
	  if (   (send_mode == mad_send_SAFER)
              || (send_mode == mad_send_CHEAPER))
	    {
	      if (buffer_mode == mad_buffer_mode_dynamic)
		{
		  interface->receive_buffer(lnk, &destination);
                  if (destination->parameter_slist)
                    {
                      if (receive_mode == mad_receive_EXPRESS)
                        {
                          parameter_slist = destination->parameter_slist;
                        }
                      else
                        {
                          if (!connection->parameter_slist)
                            {
                              connection->parameter_slist =
                                destination->parameter_slist;
                            }
                          else
                            {
                              tbx_slist_merge_after(connection->parameter_slist, parameter_slist);
                              tbx_slist_clear_and_free(parameter_slist);
                            }
                        }


                      destination->parameter_slist = NULL;
                    }

		  mad_free_buffer_struct(destination);
		  connection->flushed        = tbx_true;
		}
	      else if (buffer_mode == mad_buffer_mode_static)
		{
		  if (   (tbx_empty_list(src_list))
			 || (!connection->more_data))
		    {
		      interface->receive_buffer(lnk, &source);
		      tbx_append_list(src_list, source);
		    }
		  else
		    {
		      source = tbx_get_list_object(src_list);
		    }

		  do
		    {
		      if (!mad_more_data(source))
			{
			  interface->return_static_buffer(lnk, source);

			  interface->receive_buffer(lnk, &source);
			  tbx_append_list(src_list, source);
			}
		      mad_copy_buffer(source, destination);
		    }
		  while(!mad_buffer_full(destination));

		  if (mad_more_data(source))
		    {
		      connection->more_data = tbx_true;
		      connection->flushed   = tbx_false ;
		    }
		  else
		    {
		      connection->more_data = tbx_false;
		      connection->flushed   = tbx_true;

		      interface->return_static_buffer(lnk, source);

		      tbx_mark_list(src_list);
		    }

                  if (destination->parameter_slist)
                    {
                      if (receive_mode == mad_receive_EXPRESS)
                        {
                          parameter_slist = destination->parameter_slist;
                        }
                      else
                        {
                          if (!connection->parameter_slist)
                            {
                              connection->parameter_slist =
                                destination->parameter_slist;
                            }
                          else
                            {
                              tbx_slist_merge_after(connection->parameter_slist,
                                                    parameter_slist);
                              tbx_slist_clear_and_free(parameter_slist);
                            }
                        }

                      destination->parameter_slist = NULL;
                    }

		  mad_free_buffer_struct(destination);
		}
	      else
		TBX_FAILURE("unknown buffer mode");
	    }
	  else if (send_mode == mad_send_LATER)
	    TBX_FAILURE("send_LATER data cannot be received in buffer mode");
	  else
	    TBX_FAILURE("unknown send mode");
	}
      else
	TBX_FAILURE("unknown receive mode");

      connection->last_link      = lnk;
      connection->last_link_mode = link_mode;
    }
  else if (link_mode == mad_link_mode_buffer_group)
    {
      /* C H U N K   mode
	 ---------        */
      if (receive_mode == mad_receive_EXPRESS)
	{
	  if (   (   (send_mode == mad_send_SAFER)
	          || (send_mode == mad_send_CHEAPER)
	          || (send_mode == mad_send_LATER)))
	    {
	      if (buffer_mode == mad_buffer_mode_dynamic)
		{
		  mad_buffer_group_t buffer_group;

		  tbx_append_list(src_list, destination);
		  mad_make_buffer_group(&buffer_group, src_list, lnk);

		  if (lnk->group_mode == mad_group_mode_split
		      && buffer_group.buffer_list.length == 1)
		    {
                      p_mad_buffer_t buffer = buffer_group.
                        buffer_list.first->object;

		      interface->receive_buffer(lnk, &buffer);
		    }
		  else
		    {
		      interface->
			receive_sub_buffer_group(lnk,
						 connection->
						 first_sub_buffer_group,
						 &buffer_group);
		    }

		  connection->flushed                = tbx_true;
		  connection->more_data              = tbx_false;
		  if (group_mode == mad_group_mode_split)
		    {
		      connection->first_sub_buffer_group = tbx_true;
		    }
		  else if (group_mode == mad_group_mode_aggregate)
		    {
		      connection->first_sub_buffer_group = tbx_false;
		    }
		  else
		    TBX_FAILURE("unknown group mode");

		  tbx_mark_list(src_list);
		}
	      else if (buffer_mode == mad_buffer_mode_static)
		{
		  tbx_append_list(dest_list, destination);

		  if (   (tbx_empty_list(src_list))
		      || (!connection->more_data))
		    {
		      interface->receive_buffer(lnk, &source);
		      tbx_append_list(src_list, source);
		    }
		  else
		    {
		      source = tbx_get_list_object(src_list);
		    }

		  do
		    {
		      destination = tbx_get_list_reference_object(dest_ref);

		      do
			{
			  if (!mad_more_data(source))
			    {
			      interface->return_static_buffer(lnk, source);
			      interface->receive_buffer(lnk, &source);
			      tbx_append_list(src_list, source);
			    }
			  mad_copy_buffer(source, destination);
			}
		      while (!mad_buffer_full(destination));
		    }
		  while (tbx_forward_list_reference(dest_ref));

		  if (   mad_more_data(source)
                         && (group_mode == mad_group_mode_aggregate))
		    {
		      connection->more_data = tbx_true;
		      connection->flushed   = tbx_false;
		    }
		  else
		    {
		      interface->return_static_buffer(lnk, source);
		      tbx_mark_list(src_list);
		      connection->more_data = tbx_false;
		      connection->flushed   = tbx_true;
		    }
		}
	      else
		TBX_FAILURE("unknown buffer mode");

              if (destination->parameter_slist)
                {
                  parameter_slist = destination->parameter_slist;
                  destination->parameter_slist = NULL;
                }
	    }
	  else
	    TBX_FAILURE("unknown send mode");
	}
      else
	{
	  if (connection->delayed_send == tbx_true)
	    TBX_FAILURE("Cheaper data is not received in buffer_group mode "
		    "when delayed send is on");

	  if (   (send_mode == mad_send_SAFER)
              || (send_mode == mad_send_CHEAPER)
	      || (send_mode == mad_send_LATER))
	    {
	      if (buffer_mode == mad_buffer_mode_dynamic)
		{
		  tbx_append_list(src_list, destination);
		}
	      else if (buffer_mode == mad_buffer_mode_static)
		{
		  tbx_append_list(dest_list, destination);
		}
	    }
	  else
	    TBX_FAILURE("unknown send mode");

	  connection->flushed   = tbx_false;
	  connection->more_data = tbx_false;
	}

      connection->last_link       = lnk;
      connection->last_link_mode  = link_mode;
    }
  else if (link_mode == mad_link_mode_link_group)
    {
      /* G R O U P   mode
         ---------        */
      if (receive_mode == mad_receive_CHEAPER)
	{
	  if (   (send_mode == mad_send_SAFER)
	      || (send_mode == mad_send_CHEAPER)
	      || (send_mode == mad_send_LATER))
	    {
	      if (buffer_mode == mad_buffer_mode_dynamic)
		{
		  tbx_append_list(lnk->buffer_list, destination);
		}
	      else if (buffer_mode == mad_buffer_mode_static)
		{
		  tbx_append_list(lnk->user_buffer_list, destination);
		}
	      else
		TBX_FAILURE("unknown buffer mode");
	    }
	  else
	    TBX_FAILURE("unknown send mode");
	}
      else if (receive_mode == mad_receive_EXPRESS)
	TBX_FAILURE("Express data cannot be received in group mode");
      else
	TBX_FAILURE("unknown receive mode");
    }
  else
    TBX_FAILURE("unknown link mode");

  LOG_OUT();

  return parameter_slist;
}

void
mad_unpack(p_mad_connection_t  connection,
	   void               *user_buffer,
	   size_t              user_buffer_length,
	   mad_send_mode_t     send_mode,
	   mad_receive_mode_t  receive_mode)
{
  p_tbx_slist_t parameter_slist = NULL;

  parameter_slist = mad_unpack_ext(connection,
                                   user_buffer,
                                   user_buffer_length,
                                   send_mode,
                                   receive_mode);
  mad_free_parameter_slist(parameter_slist);
}

#endif /* CONFIG_SCHED_OPT */
