/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.
*/

/*
 * Mad_communication.c
 * ===================
 */
/* #define DEBUG */
#include <madeleine.h>

p_mad_connection_t
mad_begin_packing(p_mad_channel_t   channel,
		  mad_host_id_t     remote_host_id)
{
  p_mad_driver_interface_t   interface =
    &(channel->adapter->driver->interface);
  p_mad_connection_t         connection;
  mad_link_id_t              link_id;
  
  LOG_IN();

  TIME_INIT();
  
  PM2_LOCK_SHARED(channel);
  connection =
    &(channel->output_connection[remote_host_id]);
  
#ifdef PM2
  while (connection->lock == mad_true)
    {
      PM2_UNLOCK_SHARED(channel) ;
      PM2_YIELD();
      PM2_LOCK_SHARED(channel) ;
    }
#else /* PM2 */
  /* NOTE: the test and the affectation must be done
     atomically in a multithreaded environnement*/
  if (connection->lock == mad_true)
    FAILURE("mad_begin_packing: connection dead lock");      
#endif /* PM2 */
  connection->lock = mad_true;
  connection->send = mad_true;
  PM2_UNLOCK_SHARED(channel) ;

  interface->new_message(connection);
  
  /* structure initialisation */
  mad_list_init(&(connection->buffer_list));
  mad_list_init(&(connection->buffer_group_list));
  
  for(link_id = 0;
      link_id < connection->nb_link;
      link_id++)
    {
      mad_list_init(&(connection->link[link_id].buffer_list));
    }
  
  connection->pair_list_used = mad_false;
  connection->delayed_send   = mad_false;
  connection->flushed        = mad_true;
  connection->last_link      = NULL;

  LOG_PTR("mad_begin_packing: ", connection->specific);
  TIME("mad_begin_packing <--");
  LOG_OUT();
  return connection;
}

p_mad_connection_t
mad_begin_unpacking(p_mad_channel_t channel)
{
  p_mad_driver_interface_t   interface =
    &(channel->adapter->driver->interface);
  p_mad_connection_t         connection;
  mad_link_id_t              link_id;
  
  LOG_IN();
  TIME_INIT();
  
  PM2_LOCK_SHARED(channel);
#ifdef PM2
  while (channel->reception_lock == mad_true)
    {
      PM2_UNLOCK_SHARED(channel) ;
      marcel_yield();
      PM2_LOCK_SHARED(channel) ;
    }
#else /* PM2 */
  /* NOTE: the test and the affectation must be done
     atomically in a multithreaded environnement*/
  if (channel->reception_lock == mad_true)
    FAILURE("mad_begin_unpacking: reception dead lock");
#endif /* PM2 */
  channel->reception_lock = mad_true;
  PM2_UNLOCK_SHARED(channel);

  /* now we wait for an incoming communication */

  connection = interface->receive_message(channel);

  if (connection->lock == mad_true)
    FAILURE("connection dead lock");

   mad_list_init(&(connection->buffer_list));
   mad_list_init(&(connection->buffer_group_list));
   mad_list_init(&(connection->user_buffer_list));
   mad_list_reference_init(&(connection->user_buffer_list_reference),
			   &(connection->user_buffer_list));
   
  for(link_id = 0;
      link_id < connection->nb_link;
      link_id++)
    {
      mad_list_init(&(connection->link[link_id].buffer_list));
      mad_list_init(&(connection->link[link_id].user_buffer_list));
    }
  
  connection->lock            = mad_true;
  connection->send            = mad_false;
  connection->pair_list_used  = mad_false;
  connection->delayed_send    = mad_false;
  connection->flushed         = mad_true;
  connection->last_link       = NULL;
  connection->first_sub_buffer_group = mad_true;
  connection->more_data       = mad_false;

  TIME("mad_begin_unpacking <--");
  LOG_OUT();
  return connection;
}

void
mad_end_packing(p_mad_connection_t connection)
{
  mad_link_id_t              link_id;
  p_mad_driver_interface_t   interface =
    &(connection->channel->adapter->driver->interface);

  LOG_IN();
  LOG_PTR("mad_end_packing: ", connection->specific);
  TIME_INIT();
  
  if (connection->flushed == mad_false)
    {
      p_mad_link_t    last_link    = connection->last_link;
      p_mad_list_t    buffer_list  = &(connection->buffer_list);
  
     if (connection->delayed_send == mad_false)
	{
	  if (connection->last_link_mode == mad_link_mode_buffer)
	    {
	      p_mad_buffer_t buffer;

	      buffer = mad_get_list_object(buffer_list);
	      TIME("    send buffer -->");
	      interface->send_buffer(last_link, buffer);
	      TIME("    send buffer <--");
	    }
	  else if (connection->last_link_mode == mad_link_mode_buffer_group)
	    {
	      mad_buffer_group_t buffer_group;
	      
	      mad_make_buffer_group(&buffer_group,
				    buffer_list,
				    last_link);
	      interface->send_buffer_group(last_link, &buffer_group);
	    }
	  else
	    FAILURE("invalid link mode");
	}
      else
	{
	  if (connection->last_link_mode == mad_link_mode_buffer_group)
	    {
	      p_mad_buffer_group_t buffer_group;
	      	      
	      buffer_group = mad_alloc_buffer_group_struct();
	      mad_make_buffer_group(buffer_group,
				    buffer_list,
				    last_link);
	      mad_append_list(&(connection->buffer_group_list), buffer_group);
	    }
	  else
	    FAILURE("invalid link mode");
	}

      connection->flushed = mad_true;      
    }

  /* buffer pair handling */
  if (connection->pair_list_used)
    {
      if (!mad_empty_list(&(connection->pair_list)))
	{
	  mad_list_reference_t ref;

	  mad_list_reference_init(&ref, &(connection->pair_list));

	  do
	    {
	      p_mad_buffer_pair_t pair ;

	      pair = mad_get_list_reference_object(&ref);
	      mad_copy_buffer(&(pair->dynamic_buffer),
			      &(pair->static_buffer));
	    }
	  while (mad_forward_list_reference(&ref));

	  mad_foreach_destroy_list(&(connection->pair_list),
				   mad_foreach_free_buffer_pair_struct);
	}
    }

  /* residual buffer_group flushing */
  if (!mad_empty_list(&(connection->buffer_group_list)))
    {
      mad_list_reference_t list_ref;

      mad_list_reference_init(&list_ref,
			      &(connection->buffer_group_list));

      do
	{
	  p_mad_buffer_group_t buffer_group ;

	  buffer_group = mad_get_list_reference_object(&list_ref);
	  interface->send_buffer_group(buffer_group->link, buffer_group);
	}
      while (mad_forward_list_reference(&list_ref));
    }
  
  /* link data groups flushing */
  for (link_id = 0;
       link_id < connection->nb_link;
       link_id++)
    {
      p_mad_link_t link = &(connection->link[link_id]);
      
      if (!mad_empty_list(&(link->buffer_list)))
	{
	  mad_buffer_group_t buffer_group;
	  
	  mad_make_buffer_group(&buffer_group,
				&(link->buffer_list),
				link);
	  interface->send_buffer_group(link, &buffer_group);
	  }
    }
  
  mad_foreach_destroy_list(&(connection->buffer_group_list),
			   mad_foreach_free_buffer_group_struct);
  mad_foreach_destroy_list(&(connection->buffer_list),
			   mad_foreach_free_buffer);
  for (link_id = 0;
       link_id < connection->nb_link;
       link_id++)
    {
	mad_foreach_destroy_list(&(connection->link[link_id].buffer_list),
				 mad_foreach_free_buffer);
    }
  
  connection->lock = mad_false; 
  TIME("mad_end_packing <--");
  LOG_OUT();
}

void
mad_end_unpacking(p_mad_connection_t connection)
{
  p_mad_driver_interface_t   interface =
    &(connection->channel->adapter->driver->interface);
  p_mad_buffer_t             source;
  p_mad_buffer_t             destination;
  p_mad_list_t               src_list  = NULL;
  p_mad_list_t               dest_list = NULL;
  p_mad_list_reference_t     dest_ref  = NULL;
  mad_link_id_t              link_id;
  
  LOG_IN();
  TIME_INIT();
  if (connection->flushed == mad_false)
    {
      p_mad_link_t        last_link        = connection->last_link;
      mad_buffer_mode_t   last_buffer_mode = last_link->buffer_mode;

      src_list  = &(connection->buffer_list);
      dest_list = &(connection->user_buffer_list);
      dest_ref  = &(connection->user_buffer_list_reference);

      if (connection->last_link_mode == mad_link_mode_buffer_group)
	{
	  if (last_buffer_mode == mad_buffer_mode_static)
	    {
	      if (   !mad_empty_list(dest_list)
		  && !mad_reference_after_end_of_list(dest_ref))
		{
		  if (   mad_empty_list(src_list)
		      || !connection->more_data)
		    {
		      interface->receive_buffer(last_link, &source);
		      mad_append_list(src_list, source);
		    }
		  else
		    {
		      source = mad_get_list_object(src_list);
		    }

		  do
		    {
		      destination = mad_get_list_reference_object(dest_ref);
		      
		      do
			{
			  if (!mad_more_data(source))
			    {
			      interface->
				return_static_buffer(last_link, source);
			      interface->
				receive_buffer(last_link, &source);
			      mad_append_list(src_list, source);
			    }
			  mad_copy_buffer(source, destination);
			}
		      while (!mad_buffer_full(destination));
		    }
		  while (mad_forward_list_reference(dest_ref));
		  
		  interface->return_static_buffer(last_link, source);
		}
	      else
		{
		  if (connection->more_data)
		    {
		      source = mad_get_list_object(src_list);
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
	      interface->
		receive_sub_buffer_group(last_link,
					 connection->first_sub_buffer_group,
					 &buffer_group);
	    }
	}
      else if (connection->last_link_mode == mad_link_mode_buffer)
	{
	  interface->return_static_buffer(last_link,
					  mad_get_list_object(src_list));
	}
      else
	FAILURE("invalid link mode");

      mad_mark_list(src_list);
      connection->more_data              = mad_false;
      connection->flushed                = mad_true;
      connection->first_sub_buffer_group = mad_false;
    }

  /* link data groups reception */
  for (link_id = 0;
       link_id < connection->nb_link;
       link_id++)
    {
      /* A buffer_group must be created from the buffers */
      p_mad_link_t           link = &(connection->link[link_id]);
      mad_list_reference_t   destination_list_reference;

      src_list  = &(link->buffer_list);
      dest_list = &(link->user_buffer_list);
      dest_ref  = &destination_list_reference;
      mad_list_reference_init(dest_ref, dest_list);

      if (link->buffer_mode == mad_static_buffer)
	{	    
	  if (!mad_empty_list(dest_list))
	    {
	      interface->receive_buffer(link, &source);
	      mad_append_list(src_list, source);
	      
	      do
		{
		  destination = mad_get_list_reference_object(dest_ref);
		    
		  while (!mad_buffer_full(destination))
		    {
		      if (!mad_more_data(source))
			{
			  interface->return_static_buffer(link, source);
			  interface->receive_buffer(link, &source);
			  mad_append_list(src_list, source);
			} /* !mad_more_data(source) */
		      mad_copy_buffer(source, destination);
		    }
		}
	      while(mad_forward_list_reference(dest_ref));
		
	      interface->return_static_buffer(link, source);
	    } /* !mad_empty_list(dest_list) */
	} /* link->buffer_mode == mad_static_buffer */
      else
	{
	  if (!mad_empty_list(src_list))
	    {
	      mad_buffer_group_t buffer_group;
	      
	      mad_make_buffer_group(&buffer_group, src_list, link);
	      interface->
		receive_sub_buffer_group(link, mad_true, &buffer_group);
	    }
	}
    }

  mad_foreach_destroy_list(&(connection->buffer_list),
			   mad_foreach_free_buffer);
  mad_foreach_destroy_list(&(connection->user_buffer_list),
			   mad_foreach_free_buffer);
  for (link_id = 0 ;
       link_id < connection->nb_link ;
       link_id++)
    {
      mad_foreach_destroy_list(&(connection->link[link_id].buffer_list),
			       mad_foreach_free_buffer);
      mad_foreach_destroy_list(&(connection->link[link_id].user_buffer_list),
			       mad_foreach_free_buffer);
    }

  connection->lock = mad_false;
  connection->channel->reception_lock = mad_false;
  TIME("mad_end_unpacking <--");
  LOG_OUT();
}

/* 
 * mad_pack
 * --------
 */
void
mad_pack(p_mad_connection_t   connection,
	 void                *user_buffer,
	 size_t               user_buffer_length,
	 mad_send_mode_t      send_mode,
	 mad_receive_mode_t   receive_mode)
{
  p_mad_driver_interface_t   interface         =
    &(connection->channel->adapter->driver->interface);
  p_mad_link_t               link;
  mad_link_id_t              link_id;
  mad_link_mode_t            link_mode;
  mad_buffer_mode_t          buffer_mode ;
  mad_bool_t                 last_delayed_send;
  p_mad_buffer_t             source            = NULL;
  p_mad_buffer_t             destination       = NULL;
  p_mad_list_t               dest_list         = &(connection->buffer_list);
  p_mad_list_t               buffer_group_list =
    &(connection->buffer_group_list);
  
  LOG_IN();
  LOG_PTR("mad_pack (1): ", connection->specific);
  TIME_INIT();
  
  if (!connection->lock)
    FAILURE("invalid connection object");

  if (user_buffer_length == 0) return;

  link = interface->choice(connection,
			   user_buffer_length,
			   send_mode,
			   receive_mode);
  link_id               = link->id;
  LOG("mad_pack: 1");
  link_mode             = link->link_mode;
  buffer_mode           = link->buffer_mode;
  last_delayed_send     = connection->delayed_send;

  if (      (send_mode == mad_send_LATER)
      && (receive_mode == mad_receive_EXPRESS))
    {
      connection->delayed_send = mad_true;
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
	  link_mode = link->link_mode;

	  if (   (receive_mode == mad_receive_EXPRESS)
	      && (link_mode    == mad_link_mode_link_group))
	    {
	      link_mode = mad_link_mode_buffer_group;
	    }
	}
    }

  LOG("mad_pack: 2");
  if (   (connection->last_link != NULL)
      && (link_mode != mad_link_mode_link_group)
      && (   (link != connection->last_link)
	  || (    (last_delayed_send != connection->delayed_send)
	       && (connection->last_link_mode == mad_link_mode_buffer)))
      && (connection->flushed == mad_false))
    {
      p_mad_link_t    last_link      = connection->last_link;

      if (last_delayed_send == mad_false)
	{
	  if (connection->last_link_mode == mad_link_mode_buffer)
	    {
	      TIME("    send buffer -->");
	      interface->
		send_buffer(last_link, mad_get_list_object(dest_list));
	      TIME("    send buffer <--");
	    }
	  else if (connection->last_link_mode ==
		   mad_link_mode_buffer_group)
	    {
	      mad_buffer_group_t buffer_group;
	      
	      mad_make_buffer_group(&buffer_group, dest_list, last_link);
	      interface->send_buffer_group(last_link, &buffer_group);
	    }
	  else
	    FAILURE("invalid link mode");
	}
      else
	{
	  if (connection->last_link_mode == mad_link_mode_buffer_group)
	    {
	      p_mad_buffer_group_t buffer_group;

	      buffer_group = mad_alloc_buffer_group_struct();
	      mad_make_buffer_group(buffer_group, dest_list, last_link);
	      mad_append_list(buffer_group_list, buffer_group);
	    }
	  else
	    FAILURE("invalid link mode");
	}

      connection->flushed = mad_true;
      mad_mark_list(dest_list);
    }

  source = mad_get_user_send_buffer(user_buffer, user_buffer_length);

  LOG("mad_pack: 3");
  if (link_mode == mad_link_mode_buffer)
    {
      /* B U F F E R   mode
	 -----------        */      
      if (connection->delayed_send == mad_true)
	FAILURE("cannot send data in buffer mode when delayed send is on");

      if (   (receive_mode == mad_receive_EXPRESS)
	  || (receive_mode == mad_receive_CHEAPER))
	{
	  if (   (send_mode == mad_send_SAFER)
	      || (send_mode == mad_send_CHEAPER))
	    {
	      if (buffer_mode == mad_buffer_mode_dynamic)
		{
		  TIME("    send buffer -->");
		  interface->send_buffer(link, source);
		  TIME("    send buffer <--");
		  mad_free_buffer_struct(source);		  
		  connection->flushed = mad_true;
		}
	      else if (buffer_mode == mad_buffer_mode_static)
		{		  
		  if (mad_empty_list(dest_list) || connection->flushed)
		    {
		      destination = interface->get_static_buffer(link);
		  
		      mad_append_list(dest_list, destination);
		    } /* mad_empty_list(dest_list) */
		  else
		    {
		      destination = mad_get_list_object(dest_list);
		    } /* mad_empty_list(dest_list) */
	      
		  do
		    {
		      if (mad_buffer_full(destination))
			{  
			  TIME("    send buffer -->");
			  interface->send_buffer(link, destination);
			  TIME("    send buffer <--");
			  destination = interface->get_static_buffer(link);
			  mad_append_list(dest_list, destination);
			} /* mad_buffer_full(destination) */
		  
		      mad_copy_buffer(source, destination);  
		    }
		  while(mad_more_data(source));
	      
		  if (mad_buffer_full(destination))
		    {  
		      TIME("    send buffer -->");
		      interface->send_buffer(link, destination);
		      TIME("    send buffer <--");
		      connection->flushed = mad_true;
		    }
		  else
		    {
		      connection->flushed = mad_false;
		    }

		  mad_mark_list(dest_list);
		}
	      else
		FAILURE("unknown buffer mode");
	    }
	  else if (send_mode == mad_send_LATER)
	    FAILURE("send_LATER data cannot be sent in buffer mode");
	  else
	    FAILURE("unknown send mode");
	}
      else
	FAILURE("unknown receive mode");

      connection->last_link      = link;
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
		  mad_append_list(dest_list, mad_duplicate_buffer(source));
		}
	      else if (   (send_mode == mad_send_LATER)
		       || (send_mode == mad_send_CHEAPER))
		{
		  mad_append_list(dest_list, source);
		}
	      else	    
		FAILURE("unknown send mode");
	    }
	  else if (buffer_mode == mad_buffer_mode_static)
	    {
	      if (   (send_mode == mad_send_SAFER)
		     || (send_mode == mad_send_CHEAPER))
		{
		  if (mad_empty_list(dest_list) || connection->flushed)
		    {
		      destination = interface->get_static_buffer(link);
		      mad_append_list(dest_list, destination);
		    } /* mad_empty_list(dest_list) */
		  else
		    {
		      destination = mad_get_list_object(dest_list);
		    } /* mad_empty_list(dest_list) */
	      
		  do
		    {
		      if (mad_buffer_full(destination))
			{  
			  destination = interface->get_static_buffer(link);
			  mad_append_list(dest_list, destination);
			} /* mad_buffer_full(destination) */
		  
		      mad_copy_buffer(source, destination);  
		    }
		  while(mad_more_data(source));
		}
	      else if (send_mode == mad_send_LATER)
		{
		  if (mad_empty_list(dest_list) || connection->flushed)
		    {
		      destination = interface->get_static_buffer(link);
		      mad_append_list(dest_list, destination);
		    } /* mad_empty_list(dest_list) */
		  else
		    {
		      destination = mad_get_list_object(dest_list);
		    } /* mad_empty_list(dest_list) */
	      
		  do
		    {
		      if (mad_buffer_full(destination))
			{  
			  destination = interface->get_static_buffer(link);
			  mad_append_list(dest_list, destination);
			} /* mad_buffer_full(destination) */
		  
		      mad_append_list(&(connection->pair_list),
				      mad_make_sub_buffer_pair(source,
							       destination));
		      mad_pseudo_copy_buffer(source, destination);  
		    }
		  while(mad_more_data(source));	      
		  connection->pair_list_used = mad_true;
		}
	      else
		FAILURE("unknown send mode");
	    }
	  else
	    FAILURE("unknown buffer mode");
	}
      else
	FAILURE("unknown receive mode");

      connection->flushed        = mad_false;
      connection->last_link      = link;
      connection->last_link_mode = link_mode;
    }
  else if (link_mode == mad_link_mode_link_group)
    {
      /* G R O U P   mode
         ---------        */      
      dest_list = &(link->buffer_list);

      if (receive_mode == mad_receive_CHEAPER)
	{
	  if (buffer_mode == mad_buffer_mode_static)
	    {
	      if (   (send_mode == mad_send_SAFER)
		  || (send_mode == mad_send_CHEAPER))
		{
		  if (mad_empty_list(dest_list))
		    {
		      destination =
			interface->get_static_buffer(link);
		      mad_append_list(dest_list, destination);
		    } /* mad_empty_list(dest_list) */
		  else
		    {
		      destination = mad_get_list_object(dest_list);
		    } /* mad_empty_list(dest_list) */
	      
		  do
		    {
		      if (mad_buffer_full(destination))
			{  
			  destination = interface->get_static_buffer(link);
			  mad_append_list(dest_list, destination);
			} /* mad_buffer_full(destination) */
		      mad_copy_buffer(source, destination);  
		    }
		  while(mad_more_data(source));
		}
	      else if (send_mode == mad_send_LATER)
		{
		  if (mad_empty_list(dest_list) || connection->flushed)
		    {
		      destination = interface->get_static_buffer(link);
		      mad_append_list(dest_list, destination);
		    } /* mad_empty_list(dest_list) */
		  else
		    {
		      destination = mad_get_list_object(dest_list);
		    } /* mad_empty_list(dest_list) */
	      
		  do
		    {
		      if (mad_buffer_full(destination))
			{  
			  destination = interface->get_static_buffer(link);
			  mad_append_list(dest_list, destination);
			} /* mad_buffer_full(destination) */
		  
		      mad_append_list(&(connection->pair_list),
				      mad_make_sub_buffer_pair(source,
							       destination));
		      mad_pseudo_copy_buffer(source, destination);  
		    }
		  while(mad_more_data(source));
		  connection->pair_list_used = mad_true;
		}
	      else
		FAILURE("unknown send mode");
	    }
	  else if (buffer_mode == mad_buffer_mode_dynamic)
	    {
	      if (send_mode == mad_send_SAFER)
		{
		  mad_append_list(dest_list, mad_duplicate_buffer(source));
		}
	      else if (   (send_mode == mad_send_LATER)
		       || (send_mode == mad_send_CHEAPER))
		{
		  mad_append_list(dest_list, source);
		}
	      else
		FAILURE("unknown send mode");
	    }	 
	  else
	    FAILURE("unknown buffer mode"); 
	}
      else if (receive_mode == mad_receive_EXPRESS)
	FAILURE("Express data cannot be sent in group mode");
      else
	FAILURE("unknown receive mode");
    }
  else
    FAILURE("unknown link mode");

  LOG("mad_pack: 4");
  LOG_PTR("mad_pack (2): ", connection->specific);
  TIME("mad_pack <--");
  LOG_OUT();
}

/*
 * mad_unpack
 * ----------
 */
void
mad_unpack(p_mad_connection_t   connection,
	   void                *user_buffer,
	   size_t               user_buffer_length,
	   mad_send_mode_t      send_mode,
	   mad_receive_mode_t   receive_mode)
{
  p_mad_driver_interface_t   interface =
    &(connection->channel->adapter->driver->interface);
  p_mad_link_t               link;
  mad_link_id_t              link_id;
  mad_link_mode_t            link_mode;
  mad_buffer_mode_t          buffer_mode;
  mad_bool_t                 last_delayed_send;
  p_mad_buffer_t             source;
  p_mad_buffer_t             destination;
  p_mad_list_t               src_list  = &(connection->buffer_list);
  p_mad_list_t               dest_list = &(connection->user_buffer_list);
  p_mad_list_reference_t     dest_ref  =
    &(connection->user_buffer_list_reference);

  LOG_IN();
  TIME_INIT();

  if (!connection->lock)
    FAILURE("invalid connection object");

  if (user_buffer_length == 0) return;

  link = interface->choice(connection,
			   user_buffer_length,
			   send_mode,
			   receive_mode);
  link_id               = link->id;
  link_mode             = link->link_mode;
  buffer_mode           = link->buffer_mode;
  last_delayed_send     = connection->delayed_send;

  if (   (send_mode    == mad_send_LATER)
      && (receive_mode == mad_receive_EXPRESS))
    {
      connection->delayed_send = mad_true;
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
	  link_mode = link->link_mode;

	  if ((receive_mode == mad_receive_EXPRESS)
	      && (link_mode == mad_link_mode_link_group))
	    {
	      link_mode = mad_link_mode_buffer_group;
	    }
	}
    }
  
  if (   (connection->last_link != NULL)
      && (link_mode != mad_link_mode_link_group)
      && (   (link != connection->last_link)
	  || (    (last_delayed_send != connection->delayed_send)
	       && (connection->last_link_mode == mad_link_mode_buffer))))
    {
      p_mad_link_t        last_link        = connection->last_link;
      mad_buffer_mode_t   last_buffer_mode = last_link->buffer_mode;

      LOG("mad_unpack: 1");
      if (connection->flushed == mad_false)
	{
	  if (connection->last_link_mode == mad_link_mode_buffer_group)
	    {
	      if (last_buffer_mode == mad_buffer_mode_static)
		{
		  if (   !mad_empty_list(dest_list)
		      && !mad_reference_after_end_of_list(dest_ref))
		    {
		      if (   mad_empty_list(src_list)
			  || !connection->more_data)
			{
			  interface->receive_buffer(link, &source);
			  mad_append_list(src_list, source);
			}
		      else
			{
			  source = mad_get_list_object(src_list);
			}

		      do
			{
			  destination =
			    mad_get_list_reference_object(dest_ref);
		      
			  do
			    {
			      if (!mad_more_data(source))
				{
				  interface->
				    return_static_buffer(link, source);
				  interface->
				    receive_buffer(last_link, &source);
				  mad_append_list(src_list, source);
				}
			      mad_copy_buffer(source, destination);
			    }
			  while (!mad_buffer_full(destination));
			}
		      while (mad_forward_list_reference(dest_ref));

		      interface->return_static_buffer(link, source);
		    }
		  else
		    {
		      if (connection->more_data)
			{
			  source = mad_get_list_object(src_list);
			  interface->return_static_buffer(link, source);
			}
		    }	      
		}
	      else
		{
		  mad_buffer_group_t buffer_group;
		  
		  mad_make_buffer_group(&buffer_group,
					src_list,
					last_link);
		  interface->
		    receive_sub_buffer_group(last_link,
					     connection->
					     first_sub_buffer_group,
					    &buffer_group);
		}
	    }
	  else if (connection->last_link_mode == mad_link_mode_buffer)
	    {
	      interface->return_static_buffer(last_link,
					   mad_get_list_object(src_list));
	    }
	  else
	    FAILURE("invalid link mode");
	  
	  LOG("mad_unpack: data flushed");
	  mad_mark_list(src_list);
	  connection->more_data = mad_false;
	  connection->flushed   = mad_true ;
	}

      connection->first_sub_buffer_group = mad_true;
      LOG("mad_unpack: 2");
    }
  
  destination = mad_get_user_receive_buffer(user_buffer, user_buffer_length);
  LOG_PTR("mad_unpack: user buffer:", user_buffer);
  LOG_VAL("mad_unpack: user buffer length: ", user_buffer_length);
  
  if (link_mode == mad_link_mode_buffer)
    {
      /* B U F F E R   mode
	 -----------        */
      if (connection->delayed_send == mad_true)
	FAILURE("cannot receive data in buffer mode when delayed send is on");

      if (   (receive_mode == mad_receive_EXPRESS)
	  || (receive_mode == mad_receive_CHEAPER))
	{
	  if (   (send_mode == mad_send_SAFER)
	      || (send_mode == mad_send_CHEAPER))
	    {
	      if (buffer_mode == mad_buffer_mode_dynamic)
		{
		  LOG("mad_unpack: 3");
		  TIME("    receive buffer -->");
		  interface->receive_buffer(link, &destination);
		  TIME("    receive buffer <--");
		  connection->flushed        = mad_true;
		  LOG("mad_unpack: 4");
		}
	      else if (buffer_mode == mad_buffer_mode_static)
		{
		  if (   (mad_empty_list(src_list))
			 || (!connection->more_data))
		    {
		      TIME("    receive buffer -->");
		      interface->receive_buffer(link, &source);
		      TIME("    receive buffer <--");
		      mad_append_list(src_list, source);
		    }
		  else
		    {
		      source = mad_get_list_object(src_list);
		    }
	      
		  do
		    {
		      if (!mad_more_data(source))
			{
			  interface->return_static_buffer(link, source);
		      
			  TIME("    receive buffer -->");
			  interface->receive_buffer(link, &source);
			  TIME("    receive buffer <--");
			  mad_append_list(src_list, source);
			}
		      mad_copy_buffer(source, destination);
		    }
		  while(!mad_buffer_full(destination));
	      
		  if (mad_more_data(source))
		    {
		      connection->more_data = mad_true;
		      connection->flushed   = mad_false ;
		    }
		  else
		    {
		      connection->more_data = mad_false;
		      connection->flushed   = mad_true;

		      interface->return_static_buffer(link, source);
		  
		      mad_mark_list(src_list);
		    }
		}
	      else
		FAILURE("unknown buffer mode");
	    }
	  else if (send_mode == mad_send_LATER)
	    FAILURE("send_LATER data cannot be receive in buffer mode");
	  else
	    FAILURE("unknown send mode");
	}
      else
	FAILURE("unknown receive mode");

      connection->last_link      = link;
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

		  mad_append_list(src_list, destination);
		  mad_make_buffer_group(&buffer_group, src_list, link);
		  interface->
		    receive_sub_buffer_group(link,
					     connection->
					     first_sub_buffer_group,
					     &buffer_group);
		  
		  connection->flushed                = mad_true;
		  connection->more_data              = mad_false;
		  connection->first_sub_buffer_group = mad_false;
		  mad_mark_list(src_list);
		}
	      else if (buffer_mode == mad_buffer_mode_static)
		{
		  LOG("mad_unpack: static buffer buffer_group");
		  mad_append_list(dest_list, destination);
		  
		  if (   (mad_empty_list(src_list))
		      || (!connection->more_data))
		    {
		      interface->receive_buffer(link, &source);
		      mad_append_list(src_list, source);
		    }
		  else
		    {
		      source = mad_get_list_object(src_list);
		    }

		  do
		    {
		      destination = mad_get_list_reference_object(dest_ref);
		      
		      do
			{
			  if (!mad_more_data(source))
			    {
			      interface->return_static_buffer(link, source);
			      interface->receive_buffer(link, &source);
			      mad_append_list(src_list, source);
			    }
			  mad_copy_buffer(source, destination);
			}
		      while (!mad_buffer_full(destination));
		    }
		  while (mad_forward_list_reference(dest_ref));

		  if (mad_more_data(source))
		    {
		      connection->more_data = mad_true;
		      connection->flushed   = mad_false; 
		    }
		  else
		    {
		      interface->return_static_buffer(link, source);
		      mad_mark_list(src_list);
		      connection->more_data = mad_false;
		      connection->flushed   = mad_true; 
		    }
		}
	      else 
		FAILURE("unknown buffer mode");
	    }
	  else
	    FAILURE("unknown send mode");
	}
      else
	{
	  if (connection->delayed_send == mad_true)
	    FAILURE("Cheaper data is not received in buffer_group mode "
		    "when delayed send is on");

	  if (   (send_mode == mad_send_SAFER)
              || (send_mode == mad_send_CHEAPER)
	      || (send_mode == mad_send_LATER))
	    {
	      if (buffer_mode == mad_buffer_mode_dynamic)
		{
		  mad_append_list(src_list, destination);
		}
	      else if (buffer_mode == mad_buffer_mode_static)
		{
		  mad_append_list(dest_list, destination);
		}
	    }
	  else
	    FAILURE("unknown send mode");

	  connection->flushed   = mad_false;	      
	  connection->more_data = mad_false;
	}

      connection->last_link       = link;
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
		  mad_append_list(&(link->buffer_list), destination);
		}
	      else if (buffer_mode == mad_buffer_mode_static)
		{
		  mad_append_list(&(link->user_buffer_list), destination);
		}
	      else
		FAILURE("unknown buffer mode");
	    }
	  else 
	    FAILURE("unknown send mode");
	}
      else if (receive_mode == mad_receive_EXPRESS)
	FAILURE("Express data cannot be received in group mode");
      else 
	FAILURE("unknown receive mode");
    }
  else
    FAILURE("unknown link mode");
  TIME("mad_unpack <--");
  LOG_OUT();
}
