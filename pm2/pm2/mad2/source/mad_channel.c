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
 * Mad_channel.c
 * =============
 */
/* #define DEBUG */
#include <madeleine.h>

p_mad_channel_t
mad_open_channel(p_mad_madeleine_t madeleine,
		 mad_channel_id_t channel_id,
		 mad_adapter_id_t adapter_id)
{
  p_mad_configuration_t      configuration = &(madeleine->configuration);
  p_mad_adapter_t            adapter       =
    &(madeleine->adapter[adapter_id]);
  p_mad_driver_t             driver        = adapter->driver;
  p_mad_driver_interface_t   interface     = &(driver->interface);
  p_mad_channel_t            channel;
  mad_host_id_t              host;

  LOG_IN();
  
  PM2_LOCK_SHARED(madeleine);
  PM2_LOCK_SHARED(adapter);
  channel = malloc(sizeof(mad_channel_t));
  CTRL_ALLOC(channel);

  PM2_INIT_SHARED(channel);
  PM2_LOCK_SHARED(channel);
  channel->id             = channel_id;
  channel->adapter        = adapter;
  channel->reception_lock = mad_false;
  channel->specific       = NULL;

  interface->channel_init(channel);

  channel->input_connection =
    malloc(configuration->size * sizeof(mad_connection_t));
  CTRL_ALLOC(channel->input_connection);
  
  channel->output_connection =
    malloc(configuration->size * sizeof(mad_connection_t));
  CTRL_ALLOC(channel->output_connection);  

  for (host = 0; host < configuration->size; host++)
    {
      p_mad_connection_t   in  = &(channel->input_connection[host]);
      p_mad_connection_t   out = &(channel->output_connection[host]);
      mad_link_id_t        link_id;
      
      in->remote_host_id = host;
      in->channel        = channel;
      in->reverse        = out;
      in->way            = mad_incoming_connection;
      in->nb_link        = 0;
      in->link           = NULL;
      in->lock           = mad_false;
      in->send           = mad_false;
      in->specific       = NULL;
      
      out->remote_host_id = host;
      out->channel        = channel;
      out->reverse        = out;
      out->way            = mad_incoming_connection;
      out->nb_link        = 0;
      out->link           = NULL;
      out->lock           = mad_false;
      out->send           = mad_false;
      out->specific       = NULL;
      
      interface->connection_init(in, out);

      if (   (in->nb_link == -1)
	  || (out->nb_link == -1)
	  || (in->nb_link != out->nb_link))
	{
	  FAILURE("mad_open_channel: invalid link number");
	}
      
      in->link  = malloc( in->nb_link * sizeof(mad_link_t));
      CTRL_ALLOC(in->link);
      out->link = malloc(out->nb_link * sizeof(mad_link_t));
      CTRL_ALLOC(out->link);

      for (link_id = 0;
	   link_id < in->nb_link;
	   link_id++)
	{
	  p_mad_link_t   in_link  = &(in->link[link_id]);
	  p_mad_link_t   out_link = &(out->link[link_id]);
	  
	  in_link->connection = in;
	  in_link->id         = link_id;
	  in_link->specific   = NULL;

	  out_link->connection = out;
	  out_link->id         = link_id;
	  out_link->specific   = NULL;

	  interface->link_init(in_link);
	  interface->link_init(out_link);
	}
    }

  interface->before_open_channel(channel);

  if (driver->connection_type == mad_bidirectional_connection)
    {
      for (host = 0;
	   host < configuration->local_host_id;
	   host++)
	{
	  interface->accept(channel);
	}      

      for(host = madeleine->configuration.local_host_id + 1;
	  host < configuration->size;
	  host++)
	{
	  interface->connect(&(channel->output_connection[host]));
	}
    }
  else
    {
      for(host = 0;
	  host < configuration->size;
	  host++)
	{
	  if (host == configuration->local_host_id)
	    {
	      mad_host_id_t remote_host;

	      for (remote_host = 0;
		   remote_host < configuration->size;
		   remote_host++)
		{
		  if (remote_host != configuration->local_host_id)
		    {
		      interface->accept(channel);
		    }
		}
	    }
	  else
	    {
	      interface->connect(&(channel->output_connection[host]));
	    }
	}
    }

  interface->after_open_channel(channel);
  mad_append_list(&(madeleine->channel), channel);
  PM2_UNLOCK_SHARED(channel);
  PM2_UNLOCK_SHARED(adapter);
  PM2_UNLOCK_SHARED(madeleine);
  LOG_OUT();
  
  return channel;
}

void
mad_close_channel(p_mad_channel_t channel)
{
  p_mad_adapter_t            adapter       = channel->adapter;
  p_mad_driver_t             driver        = adapter->driver;
  p_mad_driver_interface_t   interface     = &(driver->interface);
  p_mad_madeleine_t          madeleine     = driver->madeleine;
  p_mad_configuration_t      configuration = &(madeleine->configuration);
  mad_host_id_t              host;

  LOG_IN();

  PM2_LOCK_SHARED(madeleine);
  PM2_LOCK_SHARED(adapter);
  PM2_LOCK_SHARED(channel);
  
  interface->before_close_channel(channel);

  if (driver->connection_type == mad_bidirectional_connection)
    {
      for(host = 0;
	  host < configuration->local_host_id;
	  host++)
	{
	  interface->disconnect(&(channel->input_connection[host]));
	}

      for(host = madeleine->configuration.local_host_id + 1;
	  host < configuration->size;
	  host++)
	{
	  interface->disconnect(&(channel->output_connection[host]));
	}
    }
  else
    {
      for(host = 0;
	  host < configuration->size;
	  host++)
	{
	  if (host == configuration->local_host_id)
	    {
	      mad_host_id_t remote_host;

	      for (remote_host = 0 ;
		   remote_host < configuration->size;
		   remote_host++)
		{
		  if (remote_host != configuration->local_host_id)
		    {
		      interface->
			disconnect(&(channel
				     ->input_connection[remote_host]));
		    }
		}
	    }
	  else
	    {
	      interface->disconnect(&(channel->output_connection[host]));
	    }
	}
    }
   
  interface->after_close_channel(channel);

  channel->id = -1;
  
  PM2_UNLOCK_SHARED(channel);
  /* free(channel);
   * NOTE: Channel cannot currently be removed from madeleine's list
   *       (mad2 lists can only grow)
   *       Hence 'channel' cannot be freed here 
   */
  PM2_UNLOCK_SHARED(adapter);
  PM2_UNLOCK_SHARED(madeleine);
  LOG_OUT();
}
