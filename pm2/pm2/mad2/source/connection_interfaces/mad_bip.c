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
Revision 1.3  2000/06/08 07:45:09  rnamyst
Premiere version qui marche ;-)

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

#include "pm2debug.h"

DEBUG_DECLARE(bip)

#undef DEBUG_NAME
#define DEBUG_NAME bip


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
  int dummy;
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
  int begin_receive, ack_sent;
  int begin_send, ack_received;
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

static __inline__ void credits_received(p_mad_bip_channel_specific_t p,
					int host_id, int credits)
{
  LOG_IN();

  if(credits) {
    marcel_mutex_lock(&p->mutex);

    TRACE("Credits received from host %d\n", host_id);

    p->credits_disponibles[host_id] += credits;
    marcel_cond_signal(&p->cond_cred[host_id]);
    marcel_mutex_unlock(&p->mutex);
  }

  LOG_OUT();
}

static __inline__ void ack_received(p_mad_bip_channel_specific_t p,
				    int host_id, int credits)
{
  LOG_IN();

  credits_received(p, host_id, credits);
  marcel_sem_V(&p->sem_ack[host_id]);

  LOG_OUT();
}

static __inline__ void wait_ack(p_mad_bip_channel_specific_t p, int host_id)
{
  LOG_IN();

  TRACE("Waiting for ack\n");
  marcel_sem_P(&p->sem_ack[host_id]);

  LOG_OUT();
}

static int bip_asend (int host, int tag, int *message, int size);

static __inline__ void send_ack(p_mad_bip_channel_specific_t p, int host_id)
{
  ack_message_t ack_msg;

  LOG_IN();

  ack_msg.credits = p->credits_a_rendre[host_id];
  p->credits_a_rendre[host_id] = 0;

  bip_asend(host_id,
	    p->communicator + MAD_BIP_REPLY_TAG,
	    (int*)&ack_msg,
	    sizeof(ack_msg)/sizeof(int));

  LOG_OUT();
}

static __inline__ void send_new(p_mad_bip_channel_specific_t p, int host_id)
{
  new_message_t new_msg;

  LOG_IN();

  new_msg.credits = p->credits_a_rendre[host_id];
  p->credits_a_rendre[host_id] = 0;

  bip_asend(host_id,
	    p->communicator + MAD_BIP_SERVICE_TAG,
	    (int*)&new_msg,
	    sizeof(new_msg)/sizeof(int));

  LOG_OUT();
}

static int bip_asend (int host, int tag, int *message, int size)
{
  int request ;
  int status ;

  LOG_IN();

  TRACE("bip_tisend(host=%d, tag=%d, size=%d)", host, tag, size);
  bip_lock();
  request = bip_tisend(host, tag, message, size);
  status = bip_stest(request);
  bip_unlock();

  while(status == 0) {
    marcel_yield();

    TRACE("bip_stest...");
    bip_lock();
    status = bip_stest (request);    
    bip_unlock();
  }

  LOG_OUT();

  return status;
}

static int bip_recv_post(int tag, int *message, int size)
{
  int request;

  LOG_IN();

  bip_lock();
  request = bip_tirecv(tag, message, size);
  bip_unlock();

  LOG_OUT();

  return request;
}

static int bip_recv_poll(p_mad_bip_channel_specific_t p, int request, int *host)
{
  int tmphost, which = 0, status = -1;

  LOG_IN();

  for(;;) {

    switch(which) {
    case 0 : { /* SERVICE ou TRANSFERT */
      bip_lock();
      status = bip_rtestx(request, host);
      bip_unlock();

      if(status != -1) {
	LOG_OUT();
	return status;
      }

      break;
    }
    case 1 : { /* ACKNOWLEDGEMENT */
      bip_lock();
      status = bip_rtestx(p->ack_request, &tmphost);
      bip_unlock();

      if(status != -1) {
	ack_received(p, tmphost, p->ack_msg.credits);

	bip_lock();
	p->ack_request =
	  bip_tirecv(p->communicator + MAD_BIP_REPLY_TAG,
		     (int *)&p->ack_msg,
		     sizeof(ack_message_t)/sizeof(int));
	bip_unlock();
      }
      break;
    }
    case 2 : { /* CREDITS (FLOW_CONTROL) */
      bip_lock();
      status = bip_rtestx(p->cred_request, &tmphost);
      bip_unlock();

      if(status != -1) {
	credits_received(p, tmphost, p->cred_msg.credits);

	bip_lock();
	p->cred_request =
	  bip_tirecv(p->communicator + MAD_BIP_FLOW_CONTROL_TAG,
		     (int *)&p->cred_msg,
		     sizeof(cred_message_t)/sizeof(int));
	bip_unlock();
      }
      break;
    }
    default:
      fprintf(stderr, "Oh my God! We should never execute that code!\n");
      break;
    }

    which = (which + 1) % 3;
    marcel_yield();
  };

  /* Never reached */
  fprintf(stderr, "Oh my God! We should never execute that code!\n");
  return -1;
}

static __inline__ int bip_arecv (p_mad_bip_channel_specific_t p,
				 int tag, int *message, int size, int *host)
{
  LOG_IN();
  return bip_recv_poll(p, bip_recv_post(tag, message, size), host);
  LOG_OUT();
}

#endif


static void wait_credits(p_mad_bip_channel_specific_t p, int host_id)
{
  LOG_IN();

#ifdef MARCEL

  TRACE("Checking credits\n");
  marcel_mutex_lock(&p->mutex);

  while(p->credits_disponibles[host_id] == 0) {
    TRACE("Thread %p is waiting for credits from host %d\n",
	 marcel_self(), host_id);
    marcel_cond_wait(&p->cond_cred[host_id], &p->mutex);
  }

  TRACE("Thread %p has %d credits to talk to host %d\n",
	marcel_self(), p->credits_disponibles[host_id], host_id);
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

 LOG_OUT();
}

static void give_back_credits (p_mad_bip_channel_specific_t p, int host_id)
{
  cred_message_t msg;

  LOG_IN();

#ifdef MARCEL

  marcel_mutex_lock(&p->mutex);

  p->credits_a_rendre[host_id]++;

  if ((p->credits_a_rendre[host_id]) == (BIP_MAX_CREDITS-1)) {

    msg.credits = p->credits_a_rendre[host_id];
    p->credits_a_rendre[host_id] = 0 ;

    marcel_mutex_unlock(&p->mutex);

    bip_asend(host_id, MAD_BIP_FLOW_CONTROL_TAG,
	      (int *)&msg, sizeof(cred_message_t)/sizeof(int));
  } else {
    marcel_mutex_unlock(&p->mutex);
    TRACE("Give_back_credits : %d credits to give, so forget it...",
	  p->credits_a_rendre[host_id]);
  }

#else

  p->credits_a_rendre [host_id] ++ ;
  if ((p->credits_a_rendre [host_id]) == (BIP_MAX_CREDITS-1))
    {
     bip_tsend (host_id, MAD_BIP_FLOW_CONTROL_TAG, (int *) &(p->credits_a_rendre [host_id]), 1) ;
     p->credits_a_rendre [host_id] = 0 ;
    }

#endif

  LOG_OUT();
}

void
mad_bip_register(p_mad_driver_t driver)
{
  p_mad_driver_interface_t interface;

  DEBUG_INIT(bip);

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
  bip_unlock();
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
  TRACE("Threads %p begins a new message", marcel_self());
  LOG_IN();

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

  channel_specific->message_size = bip_arecv(
		 channel_specific,
		 channel_specific->communicator+MAD_BIP_SERVICE_TAG, 
		 (int *) channel_specific->small_buffer, 
		 BIP_SMALL_MESSAGE,
		 &host
		 );
#else
  channel_specific->message_size = bip_trecvx(
		 channel_specific->communicator+MAD_BIP_SERVICE_TAG, 
		 (int *) channel_specific->small_buffer, 
		 BIP_SMALL_MESSAGE,
		 &host
		 );

  give_back_credits (channel_specific, host) ;

#endif

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
			     (int *)configuration->host_name[host_id],
			     MAXHOSTNAMELEN/sizeof(int)) ;     
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
  LOG_IN();

  printf ("NOT YET IMPLEMENTED call a mad_bip_send_adapter_parameter\n") ;
  fflush (stdout) ;

  exit (-1) ;

  LOG_OUT();
}

void
mad_bip_receive_adapter_parameter(p_mad_adapter_t   spawn_adapter
				  __attribute__ ((unused)),
				  char            **parameter)
{
  LOG_IN();

  printf ("NOT YET IMPLEMENTED call a mad_bip_receive_adapter_parameter\n") ;
  fflush (stdout) ;
  exit (-1) ;
  
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

  TRACE("Thread %p sends a buffer", marcel_self());
  LOG_IN();

  if (connection_specific->begin_send == 1)
    {
#ifdef MARCEL

      TRACE("Sending short buffer. Size = %d. First int = %d\n",
	    (buffer->length), ((int*)buffer->buffer)[0]);

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
      connection_specific->ack_received = 0;
    }
  else
    {

#ifdef MARCEL

      /* On n'envoie un ACK *que* pour le second paquet... */
      if(!connection_specific->ack_received) {
	wait_ack(channel_specific, connection->remote_host_id);
	connection_specific->ack_received = 1;
      } else {
	TRACE("Skipping wait_ack");
      }

      wait_credits(channel_specific, connection->remote_host_id);

      status = bip_asend(connection->remote_host_id,
			 channel_specific->communicator+MAD_BIP_TRANSFER_TAG,
			 buffer->buffer,
			 (buffer->length)/sizeof(int));

      TRACE("Sending short buffer. Size = %d. First int = %d\n",
	    (buffer->length), ((int*)buffer->buffer)[0]);

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

  LOG_IN();

  if (connection_specific->begin_receive == 1)
    {
      memcpy (buffer->buffer,
	      channel_specific->small_buffer,
	      channel_specific->message_size*sizeof(int)) ;

      status = channel_specific->message_size ;

      TRACE("Receiving short buffer. Size = %d. First int = %d\n",
	    channel_specific->message_size*sizeof(int),
	    ((int*)channel_specific->small_buffer)[0]);

      connection_specific->begin_receive = 0 ;      
      connection_specific->ack_sent = 0;
    }
  else
    {
#ifdef MARCEL

      /* On n'envoie un ACK *que* pour le second paquet... */
      if(!connection_specific->ack_sent) {
	send_ack(channel_specific, connection->remote_host_id);
	connection_specific->ack_sent = 1;
      } else {
	TRACE("Skipping send_ack");
      }

      bip_arecv(channel_specific,
		channel_specific->communicator+MAD_BIP_TRANSFER_TAG,
		buffer->buffer,
		(buffer->length)/sizeof(int),
		&host);

      TRACE("Receiving short buffer. Size = %d. First int = %d\n",
	    buffer->length,
	    ((int*)buffer->buffer)[0]);

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

      give_back_credits (channel_specific, connection->remote_host_id);
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
  p_mad_connection_t connection = lnk->connection;
  p_mad_bip_connection_specific_t connection_specific = connection->specific;
  p_mad_channel_t channel = connection->channel;
  p_mad_bip_channel_specific_t channel_specific = channel->specific;

  LOG_IN();
  /* Code to send one buffer */

  if (connection_specific->begin_send == 1)
    {

      wait_credits(channel_specific, connection->remote_host_id);

#ifdef MARCEL

      send_new(channel_specific, connection->remote_host_id);

#else
      {
	new_message_t m ;

	m.credits = channel_specific->credits_a_rendre[connection->remote_host_id];
	channel_specific->credits_a_rendre[connection->remote_host_id] = 0;

	bip_tsend (
		   connection->remote_host_id, 
		   channel_specific->communicator+MAD_BIP_SERVICE_TAG,
		   (int *) &m,
		   sizeof(new_message_t)/sizeof(int)
		   ) ;
      }
#endif

      connection_specific->begin_send = 0;
      connection_specific->ack_received = 0;
    }


#ifdef MARCEL

  wait_ack(channel_specific, connection->remote_host_id);

#else
  bip_trecv(
	     channel_specific->communicator+MAD_BIP_REPLY_TAG,
             &credits_rendus,
             1
           );
  channel_specific->credits_disponibles [connection->remote_host_id] += credits_rendus ;

#endif

  connection_specific->ack_received = 1;

#ifdef MARCEL
  bip_asend(connection->remote_host_id,
	    channel_specific->communicator+MAD_BIP_TRANSFER_TAG,
	    buffer->buffer,
	    (buffer->length)/sizeof(int));
#else
  bip_tsend (
	     connection->remote_host_id,
	     channel_specific->communicator+MAD_BIP_TRANSFER_TAG,
	     buffer->buffer,
	     (buffer->length)/sizeof(int)
	    ) ;
#endif

  buffer->bytes_read = buffer->length;

  LOG_OUT();
}

static void
mad_bip_receive_long_buffer(p_mad_link_t     lnk,
			    p_mad_buffer_t   buffer)
{
  p_mad_connection_t connection = lnk->connection;
  p_mad_bip_connection_specific_t connection_specific = connection->specific;
  p_mad_channel_t channel = connection->channel;
  p_mad_bip_channel_specific_t channel_specific = channel->specific;
  int  request ;

#ifdef MARCEL
  int status;
#endif

  LOG_IN();
  /* Code to receive one buffer */

  if (connection_specific->begin_receive == 1)
    {
#ifdef MARCEL
      credits_received(channel_specific,
		       connection->remote_host_id,
		       ((new_message_t *)(channel_specific->small_buffer))->credits);
#else
     new_message_t m ;

     memcpy (&m, channel_specific->small_buffer, sizeof (new_message_t)) ;
     channel_specific->credits_disponibles [connection->remote_host_id] += m.credits ;
#endif

     connection_specific->begin_receive = 0 ;
     connection_specific->ack_sent = 0;
    }

#ifdef MARCEL
  request = bip_recv_post(channel_specific->communicator+MAD_BIP_TRANSFER_TAG,
			  buffer->buffer,
			  (buffer->length)/sizeof(int));
#else
  request = bip_tirecv (channel_specific->communicator+MAD_BIP_TRANSFER_TAG,
			buffer->buffer,
			(buffer->length)/sizeof(int));
#endif


#ifdef MARCEL

  send_ack(channel_specific, connection->remote_host_id);

  bip_recv_poll(channel_specific, request, &status);

#else 
  cred = channel_specific->credits_a_rendre [connection->remote_host_id] ;
  channel_specific->credits_a_rendre [connection->remote_host_id] = 0 ;

  bip_tsend (
             connection->remote_host_id,
             channel_specific->communicator+MAD_BIP_REPLY_TAG,
             &cred,
	     1
	     ) ;

  bip_rwait (request) ;
#endif

  connection_specific->ack_sent = 1;
  connection_specific->begin_receive = 0 ;

  buffer->bytes_written = buffer->length;

  LOG_OUT();
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
