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

______________________________________________________________________________
$Log: mad_communication.c,v $
Revision 1.22  2001/01/16 09:55:21  oaumage
- integration du mecanisme de forwarding
- modification de l'usage des flags

Revision 1.21  2000/06/07 11:26:52  oaumage
- Fin du retablissement de la version de mardi matin

Revision 1.20  2000/06/07 08:12:04  oaumage
- Retour a des bases saines

Revision 1.19  2000/06/07 07:57:14  oaumage
- retour a la version 1.16

Revision 1.16  2000/05/18 14:36:56  oaumage
- nettoyage du code

Revision 1.15  2000/04/20 13:32:06  oaumage
- modifications diverses

Revision 1.14  2000/03/27 08:50:53  oaumage
- pre-support decoupage de groupes
- correction au niveau du support du demarrage manuel

Revision 1.13  2000/03/15 09:59:01  oaumage
- renommage du polling Nexus
- correction d'un probleme de deverrouillage au niveau
  de la fonction de polling, en cas d'echec du polling

Revision 1.12  2000/03/08 17:19:14  oaumage
- support de compilation avec Marcel sans PM2
- pre-support de packages de Threads != Marcel
- utilisation de TBX_MALLOC

Revision 1.11  2000/03/02 15:45:46  oaumage
- support du polling Nexus

Revision 1.10  2000/03/02 14:25:06  oaumage
- optimisation: envoi d'un buffer lorsqu'un groupe est de taille 1,
  et le mode est split

Revision 1.9  2000/02/28 11:06:14  rnamyst
Changed #include "" into #include <>.

Revision 1.8  2000/02/08 17:49:06  oaumage
- support de la net toolbox

Revision 1.7  2000/01/13 14:45:57  oaumage
- adaptation pour la prise en compte de la toolbox
- suppression des fichiers redondant
- mad_channel.c, madeleine.c: amelioration des fonctions `par defaut' au niveau
  du support des drivers

Revision 1.6  2000/01/10 10:23:03  oaumage
*** empty log message ***

Revision 1.5  2000/01/05 09:42:58  oaumage
- mad_communication.c: support du nouveau `group_mode' (a surveiller !)
- madeleine.c: external_spawn_init n'est plus indispensable
- mad_list_management: rien de particulier, mais l'ajout d'une fonction
  d'acces a la longueur de la liste est a l'etude, pour permettre aux drivers
  de faire une optimisation simple au niveau des groupes de buffers de
  taille 1

Revision 1.4  2000/01/04 16:49:10  oaumage
- madeleine: corrections au niveau du support `external spawn'
- support des fonctions non definies dans les drivers

Revision 1.3  1999/12/15 17:31:27  oaumage
Ajout de la commande de logging de CVS

______________________________________________________________________________
*/

/*
 * Mad_communication.c
 * ===================
 */
/* #define DEBUG */
/* #define TRACING */
#include "madeleine.h"



#ifdef MAD_FORWARDING
p_mad_connection_t
mad_begin_packing(p_mad_user_channel_t   user_channel,
		  ntbx_host_id_t         remote_host_id)
#else // MAD_FORWARDING
p_mad_connection_t
mad_begin_packing(p_mad_channel_t   channel,
		  ntbx_host_id_t    remote_host_id)
#endif //MAD_FORWARDING

{
  p_mad_driver_interface_t   interface;
  p_mad_connection_t         connection;
  mad_link_id_t              link_id;
#ifdef MAD_FORWARDING
  p_mad_channel_t            channel;
  ntbx_host_id_t             real_remote_host_id;
#endif //MAD_FORWARDING

  
  LOG_IN();
  TRACE("New emission request");

#ifdef MAD_FORWARDING
  channel = mad_find_channel(remote_host_id,
			     user_channel,
			     &real_remote_host_id);

#endif //MAD_FORWARDING

  interface = &(channel->adapter->driver->interface);

  TBX_LOCK_SHARED(channel);
#ifdef MAD_FORWARDING
  connection =
    &(channel->output_connection[real_remote_host_id]);
#else //MAD_FORWARDING
  connection =
    &(channel->output_connection[remote_host_id]);
#endif //MAD_FORWARDING
  
#ifdef MARCEL
  while (connection->lock == tbx_true)
    {
      TBX_UNLOCK_SHARED(channel) ;
      TBX_YIELD();
      TBX_LOCK_SHARED(channel) ;
    }
#else /* MARCEL */
  if (connection->lock == tbx_true)
    FAILURE("mad_begin_packing: connection dead lock");      
#endif /* MARCEL */

  connection->lock = tbx_true;
  connection->send = tbx_true;
  TBX_UNLOCK_SHARED(channel) ;

#ifdef MAD_FORWARDING
  if(channel->is_forward)
    {
      interface = fwd_interface;
      connection->is_being_forwarded = tbx_true;
      connection->actual_destination_id = remote_host_id;
    }
#endif //MAD_FORWARDING

  if (interface->new_message)
    interface->new_message(connection);

#ifdef MAD_FORWARDING
  if(!channel->is_forward)
    {
      p_mad_link_t link;
      tbx_bool_t has_been_forwarded = tbx_false;
      p_mad_buffer_t buffer = mad_get_user_send_buffer(&has_been_forwarded, 
						       sizeof(tbx_bool_t));
      if(interface->choice)
	link = interface->choice(connection, sizeof(tbx_bool_t),
				 mad_send_CHEAPER, mad_receive_EXPRESS);
      else
	link = &(connection->link[0]);

          
     if(link->buffer_mode == mad_buffer_mode_static)
       {
	 p_mad_buffer_t static_buffer = interface->get_static_buffer(link);
	 mad_copy_buffer(buffer, static_buffer);

	 interface->send_buffer(link, static_buffer);
       }
     else
       interface->send_buffer(link, buffer);
     connection->is_being_forwarded = tbx_false;
    }
#endif //MAD_FORWARDING
  
  /* structure initialisation */
  tbx_list_init(&(connection->buffer_list));
  tbx_list_init(&(connection->buffer_group_list));
  
  for(link_id = 0;
      link_id < connection->nb_link;
      link_id++)
    {
      tbx_list_init(&(connection->link[link_id].buffer_list));
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
  p_mad_driver_interface_t   interface =
    &(channel->adapter->driver->interface);
  p_mad_connection_t         connection;
  mad_link_id_t              link_id;
  
  LOG_IN();
  TRACE("New reception request");
  
  TBX_LOCK_SHARED(channel);
#ifdef MARCEL
  if (channel->reception_lock == tbx_true)
    {
      TBX_UNLOCK_SHARED(channel) ;
      LOG_OUT();
      return NULL;
    }
#else /* MARCEL */
  if (channel->reception_lock == tbx_true)
    FAILURE("mad_message_ready: reception dead lock");
#endif /* MARCEL */
  channel->reception_lock = tbx_true;
  TBX_UNLOCK_SHARED(channel);

  if (!(connection = interface->poll_message(channel)))
    {
      channel->reception_lock = tbx_false;
      TBX_UNLOCK_SHARED(channel) ;
      LOG_OUT();
      return NULL;
    }

  if (connection->lock == tbx_true)
    FAILURE("connection dead lock");

   tbx_list_init(&(connection->buffer_list));
   tbx_list_init(&(connection->buffer_group_list));
   tbx_list_init(&(connection->user_buffer_list));
   tbx_list_reference_init(&(connection->user_buffer_list_reference),
			   &(connection->user_buffer_list));
   
  for(link_id = 0;
      link_id < connection->nb_link;
      link_id++)
    {
      tbx_list_init(&(connection->link[link_id].buffer_list));
      tbx_list_init(&(connection->link[link_id].user_buffer_list));
    }
  
  connection->lock            = tbx_true;
  connection->send            = tbx_false;
  connection->pair_list_used  = tbx_false;
  connection->delayed_send    = tbx_false;
  connection->flushed         = tbx_true;
  connection->last_link       = NULL;
  connection->first_sub_buffer_group = tbx_true;
  connection->more_data       = tbx_false;
  LOG_OUT();
  return connection;
}
#endif /* MAD_MESSAGE_POLLING */

#ifdef MAD_FORWARDING
p_mad_connection_t
mad_begin_unpacking(p_mad_user_channel_t user_channel)
#else // MAD_FORWARDING
p_mad_connection_t
mad_begin_unpacking(p_mad_channel_t channel)
#endif //MAD_FORWARDING
{
#ifdef MAD_FORWARDING
  p_mad_channel_t            channel;
#endif //MAD_FORWARDING
  p_mad_driver_interface_t   interface;
  p_mad_connection_t         connection = NULL;
  mad_link_id_t              link_id;
  
  LOG_IN();
  TRACE("New reception request");

#ifdef MAD_FORWARDING
  if(user_channel->nb_active_channels > 1)
    {
      TRACE("Waiting for a thread to have received something");
      marcel_sem_P(&(user_channel->sem_message_ready));
      /*      channel = user_channel->channels[user_channel->msg_adapter_id];
	      TRACE("Ah ! I found a message on adapter %d (= %d)", 
	      user_channel->msg_adapter_id, channel->adapter->id);*/
      connection = user_channel->msg_connection;
      TRACE("Ah ! I found a message on adapter %d", 
	      connection->channel->adapter->id); 
      channel = connection->channel;
    }
  else
    {
      int i;
      for(i = 0;;i++)
	if(user_channel->adapter_ids[i] != -1)
	  break;
      TRACE("Using adapter %d", i);
      channel = user_channel->channels[user_channel->adapter_ids[i]];
    }
#endif //MAD_FORWARDING
  
  interface = &(channel->adapter->driver->interface);

  TBX_LOCK_SHARED(channel);
#ifdef MARCEL
  while (channel->reception_lock == tbx_true)
    {
      TBX_UNLOCK_SHARED(channel) ;
      marcel_yield();
      TBX_LOCK_SHARED(channel) ;
    }
#else /* MARCEL */
  if (channel->reception_lock == tbx_true)
    FAILURE("mad_begin_unpacking: reception dead lock");
#endif /* MARCEL */
  channel->reception_lock = tbx_true;
  TBX_UNLOCK_SHARED(channel);

#ifdef MAD_FORWARDING
  if(user_channel->nb_active_channels == 1)
#endif
  {
    TRACE("Receiving message");
    connection = interface->receive_message(channel);
    TRACE("Message received");
  }
#ifdef MAD_FORWARDING
  {
    tbx_bool_t has_been_forwarded;
    p_mad_buffer_t buffer = mad_get_user_receive_buffer(&has_been_forwarded, 
							sizeof(tbx_bool_t));
    p_mad_link_t link;
    if(interface->choice)
      link = interface->choice(connection, sizeof(tbx_bool_t),
			       mad_send_CHEAPER, mad_receive_EXPRESS);
    else
      link = &(connection->link[0]);
    
    if(link->buffer_mode == mad_buffer_mode_static)
      {
	p_mad_buffer_t static_buffer;
	interface->receive_buffer(link, &static_buffer);
	mad_copy_buffer(static_buffer, buffer);
      }
    else
      interface->receive_buffer(link, &buffer);
    
    connection->is_being_forwarded = has_been_forwarded;
#ifdef DEBUG
    if(has_been_forwarded)
      TRACE("Incoming Connection has been forwarded");
    else
      TRACE("Receiving normal connection");
#endif 
  }
#endif //MAD_FORWARDING

  if (connection->lock == tbx_true)
    FAILURE("connection dead lock");

   tbx_list_init(&(connection->buffer_list));
   tbx_list_init(&(connection->buffer_group_list));
   tbx_list_init(&(connection->user_buffer_list));
   tbx_list_reference_init(&(connection->user_buffer_list_reference),
			   &(connection->user_buffer_list));
   
  for(link_id = 0;
      link_id < connection->nb_link;
      link_id++)
    {
      tbx_list_init(&(connection->link[link_id].buffer_list));
      tbx_list_init(&(connection->link[link_id].user_buffer_list));
    }
  
  connection->lock            = tbx_true;
  connection->send            = tbx_false;
  connection->pair_list_used  = tbx_false;
  connection->delayed_send    = tbx_false;
  connection->flushed         = tbx_true;
  connection->last_link       = NULL;
  connection->first_sub_buffer_group = tbx_true;
  connection->more_data       = tbx_false;

  TRACE("Reception request initiated");
  LOG_OUT();
  return connection;
}

void
mad_end_packing(p_mad_connection_t connection)
{
  mad_link_id_t              link_id;
  p_mad_driver_interface_t   interface =
    &(connection->channel->adapter->driver->interface);

#ifdef MAD_FORWARDING
  if(connection->is_being_forwarded)
    interface = fwd_interface;
#endif //MAD_FORWARDING

  LOG_IN();
  if (connection->flushed == tbx_false)
    {
      p_mad_link_t    last_link    = connection->last_link;
      p_tbx_list_t    buffer_list  = &(connection->buffer_list);
  
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
	      tbx_append_list(&(connection->buffer_group_list), buffer_group);
	    }
	  else
	    FAILURE("invalid link mode");
	}

      connection->flushed = tbx_true;      
    }

  /* buffer pair handling */
  if (connection->pair_list_used)
    {
      if (!tbx_empty_list(&(connection->pair_list)))
	{
	  tbx_list_reference_t ref;

	  tbx_list_reference_init(&ref, &(connection->pair_list));

	  do
	    {
	      p_mad_buffer_pair_t pair ;

	      pair = tbx_get_list_reference_object(&ref);
	      mad_copy_buffer(&(pair->dynamic_buffer),
			      &(pair->static_buffer));
	    }
	  while (tbx_forward_list_reference(&ref));

	  tbx_foreach_destroy_list(&(connection->pair_list),
				   mad_foreach_free_buffer_pair_struct);
	}
    }

  /* residual buffer_group flushing */
  if (!tbx_empty_list(&(connection->buffer_group_list)))
    {
      tbx_list_reference_t list_ref;

      tbx_list_reference_init(&list_ref,
			      &(connection->buffer_group_list));

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
      p_mad_link_t link = &(connection->link[link_id]);
      
      if (!tbx_empty_list(&(link->buffer_list)))
	{
	  mad_buffer_group_t buffer_group;
	  
	  mad_make_buffer_group(&buffer_group,
				&(link->buffer_list),
				link);
	  if (   link->group_mode == mad_group_mode_split
	      && buffer_group.buffer_list.length == 1)
	    {
	      interface->send_buffer(link,
				     buffer_group.buffer_list.first->object);
	    }
	  else
	    {
	      interface->send_buffer_group(link, &buffer_group);
	    }
	  }
    }
  
  tbx_foreach_destroy_list(&(connection->buffer_group_list),
			   mad_foreach_free_buffer_group_struct);
  tbx_foreach_destroy_list(&(connection->buffer_list),
			   mad_foreach_free_buffer);
  for (link_id = 0;
       link_id < connection->nb_link;
       link_id++)
    {
	tbx_foreach_destroy_list(&(connection->link[link_id].buffer_list),
				 mad_foreach_free_buffer);
    }

#ifdef MAD_FORWARDING
  if(connection->is_being_forwarded)
    {
      p_mad_buffer_t empty_buffer = mad_get_user_send_buffer(NULL, 0);
      interface->send_buffer(connection->fwd_link, empty_buffer);
    }
#endif //MAD_FORWARDING
  connection->lock = tbx_false; 
  TRACE("Emission request completed");
  LOG_OUT();
}

void
mad_end_unpacking(p_mad_connection_t connection)
{
  p_mad_driver_interface_t   interface =
    &(connection->channel->adapter->driver->interface);
  p_mad_buffer_t             source;
  p_mad_buffer_t             destination;
  p_tbx_list_t               src_list  = NULL;
  p_tbx_list_t               dest_list = NULL;
  p_tbx_list_reference_t     dest_ref  = NULL;
  mad_link_id_t              link_id;
  
  LOG_IN();
  if (connection->flushed == tbx_false)
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
	      if (   !tbx_empty_list(dest_list)
		  && !tbx_reference_after_end_of_list(dest_ref))
		{
		  if (   tbx_empty_list(src_list)
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
		  interface->
		    receive_buffer(last_link,
				   (p_mad_buffer_t *)&buffer_group.
				     buffer_list.first->object);
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
	FAILURE("invalid link mode");

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
      p_mad_link_t           link = &(connection->link[link_id]);
      tbx_list_reference_t   destination_list_reference;

      src_list  = &(link->buffer_list);
      dest_list = &(link->user_buffer_list);
      dest_ref  = &destination_list_reference;
      tbx_list_reference_init(dest_ref, dest_list);

      if (link->buffer_mode == mad_static_buffer)
	{	    
	  if (!tbx_empty_list(dest_list))
	    {
	      interface->receive_buffer(link, &source);
	      tbx_append_list(src_list, source);
	      
	      do
		{
		  destination = tbx_get_list_reference_object(dest_ref);
		    
		  while (!mad_buffer_full(destination))
		    {
		      if (!mad_more_data(source))
			{
			  interface->return_static_buffer(link, source);
			  interface->receive_buffer(link, &source);
			  tbx_append_list(src_list, source);
			}
		      mad_copy_buffer(source, destination);
		    }
		}
	      while(tbx_forward_list_reference(dest_ref));
		
	      interface->return_static_buffer(link, source);
	    }
	} 
      else
	{
	  if (!tbx_empty_list(src_list))
	    {
	      mad_buffer_group_t buffer_group;
	      
	      mad_make_buffer_group(&buffer_group, src_list, link);

	      if (link->group_mode == mad_group_mode_split
		  && buffer_group.buffer_list.length == 1)
		{
		  interface->
		    receive_buffer(link,
				   (p_mad_buffer_t *)&buffer_group.
				     buffer_list.first->object);
		}
	      else
		{
		  interface->
		    receive_sub_buffer_group(link,
					     connection->
					       first_sub_buffer_group,
					     &buffer_group);
		}	      
	    }
	}
    }

  tbx_foreach_destroy_list(&(connection->buffer_list),
			   mad_foreach_free_buffer);
  tbx_foreach_destroy_list(&(connection->user_buffer_list),
			   mad_foreach_free_buffer);
  
  for (link_id = 0 ;
       link_id < connection->nb_link ;
       link_id++)
    {
      tbx_foreach_destroy_list(&(connection->link[link_id].buffer_list),
			       mad_foreach_free_buffer);
      tbx_foreach_destroy_list(&(connection->link[link_id].user_buffer_list),
			       mad_foreach_free_buffer);
    }

  connection->lock = tbx_false;
  connection->channel->reception_lock = tbx_false;

#ifdef MAD_FORWARDING
  {
    p_mad_user_channel_t user_channel = connection->channel->user_channel;
    if(user_channel->nb_active_channels > 1)
      {
	TRACE("Releasing Semaphore to wake polling thread");
	marcel_sem_V(&(user_channel->sem_msg_handled));
      }
  }
#endif //MAD_FORWARDING

  TRACE("Reception request completed");
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
  mad_buffer_mode_t          buffer_mode;
  mad_group_mode_t           group_mode;
  tbx_bool_t                 last_delayed_send;
  p_mad_buffer_t             source            = NULL;
  p_mad_buffer_t             destination       = NULL;
  p_tbx_list_t               dest_list         = &(connection->buffer_list);
  p_tbx_list_t               buffer_group_list =
    &(connection->buffer_group_list);

  LOG_IN();
  if (!connection->lock)
    FAILURE("invalid connection object");

  if (user_buffer_length == 0) return;

#ifdef MAD_FORWARDING
  if(connection->is_being_forwarded)
    {
      interface = fwd_interface;
      link = connection->fwd_link;
      TRACE("Fwd link chosen");
    }
  else
#endif //MAD_FORWARDING
    {
      if (interface->choice)
	{
	  link = interface->choice(connection,
				   user_buffer_length,
				   send_mode,
				   receive_mode);
	}
      else
	{
	  link = &(connection->link[0]);
	}
      TRACE("Link chosen : %d", link->id);
    }
  
  link_id               = link->id;
  link_mode             = link->link_mode;
  buffer_mode           = link->buffer_mode;
  group_mode            = link->group_mode;
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
	  link_mode = link->link_mode;

	  if (   (receive_mode == mad_receive_EXPRESS)
	      && (link_mode    == mad_link_mode_link_group))
	    {
	      link_mode = mad_link_mode_buffer_group;
	    }
	}
    }

  if (   (connection->last_link != NULL)
      && (link_mode != mad_link_mode_link_group)
      && (   (link != connection->last_link)
	  || (    (last_delayed_send != connection->delayed_send)
	       && (connection->last_link_mode == mad_link_mode_buffer)))
      && (connection->flushed == tbx_false))
    {
      p_mad_link_t    last_link      = connection->last_link;

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
	    FAILURE("invalid link mode");
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
	    FAILURE("invalid link mode");
	}

      connection->flushed = tbx_true;
      tbx_mark_list(dest_list);
    }

  source = mad_get_user_send_buffer(user_buffer, user_buffer_length);
#ifdef MAD_FORWARDING
  source->informations.send_mode = send_mode;
  source->informations.recv_mode = receive_mode;
#endif //MAD_FORWARDING
  if (link_mode == mad_link_mode_buffer)
    {
      /* B U F F E R   mode
	 -----------        */      
      if (connection->delayed_send == tbx_true)
	FAILURE("cannot send data in buffer mode when delayed send is on");

      if (   (receive_mode == mad_receive_EXPRESS)
	  || (receive_mode == mad_receive_CHEAPER))
	{
	  if (   (send_mode == mad_send_SAFER)
	      || (send_mode == mad_send_CHEAPER))
	    {
	      if (buffer_mode == mad_buffer_mode_dynamic)
		{
		  interface->send_buffer(link, source);
		  mad_free_buffer_struct(source);		  
		  connection->flushed = tbx_true;
		}
	      else if (buffer_mode == mad_buffer_mode_static)
		{		  
		  if (tbx_empty_list(dest_list) || connection->flushed)
		    {
		      destination = interface->get_static_buffer(link);
		  
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
			  interface->send_buffer(link, destination);
			  destination = interface->get_static_buffer(link);
			  tbx_append_list(dest_list, destination);
			}
		  
		      mad_copy_buffer(source, destination);  
		    }
		  while(mad_more_data(source));
	      
		  if (mad_buffer_full(destination))
		    {  
		      interface->send_buffer(link, destination);
		      connection->flushed = tbx_true;
		    }
		  else
		    {
		      connection->flushed = tbx_false;
		    }

		  tbx_mark_list(dest_list);
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
		  tbx_append_list(dest_list, mad_duplicate_buffer(source));
		}
	      else if (   (send_mode == mad_send_LATER)
		       || (send_mode == mad_send_CHEAPER))
		{
		  tbx_append_list(dest_list, source);
		}
	      else	    
		FAILURE("unknown send mode");
	    }
	  else if (buffer_mode == mad_buffer_mode_static)
	    {
	      if (   (send_mode == mad_send_SAFER)
		     || (send_mode == mad_send_CHEAPER))
		{
		  if (tbx_empty_list(dest_list) || connection->flushed)
		    {
		      destination = interface->get_static_buffer(link);
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
			  destination = interface->get_static_buffer(link);
			  tbx_append_list(dest_list, destination);
			}
		  
		      mad_copy_buffer(source, destination);  
		    }
		  while(mad_more_data(source));
		}
	      else if (send_mode == mad_send_LATER)
		{
		  if (tbx_empty_list(dest_list) || connection->flushed)
		    {
		      destination = interface->get_static_buffer(link);
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
			  destination = interface->get_static_buffer(link);
			  tbx_append_list(dest_list, destination);
			}
		  
		      tbx_append_list(&(connection->pair_list),
				      mad_make_sub_buffer_pair(source,
							       destination));
		      mad_pseudo_copy_buffer(source, destination);  
		    }
		  while(mad_more_data(source));	      
		  connection->pair_list_used = tbx_true;
		}
	      else
		FAILURE("unknown send mode");
	    }
	  else
	    FAILURE("unknown buffer mode");
	}
      else
	FAILURE("unknown receive mode");

      connection->flushed        = tbx_false;
      connection->last_link      = link;
      connection->last_link_mode = link_mode;

      if (   (group_mode == mad_group_mode_split)
	  && (receive_mode == mad_receive_EXPRESS))
	{
	  if (connection->delayed_send == tbx_false)
	    {
	      mad_buffer_group_t buffer_group;
		  
	      mad_make_buffer_group(&buffer_group, dest_list, link);
	      if (buffer_group.buffer_list.length == 1)
		{
		  interface->send_buffer(link,
					 buffer_group.buffer_list.first->object);
		}
	      else
		{
		  interface->send_buffer_group(link, &buffer_group);
		}
	    }
	  else
	    {
	      p_mad_buffer_group_t buffer_group;
		  
	      buffer_group = mad_alloc_buffer_group_struct();
	      mad_make_buffer_group(buffer_group, dest_list, link);
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
      dest_list = &(link->buffer_list);

      if (receive_mode == mad_receive_CHEAPER)
	{
	  if (buffer_mode == mad_buffer_mode_static)
	    {
	      if (   (send_mode == mad_send_SAFER)
		  || (send_mode == mad_send_CHEAPER))
		{
		  if (tbx_empty_list(dest_list))
		    {
		      destination =
			interface->get_static_buffer(link);
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
			  destination = interface->get_static_buffer(link);
			  tbx_append_list(dest_list, destination);
			}
		      mad_copy_buffer(source, destination);  
		    }
		  while(mad_more_data(source));
		}
	      else if (send_mode == mad_send_LATER)
		{
		  if (tbx_empty_list(dest_list) || connection->flushed)
		    {
		      destination = interface->get_static_buffer(link);
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
			  destination = interface->get_static_buffer(link);
			  tbx_append_list(dest_list, destination);
			}
		  
		      tbx_append_list(&(connection->pair_list),
				      mad_make_sub_buffer_pair(source,
							       destination));
		      mad_pseudo_copy_buffer(source, destination);  
		    }
		  while(mad_more_data(source));
		  connection->pair_list_used = tbx_true;
		}
	      else
		FAILURE("unknown send mode");
	    }
	  else if (buffer_mode == mad_buffer_mode_dynamic)
	    {
	      if (send_mode == mad_send_SAFER)
		{
		  tbx_append_list(dest_list, mad_duplicate_buffer(source));
		}
	      else if (   (send_mode == mad_send_LATER)
		       || (send_mode == mad_send_CHEAPER))
		{
		  tbx_append_list(dest_list, source);
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
  mad_group_mode_t           group_mode;
  tbx_bool_t                 last_delayed_send;
  p_mad_buffer_t             source;
  p_mad_buffer_t             destination;
  p_tbx_list_t               src_list  = &(connection->buffer_list);
  p_tbx_list_t               dest_list = &(connection->user_buffer_list);
  p_tbx_list_reference_t     dest_ref  =
    &(connection->user_buffer_list_reference);

  LOG_IN();
  if (!connection->lock)
    FAILURE("invalid connection object");

  if (user_buffer_length == 0) return;

#ifdef MAD_FORWARDING
  if(connection->is_being_forwarded)
    {
      interface = fwd_interface;
      link = connection->fwd_link;
    }
  else
#endif //MAD_FORWARDING
    {
      if (interface->choice)
	{
	  link = interface->choice(connection,
				   user_buffer_length,
				   send_mode,
				   receive_mode);
	}
      else
	{
	  link = &(connection->link[0]);
	}
    }
  
  link_id               = link->id;
  link_mode             = link->link_mode;
  buffer_mode           = link->buffer_mode;
  group_mode            = link->group_mode;
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
			  interface->receive_buffer(link, &source);
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
				    return_static_buffer(link, source);
				  interface->
				    receive_buffer(last_link, &source);
				  tbx_append_list(src_list, source);
				}
			      mad_copy_buffer(source, destination);
			    }
			  while (!mad_buffer_full(destination));
			}
		      while (tbx_forward_list_reference(dest_ref));

		      interface->return_static_buffer(link, source);
		    }
		  else
		    {
		      if (connection->more_data)
			{
			  source = tbx_get_list_object(src_list);
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

		  if (last_link->group_mode == mad_group_mode_split
		      && buffer_group.buffer_list.length == 1)
		    {
		      interface->
			receive_buffer(last_link,
				       (p_mad_buffer_t *)&buffer_group.buffer_list.first->object);
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
	    FAILURE("invalid link mode");
	  
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
	FAILURE("cannot receive data in buffer mode when delayed send is on");

      if (   (receive_mode == mad_receive_EXPRESS)
	  || (receive_mode == mad_receive_CHEAPER))
	{
	  if (   (send_mode == mad_send_SAFER)
	      || (send_mode == mad_send_CHEAPER))
	    {
	      if (buffer_mode == mad_buffer_mode_dynamic)
		{
		  interface->receive_buffer(link, &destination);
		  connection->flushed        = tbx_true;
		}
	      else if (buffer_mode == mad_buffer_mode_static)
		{
		  if (   (tbx_empty_list(src_list))
			 || (!connection->more_data))
		    {
		      interface->receive_buffer(link, &source);
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
			  interface->return_static_buffer(link, source);
		      
			  interface->receive_buffer(link, &source);
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

		      interface->return_static_buffer(link, source);
		  
		      tbx_mark_list(src_list);
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

		  tbx_append_list(src_list, destination);
		  mad_make_buffer_group(&buffer_group, src_list, link);

		  if (link->group_mode == mad_group_mode_split
		      && buffer_group.buffer_list.length == 1)
		    {
		      interface->
			receive_buffer(link,
				       (p_mad_buffer_t *)&buffer_group.
				       buffer_list.first->object);
		    }
		  else
		    {
		      interface->
			receive_sub_buffer_group(link,
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
		    FAILURE("unknown group mode");
		  
		  tbx_mark_list(src_list);
		}
	      else if (buffer_mode == mad_buffer_mode_static)
		{
		  tbx_append_list(dest_list, destination);
		  
		  if (   (tbx_empty_list(src_list))
		      || (!connection->more_data))
		    {
		      interface->receive_buffer(link, &source);
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
			      interface->return_static_buffer(link, source);
			      interface->receive_buffer(link, &source);
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
		      interface->return_static_buffer(link, source);
		      tbx_mark_list(src_list);
		      connection->more_data = tbx_false;
		      connection->flushed   = tbx_true; 
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
	  if (connection->delayed_send == tbx_true)
	    FAILURE("Cheaper data is not received in buffer_group mode "
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
	    FAILURE("unknown send mode");

	  connection->flushed   = tbx_false;	      
	  connection->more_data = tbx_false;
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
		  tbx_append_list(&(link->buffer_list), destination);
		}
	      else if (buffer_mode == mad_buffer_mode_static)
		{
		  tbx_append_list(&(link->user_buffer_list), destination);
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
  LOG_OUT();
}
