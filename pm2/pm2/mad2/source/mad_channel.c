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
Revision 1.18  2001/01/16 09:55:21  oaumage
- integration du mecanisme de forwarding
- modification de l'usage des flags

Revision 1.17  2000/11/10 14:17:56  oaumage
- nouvelle procedure d'initialisation

Revision 1.16  2000/07/10 14:25:45  oaumage
- Correction de l'initialisation des objets connection

Revision 1.15  2000/06/06 12:54:51  oaumage
- Ajout du calcul de la taille des groupes de buffers dynamiques

Revision 1.14  2000/05/18 14:05:48  oaumage
- Suppression des TBX_LOCK

Revision 1.13  2000/03/27 08:50:51  oaumage
- pre-support decoupage de groupes
- correction au niveau du support du demarrage manuel

Revision 1.12  2000/03/08 17:19:12  oaumage
- support de compilation avec Marcel sans PM2
- pre-support de packages de Threads != Marcel
- utilisation de TBX_MALLOC

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

/*
 * Headers
 * -------
 */
#include "madeleine.h"

/*
 * BEGIN Mad forwarding specific
 * -----------------------------
 */

#ifdef MAD_FORWARDING
#define NB_CHANNELS  2
#define NB_MACHINES  3 // ?????????

char* channel_names[NB_CHANNELS] = {"calcul", "controle"};
mad_adapter_id_t channel_topology[NB_CHANNELS][NB_MACHINES][NB_MACHINES] = {
  {{1, 0, 1},
   {0, -1, -1},
   {1, -1, 1}},
 
  {{-1, 0, 0},
   {0, -1, 0},
   {0, 0, -1}}
};

ntbx_host_id_t channel_gateways[NB_CHANNELS][NB_MACHINES][NB_MACHINES] = {
  {{0, 1, 2},
   {0, 1, 0},
   {0, 0, 2}},
 
  {{0, 1, 2},
   {0, 1, 2},
   {0, 1, 2}}
};

tbx_bool_t channel_is_gateway[NB_CHANNELS][NB_MACHINES] = {
  {tbx_true, tbx_false, tbx_false},
  {tbx_false, tbx_false, tbx_false}
};
#endif/* MAD_FORWARDING */

/*
 * END Mad forwarding specific
 * -----------------------------
 */



/*
 * Structures
 * ----------
 */


/*
 * Static prototypes
 * -----------------
 */
#ifdef MAD_FORWARDING
static
p_mad_channel_t 
mad_open_real_channel(p_mad_madeleine_t  madeleine,
		      mad_adapter_id_t   adapter_id,
		      tbx_bool_t         is_forward,
		      mad_adapter_id_t  *mask);
#endif/* MAD_FORWARDING */


/*
 * Functions
 * ---------
 */
#ifdef MAD_FORWARDING
p_mad_user_channel_t
mad_open_channel(p_mad_madeleine_t  madeleine,
		 char              *name)
{
  p_mad_configuration_t      configuration = madeleine->configuration;
  ntbx_host_id_t             size          = configuration->size;
  ntbx_host_id_t             rank          = configuration->local_host_id;
  tbx_bool_t                 adapter_initiated[madeleine->nb_adapter];
  tbx_bool_t                 adapter_is_active[madeleine->nb_adapter];
  p_mad_user_channel_t       user_channel;
  ntbx_host_id_t             host;
  ntbx_host_id_t             remote_host;
  int                        user_channel_id;
  
  LOG_IN();
  if (name)
    {
      for (user_channel_id = 0;
	   user_channel_id < NB_CHANNELS;
	   user_channel_id++)
	{
	  if (!strcmp(name, channel_names[user_channel_id]))
	    break;
	}
      
      if (user_channel_id == NB_CHANNELS)
	FAILURE("mad_open_channel: unknown channel name");
    }
  else
    user_channel_id = 0;
  
  if (!fwd_interface)
    {
      TRACE("Allocating forward interface");
      fwd_interface = TBX_MALLOC(sizeof(mad_driver_interface_t));
      CTRL_ALLOC(fwd_interface);

      fwd_interface->link_init                = mad_fwd_link_init;
      fwd_interface->new_message              = mad_fwd_new_message;
      fwd_interface->send_buffer              = mad_fwd_send_buffer;
      fwd_interface->receive_buffer           = mad_fwd_receive_buffer;
      fwd_interface->send_buffer_group        = mad_fwd_send_buffer_group;
      fwd_interface->receive_sub_buffer_group =
	mad_fwd_receive_sub_buffer_group;
    }

  TRACE("User channel allocation");
  TBX_LOCK_SHARED(madeleine);
  user_channel = TBX_MALLOC(sizeof(mad_user_channel_t));
  CTRL_ALLOC(user_channel);

  TBX_INIT_SHARED(user_channel);
  user_channel->channels = 
    TBX_CALLOC(madeleine->nb_adapter, sizeof(p_mad_adapter_t));
  CTRL_ALLOC(user_channel->channels);

  user_channel->forward_channels = 
    TBX_CALLOC(madeleine->nb_adapter, sizeof(p_mad_adapter_t));
  CTRL_ALLOC(user_channel->forward_channels);

  user_channel->name        = name;
  user_channel->adapter_ids = 
    channel_topology[user_channel_id][rank];
  user_channel->gateways    = 
    channel_gateways[user_channel_id][rank];
  
  marcel_sem_init(&(user_channel->sem_message_ready), 0);
  marcel_sem_init(&(user_channel->sem_lock_msgs), 1);
  marcel_sem_init(&(user_channel->sem_msg_handled), 0);
  user_channel->msg_connection     = NULL;
  user_channel->nb_active_channels = 0;

  {
    mad_adapter_id_t temp;
    mad_adapter_id_t i;

    for (i = 0; i < madeleine->nb_adapter; i++)
      {
	adapter_initiated[i] = tbx_false;
	adapter_is_active[i] = tbx_false;
      }
    
    for (i = 0; i < size; i++)
      {
	if ((temp = channel_topology[user_channel_id][rank][i]) != -1)
	  {
	    if (!adapter_is_active[temp])
	      {
		user_channel->nb_active_channels++;
		adapter_is_active[temp] = tbx_true;
	      }
	  }
      }    
  }
  
  for(host = 0; host < rank; host++)
    {      
      for(remote_host = host + 1; 
	  remote_host < size; 
	  remote_host++)
	{
	  mad_adapter_id_t adapter_id =
	    channel_topology[user_channel_id][host][remote_host];
	  
	  if (   (adapter_id != -1)
	      && (!adapter_initiated[adapter_id]))
	    {
	      user_channel->channels[adapter_id] =
		mad_open_real_channel(madeleine, adapter_id, tbx_false,
				      channel_topology[user_channel_id][rank]);

	      user_channel->channels[adapter_id]->user_channel = user_channel;

	      user_channel->forward_channels[adapter_id] =
		mad_open_real_channel(madeleine, adapter_id, tbx_true,
				      channel_topology[user_channel_id][rank]);

	      user_channel->forward_channels[adapter_id]->user_channel =
		user_channel;

	      adapter_initiated[adapter_id] = tbx_true;
	    }
	
	}
    }
  
  if (channel_is_gateway[user_channel_id][rank])
    {
      mad_adapter_id_t i;
      
      for (i = 0; i < madeleine->nb_adapter; i++)
	{
	  if (adapter_is_active[i] == tbx_true)
	    {
	      TRACE("reate forwarding tread");
	      marcel_create(NULL,
			    NULL,
			    (marcel_func_t) mad_forward,
			    user_channel->forward_channels[i]);
	    
	      if (user_channel->nb_active_channels > 1)
		{
		  TRACE("Create polling thread");
		  marcel_create(NULL,
				NULL,
				(marcel_func_t) mad_polling,
				user_channel->channels[i]);
		}
	    
	      TRACE("Threads created for adpater %d", i);
	    }
	}
    }


  //  TBX_FREE(adapter_initiated);
  TBX_UNLOCK_SHARED(madeleine);
  LOG_OUT();

  return user_channel;
}

static
p_mad_channel_t 
mad_open_real_channel(p_mad_madeleine_t  madeleine,
		      mad_adapter_id_t   adapter_id,
		      tbx_bool_t         is_forward,
		      mad_adapter_id_t  *mask)
{
  p_mad_configuration_t      configuration = madeleine->configuration;
  p_mad_adapter_t            adapter       =
    &(madeleine->adapter[adapter_id]);
  p_mad_driver_t             driver        = adapter->driver;
  p_mad_driver_interface_t   interface     = &(driver->interface);
  p_mad_channel_t            channel       = NULL;
  ntbx_host_id_t             size          = configuration->size;
  ntbx_host_id_t             rank          = configuration->local_host_id;
  ntbx_host_id_t             host;

  LOG_IN();
  TRACE("channel allocation on adapter %d", adapter_id);
  TBX_LOCK_SHARED(adapter);
  channel = TBX_MALLOC(sizeof(mad_channel_t));
  CTRL_ALLOC(channel);

  TBX_INIT_SHARED(channel);

  channel->id             = madeleine->nb_channel++;
  channel->adapter        = adapter;
  channel->reception_lock = tbx_false;
  channel->specific       = NULL;
  channel->is_forward     = is_forward;

  if (interface->channel_init)
    interface->channel_init(channel);
  
  channel->input_connection = TBX_MALLOC(size * sizeof(mad_connection_t));
  CTRL_ALLOC(channel->input_connection);
  
  channel->output_connection = TBX_MALLOC(size * sizeof(mad_connection_t));
  CTRL_ALLOC(channel->output_connection);  

  for (host = 0; host < size; host++)
    {
      p_mad_connection_t   in  = &(channel->input_connection[host]);
      p_mad_connection_t   out = &(channel->output_connection[host]);
      mad_link_id_t        link_id;
      mad_adapter_id_t     mask_adapter_id = mask[host];
      
      if (mask_adapter_id != adapter_id)
	{
	  TRACE("Connection to %d is inactive", host);
	  in->active = tbx_false;
	  out->active = tbx_false;
	}
      else
	{
	  in->remote_host_id = host;
	  in->channel        = channel;
	  in->reverse        = out;
	  in->way            = mad_incoming_connection;
	  in->nb_link        = 0;
	  in->link           = NULL;
	  in->lock           = tbx_false;
	  in->send           = tbx_false;
	  in->specific       = NULL;
	  in->active         = tbx_true;
	  
	  out->remote_host_id = host;
	  out->channel        = channel;
	  out->reverse        = in;
	  out->way            = mad_outgoing_connection;
	  out->nb_link        = 0;
	  out->link           = NULL;
	  out->lock           = tbx_false;
	  out->send           = tbx_false;
	  out->specific       = NULL;
	  out->active         = tbx_true;
	
	  if (interface->connection_init)
	    {
	      interface->connection_init(in, out);
	      
	      if (   (in->nb_link  <= 0)
		     || (out->nb_link <= 0)
		     || (in->nb_link  != out->nb_link))
		{
		  FAILURE("mad_open_real_channel: invalid link number");
		}
	    }
	  else
	    {
	      in->nb_link  = 1;
	      out->nb_link = 1;
	    }
	  
	  in->link  = TBX_MALLOC( in->nb_link * sizeof(mad_link_t));
	  CTRL_ALLOC(in->link);
	  out->link = TBX_MALLOC(out->nb_link * sizeof(mad_link_t));
	  CTRL_ALLOC(out->link);
	  
	  for (link_id = 0;
	       link_id < in->nb_link;
	       link_id++)
	    {
	      p_mad_link_t   in_link  = &(in->link[link_id]);
	      p_mad_link_t   out_link = &(out->link[link_id]);
	      
	      in_link->connection            = in;
	      in_link->id                    = link_id;
	      in_link->specific              = NULL;
	      
	      out_link->connection            = out;
	      out_link->id                    = link_id;
	      out_link->specific              = NULL;
	      
	      if (interface->link_init)
		{
		  interface->link_init(in_link);
		  interface->link_init(out_link);
		}
	    }

	  if (is_forward)
	    {
	      p_mad_link_t   out_link;

	      out->fwd_link = TBX_MALLOC(sizeof(mad_link_t));
	      CTRL_ALLOC(out->fwd_link);

	      out_link = out->fwd_link;

	      out_link->connection            = out;
	      out_link->specific              = NULL;

	      fwd_interface->link_init(out_link);
	    }
	  else
	    {
	      p_mad_link_t   in_link;

	      in->fwd_link = TBX_MALLOC(sizeof(mad_link_t));
	      CTRL_ALLOC(in->fwd_link);

	      in_link = in->fwd_link;

	      in_link->connection            = in;
	      in_link->specific              = NULL;

	      fwd_interface->link_init(in_link);
	    }
	}
    }
  
  if (interface->before_open_channel)
    interface->before_open_channel(channel);
  
  if (interface->accept && interface->connect)
    {
      if (driver->connection_type == mad_bidirectional_connection)
	{
	  for (host = 0; host < rank; host++)
	    {
	      if (channel->input_connection[host].active)
		interface->accept(channel);
	    }      

	  for(host = rank + 1; host < size; host++)
	    {
	      if (channel->output_connection[host].active)
		interface->connect(&(channel->output_connection[host]));
	    }
	}
      else
	{
	  for(host = 0; host < size; host++)
	    {
	      if (host == rank)
		{
		  ntbx_host_id_t remote_host;

		  for (remote_host = 0; remote_host < size; remote_host++)
		    {
		      if (remote_host != rank)
			{
			  if(channel->input_connection[remote_host].active)
			    interface->accept(channel);
			}
		    }
		}
	      else
		{
		  if (channel->output_connection[host].active)
		    interface->connect(&(channel->output_connection[host]));
		}
	    }
	}
    }
  
  if (interface->after_open_channel)
    interface->after_open_channel(channel);

  tbx_append_list(&(madeleine->channel), channel);
  TBX_UNLOCK_SHARED(adapter);
  LOG_OUT();
  
  return channel;
}
#else /* MAD_FORWARDING */
p_mad_channel_t
mad_open_channel(p_mad_madeleine_t madeleine,
		 mad_adapter_id_t  adapter_id)
{
  p_mad_configuration_t      configuration = madeleine->configuration;
  p_mad_adapter_t            adapter       =
    &(madeleine->adapter[adapter_id]);
  p_mad_driver_t             driver        = adapter->driver;
  p_mad_driver_interface_t   interface     = &(driver->interface);
  p_mad_channel_t            channel       = NULL;
  ntbx_host_id_t             size          = configuration->size;
  ntbx_host_id_t             rank          = configuration->local_host_id;
  ntbx_host_id_t             host;

  LOG_IN();
  TRACE("channel allocation");
  TBX_LOCK_SHARED(madeleine);
  TBX_LOCK_SHARED(adapter);
  channel = TBX_MALLOC(sizeof(mad_channel_t));
  CTRL_ALLOC(channel);

  TBX_INIT_SHARED(channel);
  channel->id             = madeleine->nb_channel++;
  channel->adapter        = adapter;
  channel->reception_lock = tbx_false;
  channel->specific       = NULL;

  if (interface->channel_init)
    interface->channel_init(channel);

  channel->input_connection = TBX_MALLOC(size * sizeof(mad_connection_t));
  CTRL_ALLOC(channel->input_connection);
  
  channel->output_connection = TBX_MALLOC(size * sizeof(mad_connection_t));
  CTRL_ALLOC(channel->output_connection);  

  for (host = 0; host < size; host++)
    {
      p_mad_connection_t in  = &(channel->input_connection[host]);
      p_mad_connection_t out = &(channel->output_connection[host]);
      mad_link_id_t      link_id;
      
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
      /* A voir */
      out->reverse        = in;
      out->way            = mad_outgoing_connection;
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
      
      in->link  = TBX_MALLOC( in->nb_link * sizeof(mad_link_t));
      CTRL_ALLOC(in->link);
      out->link = TBX_MALLOC(out->nb_link * sizeof(mad_link_t));
      CTRL_ALLOC(out->link);

      for (link_id = 0;
	   link_id < in->nb_link;
	   link_id++)
	{
	  p_mad_link_t   in_link  = &(in->link[link_id]);
	  p_mad_link_t   out_link = &(out->link[link_id]);
	  
	  in_link->connection            = in;
	  in_link->id                    = link_id;
	  in_link->specific              = NULL;

	  out_link->connection            = out;
	  out_link->id                    = link_id;
	  out_link->specific              = NULL;

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
	  for (host = 0; host < rank; host++)
	    interface->accept(channel);

	  for (host = rank + 1; host < size; host++)
	    interface->connect(&(channel->output_connection[host]));
	}
      else
	{
	  for(host = 0; host < size; host++)
	    {
	      if (host == rank)
		{
		  ntbx_host_id_t remote_host;

		  for (remote_host = 0; remote_host < size; remote_host++)
		    {
		      if (remote_host != rank)
			interface->accept(channel);
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
  TBX_UNLOCK_SHARED(adapter);
  TBX_UNLOCK_SHARED(madeleine);
  LOG_OUT();
  
  return channel;
}
#endif /* MAD_FORWARDING */

#ifdef MAD_FORWARDING
static
void
mad_foreach_close_channel(void *object)
{
  p_mad_channel_t            channel       = object;
  p_mad_adapter_t            adapter       = channel->adapter;
  p_mad_driver_t             driver        = adapter->driver;
  p_mad_driver_interface_t   interface     = &(driver->interface);
  p_mad_madeleine_t          madeleine     = driver->madeleine;
  p_mad_configuration_t      configuration = madeleine->configuration;
  ntbx_host_id_t             size          = configuration->size;
  ntbx_host_id_t             rank          = configuration->local_host_id;
  ntbx_host_id_t             host;

  LOG_IN();

  TBX_LOCK_SHARED(adapter);
  TBX_LOCK_SHARED(channel);

  if (interface->before_close_channel)
    interface->before_close_channel(channel);

  if (interface->disconnect)
    {      
      if (driver->connection_type == mad_bidirectional_connection)
	{
	  for (host = 0; host < rank; host++)
	    {
	      if(channel->input_connection[host].active)
		interface->disconnect(&(channel->input_connection[host]));
	    }

	  for (host = rank + 1; host < size; host++)
	    {
	      if(channel->output_connection[host].active)
		interface->disconnect(&(channel->output_connection[host]));
	    }
	}
      else
	{
	  for(host = 0; host < size; host++)
	    {
	      if (host == rank)
		{
		  ntbx_host_id_t remote_host;

		  for (remote_host = 0; remote_host < size; remote_host++)
		    {
		      if (remote_host != rank)
			{
			  if(channel->input_connection[host].active)
			    interface->
			      disconnect(&(channel
					   ->input_connection[remote_host]));
			}
		    }
		}
	      else
		{
		  if(channel->output_connection[host].active)
		    interface->disconnect(&(channel->output_connection[host]));
		}
	    }
	}
    }

  if (interface->after_close_channel)
    interface->after_close_channel(channel);

  for (host = 0; host < size; host++)
    {
      if(channel->input_connection[host].active)
	{
	  p_mad_connection_t in  = &(channel->input_connection[host]);
	  p_mad_connection_t out = &(channel->output_connection[host]);
	  mad_link_id_t      link_id;

	  if (interface->link_exit)
	    {
	      for (link_id = 0; link_id < in->nb_link; link_id++)
		{
		  p_mad_link_t in_link  = &(in->link[link_id]);
		  p_mad_link_t out_link = &(out->link[link_id]);
	      
		  interface->link_exit(out_link);
		  interface->link_exit(in_link);
		}
	    }
	  else
	    {
	      if (out->link->specific)
		{
		  TBX_FREE(out->link->specific);
		  out->link->specific = NULL;
		}

	      if (in->link->specific)
		{
		  TBX_FREE(in->link->specific);
		  in->link->specific = NULL;
		}	  
	    }
      
	  TBX_FREE(out->link);
	  out->link = NULL;
	  
	  TBX_FREE(in->link);
	  in->link = NULL;

	  if (interface->connection_exit)
	    {
	      interface->connection_exit(in, out);
	    }
	  else
	    {
	      if (out->specific)
		{	      
		  TBX_FREE(out->specific);

		  if (in->specific == out->specific)
		    {
		      in->specific = NULL;  
		    }
		  out->specific = NULL;
		}

	      if (in->specific)
		{
		  TBX_FREE(in->specific);
		  in->specific = NULL;
		}	  
	    }
	}
    }
  
  TBX_FREE(channel->output_connection);
  channel->output_connection = NULL;
  
  TBX_FREE(channel->input_connection);
  channel->input_connection  = NULL;

  if (interface->channel_exit)
    {
      interface->channel_exit(channel);
    }
  else
    {
      if (channel->specific)
	{
	  TBX_FREE(channel->specific);
	  channel->specific = NULL;
	}
    }

  TBX_FREE(channel);
  channel = NULL;
  
  /* Note: the channel is never unlocked since it is being destroyed */
  TBX_UNLOCK_SHARED(adapter);
  LOG_OUT();
}
#else /* MAD_FORWARDING */
static
void
mad_foreach_close_channel(void *object)
{
  p_mad_channel_t            channel       = object;
  p_mad_adapter_t            adapter       = channel->adapter;
  p_mad_driver_t             driver        = adapter->driver;
  p_mad_driver_interface_t   interface     = &(driver->interface);
  p_mad_madeleine_t          madeleine     = driver->madeleine;
  p_mad_configuration_t      configuration = madeleine->configuration;
  ntbx_host_id_t             size          = configuration->size;
  ntbx_host_id_t             rank          = configuration->local_host_id;
  ntbx_host_id_t             host;

  LOG_IN();

  TRACE("channel deallocation");
  TBX_LOCK_SHARED(adapter);
  TBX_LOCK_SHARED(channel);

  if (interface->before_close_channel)
    interface->before_close_channel(channel);

  if (interface->disconnect)
    {      
      if (driver->connection_type == mad_bidirectional_connection)
	{
	  for (host = 0; host < rank; host++)
	    interface->disconnect(&(channel->input_connection[host]));

	  for (host = rank + 1; host < size; host++)
	      interface->disconnect(&(channel->output_connection[host]));
	}
      else
	{
	  for (host = 0; host < size; host++)
	    {
	      if (host == rank)
		{
		  ntbx_host_id_t remote_host;

		  for (remote_host = 0 ; remote_host < size; remote_host++)
		    {
		      if (remote_host != rank)
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

  for (host = 0; host < size; host++)
    {
      p_mad_connection_t   in  = &(channel->input_connection[host]);
      p_mad_connection_t   out = &(channel->output_connection[host]);
      mad_link_id_t        link_id;

      if (interface->link_exit)
	{
	  for (link_id = 0; link_id < in->nb_link; link_id++)
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
	      TBX_FREE(out->link->specific);
	      out->link->specific = NULL;
	    }

	  if (in->link->specific)
	    {
	      TBX_FREE(in->link->specific);
	      in->link->specific = NULL;
	    }	  
	}
      
      TBX_FREE(out->link);
      out->link = NULL;
      
      TBX_FREE(in->link);
      in->link = NULL;
      
      if (interface->connection_exit)
	{
	  interface->connection_exit(in, out);
	}
      else
	{
	  if (out->specific)
	    {	      
	      TBX_FREE(out->specific);

	      if (in->specific == out->specific)
		{
		  in->specific = NULL;  
		}

	      out->specific = NULL;
	    }

	  if (in->specific)
	    {
	      TBX_FREE(in->specific);
	      in->specific = NULL;
	    }	  
	}
    }

  TBX_FREE(channel->output_connection);
  channel->output_connection = NULL;
  
  TBX_FREE(channel->input_connection);
  channel->input_connection = NULL;

  if (interface->channel_exit)
    {
      interface->channel_exit(channel);
    }
  else
    {
      if (channel->specific)
	{
	  TBX_FREE(channel->specific);
	  channel->specific = NULL;
	}
    }
  
  TBX_FREE(channel);
  channel = NULL;
  
  /* Note: the channel is never unlocked since it is being destroyed */
  TBX_UNLOCK_SHARED(adapter);
  LOG_OUT();
}
#endif /* MAD_FORWARD */

void
mad_close_channels(p_mad_madeleine_t madeleine)
{
  LOG_IN();
  tbx_foreach_destroy_list(&(madeleine->channel), mad_foreach_close_channel);
  LOG_OUT();
}
