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
$Log: mad_bip.c,v $
Revision 1.2  2000/06/06 17:43:21  rnamyst
small evolution of pm2/mad2/bip ;-)

______________________________________________________________________________
*/

/*
 * Mad_bip.c
 * =========
 */

#define MARCEL_POLL_WA


/*
 * headerfiles
 * -----------
 */

/* protocol specific header files */
#include <bip.h>

/* MadII header files */
#include "madeleine.h"

/* system header files */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h> 
#include <netdb.h>

/*
 * macros and constants definition
 * -------------------------------
 */
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif

#define MAD_BIP_SERVICE_TAG      0
#define MAD_BIP_REPLY_TAG        1
#define MAD_BIP_TRANSFER_TAG     2
#define MAD_BIP_FLOW_CONTROL_TAG 3
/* The following macro should always return the number of tags */
#define MAD_BIP_NB_OF_TAGS       4

#define MAD_BIP_FIRST_CHANNEL    0
static  int new_channel = MAD_BIP_FIRST_CHANNEL ;

#define BIP_CHAR                 0
#define BIP_BYTE                 1
#define BIP_INT                  2

#define BIP_ANY_SOURCE           -1

#define BIP_BOTTOM               (void *) -1

#define BIP_MAX_CREDITS          10

#define BIP_SMALL_MESSAGE        64
#define MAX_BIP_REQ              64

typedef struct {
  int host_id;
  int credits;
} new_message_t;

typedef struct {
  int credits;
} cred_message_t;

typedef struct {
  int credits;
} ack_message_t;

typedef struct
{
  TBX_SHARED;
  int nb_adapter;
} mad_bip_driver_specific_t, *p_mad_bip_driver_specific_t;

typedef struct
{
} mad_bip_adapter_specific_t, *p_mad_bip_adapter_specific_t;

typedef struct
{
  int communicator ; /* It's a BIP tag! */
  int small_buffer [BIP_SMALL_MESSAGE] ;
  int message_size ;

  int *credits_a_rendre ;
  int *credits_disponibles ;
#ifdef MARCEL
  int            *ack_recus ;
  int            request_credits ;
  int            credits_rendus ;

  marcel_mutex_t mutex;
  marcel_cond_t *cond_cred; // tableau
  marcel_sem_t *sem_ack;    // tableau
  int cred_request;
  cred_message_t cred_msg;
  int ack_request;
  ack_message_t ack_msg;
#endif
} mad_bip_channel_specific_t, *p_mad_bip_channel_specific_t;

typedef struct
{
  int begin_receive ;
  int begin_send ;
} mad_bip_connection_specific_t, *p_mad_bip_connection_specific_t;

typedef struct
{
} mad_bip_link_specific_t, *p_mad_bip_link_specific_t;

#ifdef MARCEL
static p_mad_bip_driver_specific_t mad_bip_driver_specific;
static marcel_mutex_t _bip_mutex;

static __inline__ void bip_lock(void)
{
  marcel_mutex_lock(&_bip_mutex);
}

static __inline__ void bip_unlock(void)
{
  marcel_mutex_unlock(&_bip_mutex);
}

static int bip_asend (int host, int tag, int *message, int size)
{
  int request ;
  int status ;

  bip_lock();
  request = bip_tisend(host, tag, message, size);
  status = bip_stest(request);
  bip_unlock();

  while(status == 0) {
    marcel_yield();

    bip_lock();
    status = bip_stest (request);    
    bip_unlock();
  }

  return status;
}

static int bip_arecv (int tag, int *message, int size, int *host)
{
  int request;
  int status;

  bip_lock();
  request = bip_tirecv(tag, message, size);
  status = bip_rtestx(request, host);
  bip_unlock();

  while(status == -1) {
    marcel_yield();

    bip_lock();
    status = bip_rtestx(request, host);
    bip_unlock();
  }
  return status;
}

static __inline__ void credits_received(p_mad_bip_channel_specific_t p,
					int host_id, int credits)
{
  marcel_mutex_lock(&p->mutex);
  fprintf(stderr, "Credits received from host %d\n", host_id);
  p->credits_disponibles[host_id] += credits;
  marcel_cond_signal(&p->cond_cred[host_id]);
  marcel_mutex_unlock(&p->mutex);
}

static __inline__ void wait_ack(p_mad_bip_channel_specific_t p, int host_id)
{
  marcel_sem_P(&p->sem_ack[host_id]);
}

static __inline__ void send_ack(p_mad_bip_channel_specific_t p, int host_id)
{
  ack_message_t ack_msg;

  marcel_mutex_lock(&p->mutex);
  ack_msg.credits = p->credits_a_rendre[host_id];
  p->credits_a_rendre[host_id] = 0;
  marcel_mutex_unlock(&p->mutex);

  bip_asend(host_id,
	    p->communicator + MAD_BIP_REPLY_TAG,
	    (int*)&ack_msg,
	    sizeof(ack_msg)/sizeof(int));
}

#endif

static void wait_credits(p_mad_bip_channel_specific_t p, int host_id) 
{
#ifdef MARCEL

  marcel_mutex_lock(&p->mutex);

  while(p->credits_disponibles[host_id] == 0) {
    fprintf(stderr, "Thread %p is waiting for credits from host %d\n",
	    marcel_self(), host_id);
    marcel_cond_wait(&p->cond_cred[host_id], &p->mutex);
  }

  fprintf(stderr, "Thread %p has sufficient credits to talk to host %d\n",
	  marcel_self(), host_id);
  p->credits_disponibles[host_id]--;

  marcel_mutex_unlock(&p->mutex);

#else
 while (p->credits_disponibles [host_id] == 0)
   {
     int remote ;
     int cred ;

     bip_trecvx (MAD_BIP_FLOW_CONTROL_TAG, (int *) &cred, 1, &remote) ;
     p->credits_disponibles [remote] += cred ;
   }
 p->credits_disponibles [host_id] -- ;
#endif
}

static void give_back_credits (p_mad_bip_channel_specific_t p, int host_id)
{
  cred_message_t msg;

#ifdef MARCEL

  marcel_mutex_lock(&p->mutex);

  p->credits_a_rendre[host_id]++;

  if ((p->credits_a_rendre[host_id]) == (BIP_MAX_CREDITS-1)) {

    msg.credits = p->credits_a_rendre[host_id];
    p->credits_a_rendre[host_id] = 0 ;

    marcel_mutex_unlock(&p->mutex);

    bip_asend(host_id, MAD_BIP_FLOW_CONTROL_TAG,
	      (int *)&msg, sizeof(cred_message_t)/sizeof(int));
  } else
    marcel_mutex_unlock(&p->mutex);

#else

  p->credits_a_rendre [host_id] ++ ;
  if ((p->credits_a_rendre [host_id]) == (BIP_MAX_CREDITS-1))
    {
     bip_tsend (host_id, MAD_BIP_FLOW_CONTROL_TAG, (int *) &(p->credits_a_rendre [host_id]), 1) ;
     p->credits_a_rendre [host_id] = 0 ;
    }

#endif
}

void
mad_bip_register(p_mad_driver_t driver)
{
  p_mad_driver_interface_t interface;

  LOG_IN();
  /* Driver module registration code */
  interface = &(driver->interface);
  
  driver->connection_type = mad_unidirectional_connection;

  /* Not used for now, but might be used in the future for
     dynamic buffer allocation */
  driver->buffer_alignment = 32;
  
  interface->driver_init                = mad_bip_driver_init;
  interface->adapter_init               = mad_bip_adapter_init;
  interface->adapter_configuration_init = NULL;
  interface->channel_init               = mad_bip_channel_init;
  interface->before_open_channel        = NULL;
  interface->connection_init            = mad_bip_connection_init;
  interface->link_init                  = mad_bip_link_init;
  interface->accept                     = NULL;
  interface->connect                    = NULL;
  interface->after_open_channel         = NULL;
  interface->before_close_channel       = NULL;
  interface->disconnect                 = NULL;
  interface->after_close_channel        = NULL;
  interface->link_exit                  = NULL;
  interface->connection_exit            = NULL;
  interface->channel_exit               = mad_bip_channel_exit;
  interface->adapter_exit               = mad_bip_adapter_exit;
  interface->driver_exit                = mad_bip_driver_exit;
  interface->choice                     = mad_bip_choice;
  interface->get_static_buffer          = NULL;
  interface->return_static_buffer       = NULL;
  interface->new_message                = mad_bip_new_message;
  interface->receive_message            = mad_bip_receive_message;
  interface->send_buffer                = mad_bip_send_buffer;
  interface->receive_buffer             = mad_bip_receive_buffer;
  interface->send_buffer_group          = mad_bip_send_buffer_group;
  interface->receive_sub_buffer_group   = mad_bip_receive_sub_buffer_group;
  interface->external_spawn_init        = mad_bip_external_spawn_init;
  interface->configuration_init         = mad_bip_configuration_init;
  interface->send_adapter_parameter     = mad_bip_send_adapter_parameter;
  interface->receive_adapter_parameter  = mad_bip_receive_adapter_parameter;;
  LOG_OUT();
}

void
mad_bip_driver_init(p_mad_driver_t driver)
{
  p_mad_bip_driver_specific_t driver_specific;

  LOG_IN();
  /* Driver module initialization code
     Note: called only once, just after
     module registration */
  driver_specific = TBX_MALLOC(sizeof(mad_bip_driver_specific_t));
  CTRL_ALLOC(driver_specific);
  driver->specific = driver_specific;
  TBX_INIT_SHARED(driver_specific);
  driver_specific->nb_adapter = 0;

#ifdef MARCEL
  mad_bip_driver_specific = driver_specific;

  marcel_mutex_init(&_bip_mutex, NULL);
#endif
  LOG_OUT();
}

void
mad_bip_adapter_init(p_mad_adapter_t adapter)
{
  p_mad_driver_t                 driver;
  p_mad_bip_driver_specific_t    driver_specific;
  p_mad_bip_adapter_specific_t   adapter_specific;
  
  LOG_IN();

  /* Adapter initialization code (part I)
     Note: called once for each adapter */
  driver          = adapter->driver;
  driver_specific = driver->specific;
  if (driver_specific->nb_adapter)
    FAILURE("BIP adapter already initialized");

  if (adapter->name == NULL)
    {
      adapter->name = TBX_MALLOC(10);
      CTRL_ALLOC(adapter->name);
      sprintf(adapter->name, "BIP%d", driver_specific->nb_adapter);
    }

  driver_specific->nb_adapter++;

  adapter_specific = TBX_MALLOC(sizeof(mad_bip_adapter_specific_t));
  CTRL_ALLOC(adapter_specific);

  adapter->specific = adapter_specific ;

  adapter->parameter = TBX_MALLOC(10);
  CTRL_ALLOC(adapter->parameter);
  sprintf(adapter->parameter, "-"); 
  LOG_OUT();
}

void
mad_bip_channel_init(p_mad_channel_t channel)
{
  p_mad_bip_channel_specific_t channel_specific;
  unsigned i;
  unsigned size = bip_numnodes;

  LOG_IN();

  /* Channel initialization code
     Note: called once for each new channel */
  channel_specific = TBX_MALLOC(sizeof(mad_bip_channel_specific_t));
  CTRL_ALLOC(channel_specific);

  channel_specific->credits_disponibles = (int *) TBX_MALLOC (size * sizeof (int)) ;
  channel_specific->credits_a_rendre = (int *) TBX_MALLOC (size * sizeof (int)) ;
#ifdef MARCEL
  channel_specific->ack_recus = (int *) TBX_MALLOC (size * sizeof (int)) ;
  channel_specific->cond_cred = (marcel_cond_t *)TBX_MALLOC(size*sizeof(marcel_cond_t));
  channel_specific->sem_ack = (marcel_sem_t *)TBX_MALLOC(size*sizeof(marcel_sem_t));
  marcel_mutex_init(&channel_specific->mutex, NULL);
#endif

  for (i=0; i < size; i++)
    {
     channel_specific->credits_a_rendre [i] = 0 ;
     channel_specific->credits_disponibles [i] = BIP_MAX_CREDITS ; 
#ifdef MARCEL
     channel_specific->ack_recus [i] = 0 ;
     marcel_cond_init(&channel_specific->cond_cred[i], NULL);
     marcel_sem_init(&channel_specific->sem_ack[i], 0);
#endif
    }

  TBX_LOCK_SHARED(mad_bip_driver_specific); 

  channel_specific->communicator = new_channel ;
  new_channel += MAD_BIP_NB_OF_TAGS ;
  channel_specific->message_size = 0 ;

  TBX_UNLOCK_SHARED(mad_bip_driver_specific) ; 
  channel->specific = channel_specific ;

#ifdef MARCEL
  bip_lock();

  channel_specific->cred_request =
    bip_tirecv(channel_specific->communicator +
	       MAD_BIP_FLOW_CONTROL_TAG,
	       (int *)&channel_specific->cred_msg,
	       sizeof(cred_message_t)/sizeof(int));

  channel_specific->ack_request =
    bip_tirecv(channel_specific->communicator +
	       MAD_BIP_REPLY_TAG,
	       (int *)&channel_specific->ack_msg,
	       sizeof(ack_message_t)/sizeof(int));
#endif

  LOG_OUT() ;
}

void
mad_bip_connection_init(p_mad_connection_t in, p_mad_connection_t out)
{
  p_mad_bip_connection_specific_t connection_specific ;
  
  LOG_IN();

  connection_specific = TBX_MALLOC(sizeof(mad_bip_connection_specific_t));
  CTRL_ALLOC (connection_specific) ;

  connection_specific->begin_send    = 0 ;
  connection_specific->begin_receive = 0 ;
  
  in->specific = connection_specific ;
  in->nb_link = 2;
  out->specific = connection_specific ;
  out->nb_link = 2;
  LOG_OUT();
}


void 
mad_bip_link_init(p_mad_link_t lnk)
{

  LOG_IN();
  /* Link initialization code */

  lnk->link_mode   = mad_link_mode_buffer;
  lnk->buffer_mode = mad_buffer_mode_dynamic;
  lnk->group_mode  = mad_group_mode_split;
  LOG_OUT();
}

void
mad_bip_channel_exit(p_mad_channel_t channel)
{  
  p_mad_bip_channel_specific_t channel_specific = channel->specific;
  
  LOG_IN();

  TBX_FREE(channel_specific);
  channel->specific = NULL;

  LOG_OUT();
}

void
mad_bip_adapter_exit(p_mad_adapter_t adapter)
{
  LOG_IN();
  /* Code to execute to clean up an adapter */
  TBX_FREE(adapter->parameter);
  TBX_FREE(adapter->name);
  TBX_FREE(adapter->specific);
  LOG_OUT();
}

void
mad_bip_driver_exit(p_mad_driver_t driver)
{
  LOG_IN();
  /* Code to execute to clean up a driver */
  /* PM2_LOCK_SHARED(mad_bip_driver_specific); */
  /* MPI_Finalize(); */
  TBX_FREE (driver->specific);
#ifdef MARCEL
  mad_bip_driver_specific = NULL;
#endif /* MARCEL */
  driver->specific = NULL;
  LOG_OUT();
}


void
mad_bip_new_message(p_mad_connection_t connection)
{
  p_mad_bip_connection_specific_t connection_specific = connection->specific ; 

  LOG_IN();
  /* Code to prepare a new message */

  connection_specific->begin_send = 1 ;

  LOG_OUT();

  return ;

}

p_mad_connection_t
mad_bip_receive_message(p_mad_channel_t channel)
{
  p_mad_bip_channel_specific_t channel_specific = channel->specific;
  p_mad_connection_t    connection       = NULL;
  p_mad_bip_connection_specific_t connection_specific = NULL ;
  int host ;

  LOG_IN();

#ifdef MARCEL

  channel_specific->message_size = bip_arecv (
		 channel_specific->communicator+MAD_BIP_SERVICE_TAG, 
		 (int *) channel_specific->small_buffer, 
		 BIP_SMALL_MESSAGE,
		 &host
		 );
      
#else
  channel_specific->message_size = bip_trecvx (
		 channel_specific->communicator+MAD_BIP_SERVICE_TAG, 
		 (int *) channel_specific->small_buffer, 
		 BIP_SMALL_MESSAGE,
		 &host
		 );
#endif
  give_back_credits (channel_specific, host) ;

  connection = &(channel->input_connection[host]);
  connection_specific = connection->specific ;      
  connection_specific->begin_receive = 1 ;
 
  LOG_OUT();

  return connection;
}


/* External spawn support functions */

void
mad_bip_external_spawn_init(p_mad_adapter_t   spawn_adapter
			    __attribute__ ((unused)),
			    int              *argc,
			    char            **argv)
{
  LOG_IN();

  bip_init () ;

  LOG_OUT();
}

void
mad_bip_configuration_init(p_mad_adapter_t       spawn_adapter
			   __attribute__ ((unused)),
			   p_mad_configuration_t configuration)
{
  ntbx_host_id_t               host_id;
  int                          rank;
  int                          size;

  LOG_IN();
  /* Code to get configuration information from the external spawn adapter */

  rank = bip_mynode ;
  size = bip_numnodes ;

  configuration->local_host_id = (ntbx_host_id_t)rank;
  configuration->size          = (mad_configuration_size_t)size;
  configuration->host_name     =
    TBX_MALLOC(configuration->size * sizeof(char *));
  CTRL_ALLOC(configuration->host_name);

  for (host_id = 0;
       host_id < configuration->size;
       host_id++)
    {
      configuration->host_name[host_id] = TBX_MALLOC(MAXHOSTNAMELEN + 1);
      CTRL_ALLOC(configuration->host_name[host_id]);

      if (host_id == rank)
	{
	  ntbx_host_id_t remote_host_id;
	  
	  gethostname(configuration->host_name[host_id], MAXHOSTNAMELEN);
	  
	  for (remote_host_id = 0;
	       remote_host_id < configuration->size;
	       remote_host_id++)
	    {
	      if (remote_host_id != rank)
		{
                  bip_tsend (remote_host_id, MAD_BIP_SERVICE_TAG, 
			     (int *)configuration->host_name[host_id], MAXHOSTNAMELEN/sizeof(int)) ;     
		}
	    }
	}
      else
	{
         bip_trecv (MAD_BIP_SERVICE_TAG, (int *)configuration->host_name[host_id],
		    MAXHOSTNAMELEN/sizeof(int)) ;
	}      
    }
  LOG_OUT();
}

void
mad_bip_send_adapter_parameter(p_mad_adapter_t   spawn_adapter
			       __attribute__ ((unused)),
			       ntbx_host_id_t    remote_host_id,
			       char             *parameter)
{  
  printf ("NOT YET IMPLEMENTED call a mad_bip_send_adapter_parameter\n") ;
  fflush (stdout) ;

  exit (-1) ;

  LOG_IN();

  LOG_OUT();
}

void
mad_bip_receive_adapter_parameter(p_mad_adapter_t   spawn_adapter
				  __attribute__ ((unused)),
				  char            **parameter)
{

  printf ("NOT YET IMPLEMENTED call a mad_bip_receive_adapter_parameter\n") ;
  fflush (stdout) ;
  exit (-1) ;
  
  LOG_IN();

  LOG_OUT();
}


p_mad_link_t
mad_bip_choice(p_mad_connection_t connection,
		 size_t             size         __attribute__ ((unused)),
		 mad_send_mode_t    send_mode    __attribute__ ((unused)),
		 mad_receive_mode_t receive_mode __attribute__ ((unused)))
{
  if (size <= 256)
    {
      return &connection->link[0];
    }
  else
    {
      return &connection->link[1];
    }
}


static void
mad_bip_send_short_buffer(p_mad_link_t     lnk,
		    p_mad_buffer_t   buffer)
{
  p_mad_connection_t           connection       = lnk->connection;
  p_mad_bip_connection_specific_t connection_specific = connection->specific ;
  p_mad_channel_t              channel          = connection->channel;
  p_mad_bip_channel_specific_t channel_specific = channel->specific;


  int status ;

  LOG_IN();
  /* Code to send one buffer */

  if (connection_specific->begin_send == 1)
    {

      wait_credits(channel_specific, connection->remote_host_id) ;

#ifdef MARCEL

      status = bip_asend (
			  connection->remote_host_id,
			  channel_specific->communicator+MAD_BIP_SERVICE_TAG,
			  buffer->buffer,
			  (buffer->length)/sizeof(int)
			 );
#else
      bip_tsend (
		 connection->remote_host_id,
		 channel_specific->communicator+MAD_BIP_SERVICE_TAG,
		 buffer->buffer,
		 (buffer->length)/sizeof(int)
		 ) ;
#endif
      connection_specific->begin_send = 0 ;
    }
  else
    {

#ifdef MARCEL

      wait_ack(channel_specific, connection->remote_host_id);

      wait_credits(channel_specific, connection->remote_host_id) ;

      status = bip_asend(connection->remote_host_id,
			 channel_specific->communicator+MAD_BIP_TRANSFER_TAG,
			 buffer->buffer,
			 (buffer->length)/sizeof(int));
#else
      bip_trecv (
		 channel_specific->communicator+MAD_BIP_REPLY_TAG,
		 &credits_rendus,
		 1
		) ;

      wait_credits (channel_specific, connection->remote_host_id) ;

      bip_tsend (
		 connection->remote_host_id,
		 channel_specific->communicator+MAD_BIP_TRANSFER_TAG,
		 buffer->buffer,
		 (buffer->length)/sizeof(int)
		) ;
#endif

    }
  buffer->bytes_read = buffer->length;

  LOG_OUT();
}



static void
mad_bip_receive_short_buffer(p_mad_link_t     lnk,
		       p_mad_buffer_t  buffer)
{
  p_mad_connection_t                     connection       = lnk->connection;
  p_mad_bip_connection_specific_t       connection_specific = connection->specific ;
  p_mad_channel_t              channel          = connection->channel;
  p_mad_bip_channel_specific_t channel_specific = channel->specific;

#ifdef MARCEL
  int  host ;
#endif

  int  status ;
  int  cred 

  LOG_IN();

  if (connection_specific->begin_receive == 1)
    {
      memcpy (buffer->buffer, channel_specific->small_buffer, (channel_specific->message_size)*sizeof(int)) ;
      connection_specific->begin_receive = 0 ;
      status = channel_specific->message_size ;
    }
  else
    {
#ifdef MARCEL

#if 0
      marcel_sem_P (&channel_specific->sem_credits) ;

        cred = channel_specific->credits_a_rendre [connection->remote_host_id] ;
        channel_specific->credits_a_rendre [connection->remote_host_id] = 0 ;

      marcel_sem_V (&channel_specific->sem_credits) ;

      bip_asend (
		 connection->remote_host_id,
		 channel_specific->communicator+MAD_BIP_REPLY_TAG,
		 &cred,
		 1
               ) ;  

      status = bip_arecv (
		          channel_specific->communicator+MAD_BIP_TRANSFER_TAG,
		          buffer->buffer,
		          (buffer->length)/sizeof(int),
		          &host
	                 ) ;
#endif

#else

      cred = channel_specific->credits_a_rendre [connection->remote_host_id] ;
      channel_specific->credits_a_rendre [connection->remote_host_id] = 0 ;

      bip_tsend (
		 connection->remote_host_id,
		 channel_specific->communicator+MAD_BIP_REPLY_TAG,
		 &cred,
		 1
		) ;

      status = bip_trecv (
		          channel_specific->communicator+MAD_BIP_TRANSFER_TAG,
		          buffer->buffer,
		          (buffer->length)/sizeof(int)
		         ) ;
#endif

      give_back_credits (channel_specific, connection->remote_host_id) ;

      connection_specific->begin_receive = 0 ;      
    }

  buffer->bytes_written = buffer->length;

  LOG_OUT();
}


static void
mad_bip_send_short_buffer_group(p_mad_link_t           lnk,
			  p_mad_buffer_group_t   buffer_group)
{
  LOG_IN();
  /* Code to send a group of static buffers */
  if (!tbx_empty_list(&(buffer_group->buffer_list)))
    {
      tbx_list_reference_t ref;

      tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
      do
	{
	  mad_bip_send_short_buffer(lnk, tbx_get_list_reference_object(&ref));
	}
      while(tbx_forward_list_reference(&ref));
    }
  LOG_OUT();
}

static void
mad_bip_receive_short_buffer_group(p_mad_link_t           lnk,
				   p_mad_buffer_group_t   buffer_group)
{
  LOG_IN();
  /* Code to send a group of static buffers */
  if (!tbx_empty_list(&(buffer_group->buffer_list)))
    {
      tbx_list_reference_t ref;

      tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
      do
	{
	  mad_bip_receive_short_buffer(lnk, tbx_get_list_reference_object(&ref));
	}
      while(tbx_forward_list_reference(&ref));
    }
  LOG_OUT();
}

static void
mad_bip_send_long_buffer(p_mad_link_t lnk,
			 p_mad_buffer_t buffer)
{
#if 0
  p_mad_connection_t connection = lnk->connection;
  p_mad_bip_connection_specific_t connection_specific = connection->specific;
  p_mad_channel_t channel = connection->channel;
  p_mad_bip_channel_specific_t channel_specific = channel->specific;
  p_mad_configuration_t configuration = 
    &(channel->adapter->driver->madeleine->configuration);

  int credits_rendus ;
#ifdef MARCEL
  int  status;
  int  host ;
#endif
  
  LOG_IN();
  /* Code to send one buffer */

  if (connection_specific->begin_send == 1)
    {
      new_message_t m ;

      wait_credits (channel_specific, connection->remote_host_id) ;

      m.host_id = configuration->local_host_id ;
      m.credits = channel_specific->credits_a_rendre [connection->remote_host_id] ;
      channel_specific->credits_a_rendre [connection->remote_host_id] = 0 ; 

#ifdef MARCEL
      bip_asend (
		 connection->remote_host_id, 
		 channel_specific->communicator+MAD_BIP_SERVICE_TAG,
		 (int *) &m,
		 sizeof(new_message_t)/sizeof(int)
		) ;

#else
      bip_tsend (
		 connection->remote_host_id, 
		 channel_specific->communicator+MAD_BIP_SERVICE_TAG,
		 (int *) &m,
		 sizeof(new_message_t)/sizeof(int)
		 ) ;
#endif
    }


#ifdef MARCEL
  bip_arecv (
	     channel_specific->communicator+MAD_BIP_REPLY_TAG,
	     &credits_rendus,
	     1,
	     &host
	    ) ;
#else
  bip_trecv (
	     channel_specific->communicator+MAD_BIP_REPLY_TAG,
             &credits_rendus,
             1
           ) ;
#endif

  channel_specific->credits_disponibles [connection->remote_host_id] += credits_rendus ;
  give_back_credits (channel_specific, connection->remote_host_id) ;

#ifdef MARCEL
  status = bip_asend (
		      connection->remote_host_id,
		      channel_specific->communicator+MAD_BIP_TRANSFER_TAG,
		      buffer->buffer,
		      (buffer->length)/sizeof(int)
		     ) ;
#else
  bip_tsend (
	     connection->remote_host_id,
	     channel_specific->communicator+MAD_BIP_TRANSFER_TAG,
	     buffer->buffer,
	     (buffer->length)/sizeof(int)
	    ) ;
#endif
  connection_specific->begin_send = 0 ;

  buffer->bytes_read = buffer->length;

  LOG_OUT();
#endif
}

static void
mad_bip_receive_long_buffer(p_mad_link_t     lnk,
			    p_mad_buffer_t   buffer)
{
#if 0
  p_mad_connection_t           connection                             = lnk->connection;
  p_mad_bip_connection_specific_t           connection_specific       = connection->specific;
  p_mad_channel_t              channel          = connection->channel;
  p_mad_bip_channel_specific_t channel_specific = channel->specific;
  int  cred  ;
  int  request ;

#ifdef MARCEL
  int  status;
#endif
  
  LOG_IN();
  /* Code to receive one buffer */

  if (connection_specific->begin_receive == 1)
    {
     new_message_t m ;

     memcpy (&m, channel_specific->small_buffer, sizeof (new_message_t)) ;
     channel_specific->credits_disponibles [connection->remote_host_id] += m.credits ;

     connection_specific->begin_receive = 0 ;
    }

#ifdef MARCEL
  lock_task () ;
     request = bip_tirecv (
			  channel_specific->communicator+MAD_BIP_TRANSFER_TAG,
			  buffer->buffer,
			  (buffer->length)/sizeof(int)
			 ) ;
  unlock_task () ;
#else
  request = bip_tirecv (
	              channel_specific->communicator+MAD_BIP_TRANSFER_TAG,
	              buffer->buffer,
	              (buffer->length)/sizeof(int)
	             ) ;
#endif

  wait_credits (channel_specific, connection->remote_host_id) ;

  cred = channel_specific->credits_a_rendre [connection->remote_host_id] ;
  channel_specific->credits_a_rendre [connection->remote_host_id] = 0 ;


#ifdef MARCEL
  bip_asend (
	     connection->remote_host_id,
	     channel_specific->communicator+MAD_BIP_REPLY_TAG,
	     &cred,
	     1
	    ) ;

  lock_task () ;
        status = bip_rtest (request) ;
  unlock_task () ;

  while (status == -1)
    {
     marcel_yield () ;

     lock_task () ;
        status = bip_rtest (request) ;
     unlock_task () ;
    }
#else 
  bip_tsend (
             connection->remote_host_id,
             channel_specific->communicator+MAD_BIP_REPLY_TAG,
             &cred,
	     1
	     ) ;

  bip_rwait (request) ;
#endif

  connection_specific->begin_receive = 0 ;

  buffer->bytes_written = buffer->length;

  LOG_OUT();
#endif
}

static void
mad_bip_send_long_buffer_group(p_mad_link_t lnk,
			       p_mad_buffer_group_t buffer_group)
{
  LOG_IN();
  /* Code to send a group of static buffers */
  if (!tbx_empty_list(&(buffer_group->buffer_list)))
    {
      tbx_list_reference_t ref;

      tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
      do
	{
	  mad_bip_send_long_buffer(lnk, tbx_get_list_reference_object(&ref));
	}
      while(tbx_forward_list_reference(&ref));
    }
  LOG_OUT();
}

static void
mad_bip_receive_long_buffer_group(p_mad_link_t           lnk,
				  p_mad_buffer_group_t   buffer_group)
{
  LOG_IN();
  /* Code to send a group of static buffers */
  if (!tbx_empty_list(&(buffer_group->buffer_list)))
    {
      tbx_list_reference_t ref;

      tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
      do
	{
	  mad_bip_receive_long_buffer(lnk, tbx_get_list_reference_object(&ref));
	}
      while(tbx_forward_list_reference(&ref));
    }
  LOG_OUT();
}

void
mad_bip_send_buffer(p_mad_link_t   lnk,
		      p_mad_buffer_t buffer)
{
  LOG_IN();
  if (lnk->id == 0)
    {
      mad_bip_send_short_buffer(lnk, buffer);
    }
  else if (lnk->id == 1)
    {
      mad_bip_send_long_buffer(lnk, buffer);
    }
  else
    FAILURE("Invalid link ID");
  LOG_OUT();
}

void 
mad_bip_receive_buffer(p_mad_link_t   lnk,
			 p_mad_buffer_t *buffer)
{
  LOG_IN();
  if (lnk->id == 0)
    {
      mad_bip_receive_short_buffer(lnk, *buffer);
    }
  else if (lnk->id == 1)
    {
      mad_bip_receive_long_buffer(lnk, *buffer);
    }
  else
    FAILURE("Invalid link ID");
  LOG_OUT();
  
}

void
mad_bip_send_buffer_group(p_mad_link_t         lnk,
			    p_mad_buffer_group_t buffer_group)
{
  LOG_IN();
  if (lnk->id == 0)
    {
      mad_bip_send_short_buffer_group(lnk, buffer_group);
    }
  else if (lnk->id == 1)
    {
      mad_bip_send_long_buffer_group(lnk, buffer_group);
    }
  else
    FAILURE("Invalid link ID");
  LOG_OUT();
}

void
mad_bip_receive_sub_buffer_group(p_mad_link_t           lnk,
				 tbx_bool_t             first_sub_group
				 __attribute__ ((unused)),
				 p_mad_buffer_group_t   buffer_group)
{
  LOG_IN();
  if (lnk->id == 0)
    {
      mad_bip_receive_short_buffer_group(lnk, buffer_group);
    }
  else if (lnk->id == 1)
    {
      mad_bip_receive_long_buffer_group(lnk, buffer_group);
    }
  else
    FAILURE("Invalid link ID");
  LOG_OUT();
}
