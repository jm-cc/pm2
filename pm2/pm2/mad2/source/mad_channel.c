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
$Log: mad_channel.c,v $
Revision 1.11  2000/02/28 11:06:12  rnamyst
Changed #include "" into #include <>.

Revision 1.10  2000/02/08 17:49:04  oaumage
- support de la net toolbox

Revision 1.9  2000/02/03 17:37:37  oaumage
- mad_channel.c : correction de la liberation des donnees specifiques aux
                  connections
- mad_sisci.c   : support DMA avec double buffering

Revision 1.8  2000/01/31 15:53:34  oaumage
- mad_channel.c : verrouillage au niveau des canaux au lieu des drivers
- madeleine.c : deplacement de aligned_malloc vers la toolbox

Revision 1.7  2000/01/13 14:45:55  oaumage
- adaptation pour la prise en compte de la toolbox
- suppression des fichiers redondant
- mad_channel.c, madeleine.c: amelioration des fonctions `par defaut' au niveau
  du support des drivers

Revision 1.6  2000/01/10 10:23:02  oaumage
*** empty log message ***

Revision 1.5  2000/01/05 15:51:25  oaumage
- mad_list_management.c: changement de `len' en `length'
- mad_channel.c: correction au niveau de l'appel a mad_link_exit

Revision 1.4  2000/01/04 16:49:09  oaumage
- madeleine: corrections au niveau du support `external spawn'
- support des fonctions non definies dans les drivers

Revision 1.3  1999/12/15 17:31:27  oaumage
Ajout de la commande de logging de CVS

______________________________________________________________________________
*/

/*
 * Mad_channel.c
 * =============
 */
/* #define DEBUG */
/* #define TRACING */
#include "madeleine.h"

p_mad_channel_t
mad_open_channel(p_mad_madeleine_t madeleine,
		 mad_adapter_id_t adapter_id)
{
  p_mad_configuration_t      configuration = &(madeleine->configuration);
  p_mad_adapter_t            adapter       =
    &(madeleine->adapter[adapter_id]);
  p_mad_driver_t             driver        = adapter->driver;
  p_mad_driver_interface_t   interface     = &(driver->interface);
  p_mad_channel_t            channel;
  ntbx_host_id_t              host;

  LOG_IN();
  PM2_LOCK();
  TRACE("channel allocation");
  PM2_LOCK_SHARED(madeleine);
  PM2_LOCK_SHARED(adapter);
  channel = malloc(sizeof(mad_channel_t));
  CTRL_ALLOC(channel);

  PM2_INIT_SHARED(channel);
  PM2_LOCK_SHARED(channel);
  channel->id             = madeleine->nb_channel++;
  channel->adapter        = adapter;
  channel->reception_lock = tbx_false;
  channel->specific       = NULL;

  if (interface->channel_init)
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
      in->lock           = tbx_false;
      in->send           = tbx_false;
      in->specific       = NULL;
      
      out->remote_host_id = host;
      out->channel        = channel;
      out->reverse        = out;
      out->way            = mad_incoming_connection;
      out->nb_link        = 0;
      out->link           = NULL;
      out->lock           = tbx_false;
      out->send           = tbx_false;
      out->specific       = NULL;
      
      if (interface->connection_init)
	{
	  interface->connection_init(in, out);

	  if (   (in->nb_link  <= 0)
	      || (out->nb_link <= 0)
	      || (in->nb_link  != out->nb_link))
	    {
	      FAILURE("mad_open_channel: invalid link number");
	    }
	}
      else
	{
	  in->nb_link  = 1;
	  out->nb_link = 1;
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

	  if (interface->link_init)
	    {
	      interface->link_init(in_link);
	      interface->link_init(out_link);
	    }
	}
    }

  if (interface->before_open_channel)
    interface->before_open_channel(channel);
  
  if (interface->accept && interface->connect)
    {
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
		  ntbx_host_id_t remote_host;

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
    }
  
  if (interface->after_open_channel)
    interface->after_open_channel(channel);

  tbx_append_list(&(madeleine->channel), channel);
  PM2_UNLOCK_SHARED(channel);
  PM2_UNLOCK_SHARED(adapter);
  PM2_UNLOCK_SHARED(madeleine);
  PM2_UNLOCK();
  LOG_OUT();
  
  return channel;
}

static void
mad_foreach_close_channel(void *object)
{
  p_mad_channel_t            channel       = object;
  p_mad_adapter_t            adapter       = channel->adapter;
  p_mad_driver_t             driver        = adapter->driver;
  p_mad_driver_interface_t   interface     = &(driver->interface);
  p_mad_madeleine_t          madeleine     = driver->madeleine;
  p_mad_configuration_t      configuration = &(madeleine->configuration);
  ntbx_host_id_t              host;

  LOG_IN();

  TRACE("channel deallocation");
  PM2_LOCK();
  PM2_LOCK_SHARED(adapter);
  PM2_LOCK_SHARED(channel);

  if (interface->before_close_channel)
    interface->before_close_channel(channel);

  if (interface->disconnect)
    {      
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
		  ntbx_host_id_t remote_host;

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
    }

  if (interface->after_close_channel)
    interface->after_close_channel(channel);
  for (host = 0; host < configuration->size; host++)
    {
      p_mad_connection_t   in  = &(channel->input_connection[host]);
      p_mad_connection_t   out = &(channel->output_connection[host]);
      mad_link_id_t        link_id;

      if (interface->link_exit)
	{
	  for (link_id = 0;
	       link_id < in->nb_link;
	       link_id++)
	    {
	      p_mad_link_t   in_link  = &(in->link[link_id]);
	      p_mad_link_t   out_link = &(out->link[link_id]);
	      
	      interface->link_exit(out_link);
	      interface->link_exit(in_link);
	    }
	}
      else
	{
	  if (out->link->specific)
	    {
	      free(out->link->specific);
	      out->link->specific = NULL;
	    }

	  if (in->link->specific)
	    {
	      free(in->link->specific);
	    }	  
	}
      
      free(out->link);
      free(in->link);
      if (interface->connection_exit)
	{
	  interface->connection_exit(in, out);
	}
      else
	{
	  if (out->specific)
	    {	      
	      free(out->specific);

	      if (in->specific == out->specific)
		{
		  in->specific = NULL;  
		}
	      out->specific = NULL;
	    }

	  if (in->specific)
	    {
	      free(in->specific);
	      in->specific = NULL;
	    }	  
	}
    }

  free(channel->output_connection);
  free(channel->input_connection);
  if (interface->channel_exit)
    {
      interface->channel_exit(channel);
    }
  else
    {
      if (channel->specific)
	{
	  free(channel->specific);
	}
    }
  free(channel);
  /* Note: the channel is never unlocked since it is being destroyed */
  PM2_UNLOCK_SHARED(adapter);
  PM2_UNLOCK();
  LOG_OUT();
}

void
mad_close_channels(p_mad_madeleine_t madeleine)
{
  tbx_foreach_destroy_list(&(madeleine->channel),
			   mad_foreach_close_channel);
}
