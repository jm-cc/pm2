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
Revision 1.1  2000/06/05 16:58:07  rnamyst
Added a beta-beta-beta support for BIP

Revision 1.9  2000/02/08 17:49:44  oaumage
- support de la net toolbox
- mad_tcp.c : deplacement des fonctions statiques de gestion des sockets
              vers la net toolbox

Revision 1.8  2000/01/31 15:54:57  oaumage
- mad_bip.c : terminaison amelioree sous PM2
- mad_tcp.c : debogage de la synchronisation finale

Revision 1.7  2000/01/13 14:46:11  oaumage
- adaptation pour la prise en compte de la toolbox

Revision 1.6  2000/01/10 10:23:16  oaumage
*** empty log message ***

Revision 1.5  2000/01/05 15:52:43  oaumage
- ajout du support PM2 avec polling optimise

Revision 1.4  2000/01/05 09:43:58  oaumage
- initialisation du nouveau champs `group_mode' dans link_init
- mad_sbp.c: suppression des fonctions vides

Revision 1.3  2000/01/04 16:50:48  oaumage
- mad_bip.c: premiere version fonctionnelle du driver
- mad_sbp.c: nouvelle correction de la transmission des noms d'hote a
  l'initialisation
- mad_tcp.c: remplacement des appels `exit' par des macros FAILURES

Revision 1.2  2000/01/04 09:18:49  oaumage
- ajout de la commande de log de CVS
- phase d'initialisation `external-spawn' terminee pour mad_bip.c
- recuperation des noms de machines dans la phase
  d'initialisation `external-spawn' de mad_sbp.c


______________________________________________________________________________
*/

/*
 * Mad_bip.c
 * =========
 */
/* #define DEBUG */
/* #define USE_MARCEL_POLL */

/*
 * !!!!!!!!!!!!!!!!!!!!!!!! Workarounds !!!!!!!!!!!!!!!!!!!!
 * _________________________________________________________
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
#endif /* MAXHOSTNAMELEN */
#define MAD_BIP_SERVICE_TAG      0
#define MAD_BIP_REPLY_TAG        1
#define MAD_BIP_TRANSFER_TAG     2
#define MAD_BIP_FLOW_CONTROL_TAG 3

#define MAD_BIP_FIRST_CHANNEL    0
static  int new_channel = MAD_BIP_FIRST_CHANNEL ;

#define BIP_CHAR                 0
#define BIP_BYTE                 1
#define BIP_INT                  2

#define BIP_ANY_SOURCE          -1

#define BIP_BOTTOM              (void *) -1

#define BIP_MAX_CREDITS             10

#define BIP_SMALL_MESSAGE        64
#define MAX_BIP_REQ              64

typedef struct {
                int host_id ;
                int credits ;
               } new_message_t ;

#ifdef PM2
typedef struct 
{
  marcel_pollinst_t pollinst;
  int        request;
  int        status;
} mad_bip_poll_arg_t, *p_mad_bip_poll_arg_t;
#endif /* PM2 */


typedef struct
{
  TBX_SHARED;
  int nb_adapter;
} mad_bip_driver_specific_t, *p_mad_bip_driver_specific_t;

typedef struct
{
  int *credits_a_rendre ;
  int *credits_disponibles ;
#ifdef PM2
  marcel_sem_t sem_credits ;
  marcel_sem_t sem_ack ;
  int            *ack_recus ;
  int            request_credits ;
  int            credits_rendus ;
#endif
} mad_bip_adapter_specific_t, *p_mad_bip_adapter_specific_t;

typedef struct
{
  int communicator ; /* It's a BIP tag! */
  int small_buffer [BIP_SMALL_MESSAGE] ;
  int message_size ;
} mad_bip_channel_specific_t, *p_mad_bip_channel_specific_t;

typedef struct
{
  int begin_receive ;
  int begin_send ;
} mad_bip_connection_specific_t, *p_mad_bip_connection_specific_t;

typedef struct
{
} mad_bip_link_specific_t, *p_mad_bip_link_specific_t;

#ifdef PM2
static p_mad_bip_driver_specific_t mad_bip_driver_specific;
#endif 

#ifdef PM2


static int bip_asend (int host, int tag, int *message, int size)
{
 int request ;
 int status ;

 lock_task () ;

   request = bip_tisend (host, tag, message, size) ;
   status = bip_stest (request) ;

 unlock_task () ;


 while (status == 0)
   {
    marcel_yield () ;

    lock_task () ;
        status = bip_stest (request) ;    
    unlock_task () ;
   }

 return status ;
}

static int bip_arecv (int tag, int *message, int size, int *host)
{
 int request ;
 int status ;

 lock_task () ;

   request = bip_tirecv (tag, message, size) ;
   status = bip_rtestx (request, host) ;

 unlock_task () ;

 while (status == -1)
   {
    marcel_yield () ;

    lock_task () ;
        status = bip_rtestx (request, host) ;    
    unlock_task () ;

   }
 return status ;
}
#endif

static int wait_credits (p_mad_bip_adapter_specific_t p, int host_id) 
{
 int remote ;
 int cred ;

#ifdef PM2

 marcel_sem_P (&p->sem_credits) ;

 while (p->credits_disponibles [host_id] == 0)
   {
     /* 
    fprintf (stderr, "begin attente de credits du host_id %d\n", host_id) ;
    fflush (stderr) ;
    */

    bip_arecv (MAD_BIP_FLOW_CONTROL_TAG, (int *) &cred, 1, &remote) ;

    /*
    fprintf (stderr, "end attente de credits du host_id %d cred %d remote %d\n", host_id, cred, remote) ;
    fflush (stderr) ;
    */

    p->credits_disponibles [remote] += cred ;
   }
  
  p->credits_disponibles [host_id] -- ;

 marcel_sem_V (&p->sem_credits) ;
 
#else
 while (p->credits_disponibles [host_id] == 0)
   {

    bip_trecvx (MAD_BIP_FLOW_CONTROL_TAG, (int *) &cred, 1, &remote) ;
    p->credits_disponibles [remote] += cred ;
   }
 p->credits_disponibles [host_id] -- ;
#endif

 return 0 ;
}

static int give_back_credits (p_mad_bip_adapter_specific_t p, int host_id)
{

#ifdef PM2   

  marcel_sem_P (&p->sem_credits) ;

    p->credits_a_rendre [host_id] ++ ;

    if ((p->credits_a_rendre [host_id]) == (BIP_MAX_CREDITS-1))
      {
        /* 
        fprintf (stderr, "begin renvoi de credits a host_id %d\n", host_id) ;
        fflush (stderr) ;
        */

	bip_asend (host_id, MAD_BIP_FLOW_CONTROL_TAG, (int *) &(p->credits_a_rendre [host_id]), 1) ;

        /*
        fprintf (stderr, "end renvoi de credits a host_id %d\n", host_id) ;
        fflush (stderr) ;
        */

	p->credits_a_rendre [host_id] = 0 ;
      }

  marcel_sem_V (&p->sem_credits) ;
#else
  p->credits_a_rendre [host_id] ++ ;
  if ((p->credits_a_rendre [host_id]) == (BIP_MAX_CREDITS-1))
    {
     bip_tsend (host_id, MAD_BIP_FLOW_CONTROL_TAG, (int *) &(p->credits_a_rendre [host_id]), 1) ;
     p->credits_a_rendre [host_id] = 0 ;
    }
#endif

 return 0 ;
}



/*
 * exported functions
 * ------------------
 */

/* Registration function */
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
  driver_specific = malloc(sizeof(mad_bip_driver_specific_t));
  CTRL_ALLOC(driver_specific);
  driver->specific = driver_specific;
  TBX_INIT_SHARED(driver_specific);
  driver_specific->nb_adapter = 0;

#ifdef PM2
  mad_bip_driver_specific = driver_specific;
#endif /* PM2 */
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
      adapter->name = malloc(10);
      CTRL_ALLOC(adapter->name);
      sprintf(adapter->name, "BIP%d", driver_specific->nb_adapter);
    }

  driver_specific->nb_adapter++;

  adapter_specific = malloc(sizeof(mad_bip_adapter_specific_t));
  CTRL_ALLOC(adapter_specific);

  adapter->specific = adapter_specific ;

  adapter->parameter = malloc(10);
  CTRL_ALLOC(adapter->parameter);
  sprintf(adapter->parameter, "-"); 
  LOG_OUT();
}

void
mad_bip_channel_init(p_mad_channel_t channel)
{
  p_mad_bip_channel_specific_t channel_specific;

  LOG_IN();

  /* Channel initialization code
     Note: called once for each new channel */
  channel_specific = malloc(sizeof(mad_bip_channel_specific_t));
  CTRL_ALLOC(channel_specific);

  TBX_LOCK_SHARED(mad_bip_driver_specific); 

  channel_specific->communicator = new_channel ;
  new_channel += 4 ;
  channel_specific->message_size = 0 ;

  TBX_UNLOCK_SHARED(mad_bip_driver_specific) ; 
  channel->specific = channel_specific ;
  LOG_OUT() ;
}

void
mad_bip_connection_init(p_mad_connection_t in, p_mad_connection_t out)
{
  p_mad_bip_connection_specific_t connection_specific ;
  
  LOG_IN();

  connection_specific = malloc(sizeof(mad_bip_connection_specific_t));
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

  free(channel_specific);
  channel->specific = NULL;

  LOG_OUT();
}

void
mad_bip_adapter_exit(p_mad_adapter_t adapter)
{
  LOG_IN();
  /* Code to execute to clean up an adapter */
  free(adapter->parameter);
  free(adapter->name);
  free(adapter->specific);
  LOG_OUT();
}

void
mad_bip_driver_exit(p_mad_driver_t driver)
{
  LOG_IN();
  /* Code to execute to clean up a driver */
  /* PM2_LOCK_SHARED(mad_bip_driver_specific); */
  /* MPI_Finalize(); */
  free (driver->specific);
#ifdef PM2
  mad_bip_driver_specific = NULL;
#endif /* PM2 */
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
  p_mad_bip_adapter_specific_t adapter_specific = channel->adapter->specific ;

  p_mad_connection_t    connection       = NULL;
  p_mad_bip_connection_specific_t connection_specific = NULL ;
  int host ;

  LOG_IN();

#ifdef PM2

      channel_specific->message_size = bip_arecv (
		 channel_specific->communicator+MAD_BIP_SERVICE_TAG, 
		 (int *) channel_specific->small_buffer, 
		 BIP_SMALL_MESSAGE,
		 &host
		) ;
      
#else
     channel_specific->message_size = bip_trecvx (
				 channel_specific->communicator+MAD_BIP_SERVICE_TAG, 
				 (int *) channel_specific->small_buffer, 
				 BIP_SMALL_MESSAGE,
				 &host
				 ) ;
#endif
      give_back_credits (adapter_specific, host) ;

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
  int                          i;
  p_mad_bip_adapter_specific_t   adapter_specific = spawn_adapter->specific ;

  LOG_IN();
  /* Code to get configuration information from the external spawn adapter */

  rank = bip_mynode ;
  size = bip_numnodes ;

  adapter_specific->credits_disponibles = (int *) malloc (size * sizeof (int)) ;
  adapter_specific->credits_a_rendre = (int *) malloc (size * sizeof (int)) ;
#ifdef PM2
  adapter_specific->ack_recus = (int *) malloc (size * sizeof (int)) ;
#endif

  for (i=0; i < size; i++)
    {
     adapter_specific->credits_a_rendre [i] = 0 ;
#ifdef PM2
     adapter_specific->ack_recus [i] = 0 ;
#endif
     adapter_specific->credits_disponibles [i] = BIP_MAX_CREDITS ; 
    }

#ifdef PM2
  marcel_sem_init (&adapter_specific->sem_credits, 1) ;
  marcel_sem_init (&adapter_specific->sem_ack, 1) ;
#endif

  configuration->local_host_id = (ntbx_host_id_t)rank;
  configuration->size          = (mad_configuration_size_t)size;
  configuration->host_name     =
    malloc(configuration->size * sizeof(char *));
  CTRL_ALLOC(configuration->host_name);

  for (host_id = 0;
       host_id < configuration->size;
       host_id++)
    {
      configuration->host_name[host_id] = malloc(MAXHOSTNAMELEN + 1);
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
  p_mad_adapter_t          adapter          = channel->adapter ;
  p_mad_bip_adapter_specific_t adapter_specific = adapter->specific ;

  int credits_rendus ;
  int status ;

#ifdef PM2
  int  host ;
  int  request ;
#endif /* PM2 */
  
  LOG_IN();
  /* Code to send one buffer */

  if (connection_specific->begin_send == 1)
    {

      wait_credits (adapter_specific, connection->remote_host_id) ;

#ifdef PM2

      status = bip_asend (
			  connection->remote_host_id,
			  channel_specific->communicator+MAD_BIP_SERVICE_TAG,
			  buffer->buffer,
			  (buffer->length)/sizeof(int)
			 ) ;
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

#ifdef PM2
      /* bip_arecv (
		 channel_specific->communicator+MAD_BIP_REPLY_TAG,
		 &credits_rendus,
		 1,
		 &host
		 ) ;
      */

      /* 
    fprintf (stderr, "BEGIN wait ack from %d\n", connection->remote_host_id) ;
    fflush (stderr) ;
    */

     marcel_sem_P (&adapter_specific->sem_ack) ;

     /* fprintf (stderr, "BEGIN wait ack from %d entree en SC\n", connection->remote_host_id) ;
          fflush (stderr) ;*/

       if (adapter_specific->ack_recus [connection->remote_host_id] == 1)
	  {
	   adapter_specific->ack_recus [connection->remote_host_id] = 0 ;
	  }
	else
	  { int found = 0 ;

            while (found == 0)
	      {
		lock_task () ;

		   request = bip_tirecv (
					 channel_specific->communicator+MAD_BIP_REPLY_TAG,
					 &credits_rendus,
					 1
					 ) ;
		   status = bip_rtestx (request, &host) ; 

		unlock_task () ;

		while (status == -1)
		  {
		    marcel_yield () ;

		    lock_task () ;
		       status = bip_rtestx (request, &host) ;
		    unlock_task () ;
		  }
                

		if (host == connection->remote_host_id)
		  {
		    found = 1 ;
		    adapter_specific->ack_recus [connection->remote_host_id] = 0 ;
		  }
		else
		  {
		    if (adapter_specific->ack_recus [connection->remote_host_id] == 1)
		      {
			found = 1 ;
			adapter_specific->ack_recus [connection->remote_host_id] = 0 ;
		      }
		    else
		      adapter_specific->ack_recus [host] = 1 ;
		  }
	      }
	  }     

      marcel_sem_V (&adapter_specific->sem_ack) ;

      wait_credits (adapter_specific, connection->remote_host_id) ;

      status = bip_asend (
			  connection->remote_host_id,
			  channel_specific->communicator+MAD_BIP_TRANSFER_TAG,
			  buffer->buffer,
			  (buffer->length)/sizeof(int)
			 ) ;
#else
      bip_trecv (
		 channel_specific->communicator+MAD_BIP_REPLY_TAG,
		 &credits_rendus,
		 1
		) ;


      wait_credits (adapter_specific, connection->remote_host_id) ;

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
  p_mad_adapter_t          adapter          = channel->adapter ;
  p_mad_bip_adapter_specific_t adapter_specific = adapter->specific ;

#ifdef PM2
  int  host ;
#endif

  int  status ;
  int  cred 

  LOG_IN();
  /* Code to receive one buffer */

  if (connection_specific->begin_receive == 1)
    {
      memcpy (buffer->buffer, channel_specific->small_buffer, (channel_specific->message_size)*sizeof(int)) ;
      connection_specific->begin_receive = 0 ;
      status = channel_specific->message_size ;
    }
  else
    {
#ifdef PM2

      marcel_sem_P (&adapter_specific->sem_credits) ;

        cred = adapter_specific->credits_a_rendre [connection->remote_host_id] ;
        adapter_specific->credits_a_rendre [connection->remote_host_id] = 0 ;

      marcel_sem_V (&adapter_specific->sem_credits) ;

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

#else

      cred = adapter_specific->credits_a_rendre [connection->remote_host_id] ;
      adapter_specific->credits_a_rendre [connection->remote_host_id] = 0 ;

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

      give_back_credits (adapter_specific, connection->remote_host_id) ;

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
  p_mad_connection_t connection = lnk->connection;
  p_mad_bip_connection_specific_t connection_specific = connection->specific;
  p_mad_channel_t channel = connection->channel;
  p_mad_bip_channel_specific_t channel_specific = channel->specific;
  p_mad_configuration_t configuration = 
    &(channel->adapter->driver->madeleine->configuration);
  p_mad_adapter_t adapter = channel->adapter ;
  p_mad_bip_adapter_specific_t adapter_specific = adapter->specific ;

  int credits_rendus ;
#ifdef PM2
  int  status;
  int  host ;
#endif
  
  LOG_IN();
  /* Code to send one buffer */

  if (connection_specific->begin_send == 1)
    {
      new_message_t m ;

      wait_credits (adapter_specific, connection->remote_host_id) ;

      m.host_id = configuration->local_host_id ;
      m.credits = adapter_specific->credits_a_rendre [connection->remote_host_id] ;
      adapter_specific->credits_a_rendre [connection->remote_host_id] = 0 ; 

#ifdef PM2
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


#ifdef PM2
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

  adapter_specific->credits_disponibles [connection->remote_host_id] += credits_rendus ;
  give_back_credits (adapter_specific, connection->remote_host_id) ;

#ifdef PM2
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
}

static void
mad_bip_receive_long_buffer(p_mad_link_t     lnk,
			    p_mad_buffer_t   buffer)
{
  p_mad_connection_t           connection                             = lnk->connection;
  p_mad_bip_connection_specific_t           connection_specific       = connection->specific;
  p_mad_channel_t              channel          = connection->channel;
  p_mad_bip_channel_specific_t channel_specific = channel->specific;
  p_mad_adapter_t          adapter          = channel->adapter ;
  p_mad_bip_adapter_specific_t adapter_specific = adapter->specific ;

  int  cred  ;
  int  request ;

#ifdef PM2
  int  status;
#endif
  
  LOG_IN();
  /* Code to receive one buffer */

  if (connection_specific->begin_receive == 1)
    {
     new_message_t m ;

     memcpy (&m, channel_specific->small_buffer, sizeof (new_message_t)) ;
     adapter_specific->credits_disponibles [connection->remote_host_id] += m.credits ;

     connection_specific->begin_receive = 0 ;
    }

#ifdef PM2
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

  wait_credits (adapter_specific, connection->remote_host_id) ;

  cred = adapter_specific->credits_a_rendre [connection->remote_host_id] ;
  adapter_specific->credits_a_rendre [connection->remote_host_id] = 0 ;


#ifdef PM2
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
}

static void
mad_bip_send_long_buffer_group(p_mad_link_t           lnk,
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
