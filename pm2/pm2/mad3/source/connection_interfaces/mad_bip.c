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
 * Mad_bip.c
 * =========
 */

#define MARCEL_POLL_WA

/* #define GROUP_SMALL_PACKS */

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
DEBUG_DECLARE(bip)

#undef DEBUG_NAME
#define DEBUG_NAME bip
*/

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
static  int new_channel = MAD_BIP_FIRST_CHANNEL;

#define BIP_MAX_CREDITS          10

#define BIP_SMALL_MESSAGE        BIPSMALLSIZE
#define MAD_BIP_SMALL_SIZE       (BIP_SMALL_MESSAGE*sizeof(int))

#define MAD_NB_INTS(bytes)    ((bytes) % sizeof(int) ? \
			       (bytes)/sizeof(int)+1 : \
                               (bytes)/sizeof(int))

typedef struct 
{
  int credits;
} new_message_t, *p_new_message_t;

typedef struct 
{
  int credits;
} cred_message_t, *p_cred_message_t;

typedef struct 
{
  int credits;
} ack_message_t, *p_ack_message_t;

typedef struct 
{
  TBX_SHARED;
  int nb_adapter;
} mad_bip_driver_specific_t, *p_mad_bip_driver_specific_t;

typedef struct 
{
  int bip_rank;
  int bip_size;
} mad_bip_adapter_specific_t, *p_mad_bip_adapter_specific_t;

typedef struct 
{
  int                communicator;  // It s a BIP tag! 
  int               *small_buffer;  // [BIP_SMALL_MESSAGE]; 
  int                message_size;

  int               *credits_a_rendre;
  int               *credits_disponibles;
  p_tbx_memory_t     bip_buffer_key;
  tbx_bool_t         first_time;

#ifdef MARCEL
  int                request_credits;
  int                credits_rendus;

  marcel_mutex_t     mutex;
  marcel_cond_t     *cond_cred;     // tableau
  marcel_sem_t      *sem_ack;       // tableau
  int                cred_request;
  cred_message_t     cred_msg;
  int                ack_request;
  ack_message_t      ack_msg;

#ifdef USE_MARCEL_POLL
  marcel_pollid_t    ack_pollid, cred_pollid, reg_pollid;
  marcel_pollinst_t *ack_poller;
  boolean           *ack_was_received;
#endif // USE_MARCEL_POLL
#endif // MARCEL
} mad_bip_channel_specific_t, *p_mad_bip_channel_specific_t;

typedef struct 
{
  tbx_bool_t msg;
  tbx_bool_t ack;
} mad_bip_connection_specific_t, *p_mad_bip_connection_specific_t;

typedef struct 
{
  int dummy;
} mad_bip_link_specific_t, *p_mad_bip_link_specific_t;

#ifdef MARCEL
static marcel_mutex_t _bip_mutex;

static __inline__
void
bip_lock(void)
{
  marcel_mutex_lock(&_bip_mutex);
}

static __inline__
unsigned
bip_trylock(void)
{
  return marcel_mutex_trylock(&_bip_mutex);
}

static __inline__
void
bip_unlock(void)
{
  marcel_mutex_unlock(&_bip_mutex);
}

static __inline__
void
credits_received(p_mad_bip_channel_specific_t p,
		 int                          host_id,
		 int                          credits);

#ifdef USE_MARCEL_POLL
typedef struct 
{
  int host;
} ack_poll_arg_t, *p_ack_poll_arg_t;

static
void
ack_poll_wait(p_mad_bip_channel_specific_t p,
	      int                          host)
{
  ack_poll_arg_t arg;

  LOG_IN();
  arg.host = host;
  marcel_poll(p->ack_pollid, &arg);
  LOG_OUT();
}

static
void
ack_group_func(marcel_pollid_t id)
{
  LOG_IN();
  /* Nothing */
  LOG_OUT();
}

static
void *
ack_fastpoll_func(marcel_pollid_t id,
		  any_t           arg,
		  boolean         first_call)
{
  p_ack_poll_arg_t              parg    = NULL;
  unsigned                      host    =    0;
  unsigned                      tmphost =    0;
  int                           status  =    0;
  p_mad_bip_channel_specific_t  p       = NULL;
  void                         *result  = MARCEL_POLL_FAILED;
  marcel_pollinst_t             poller;

  LOG_IN();
  parg = arg;
  host = parg->host;
  p    = marcel_pollid_getspecific(id);

  if (first_call) 
    {
      if (p->ack_was_received[host]) 
	{
	  p->ack_was_received[host] = FALSE;
	  result                    = MARCEL_POLL_OK;
	  goto end;
	}

      p->ack_poller[host] = GET_CURRENT_POLLINST(id);
    }
  else
    { // else if !first_call...
      if (bip_trylock()) 
	{
	  status = bip_rtestx(p->ack_request, &tmphost);

	  if (status != -1) 
	    {
	      credits_received(p, tmphost, p->ack_msg.credits);

	      p->ack_request =
		bip_tirecv(p->communicator + MAD_BIP_REPLY_TAG,
			   (int *)&p->ack_msg,
			   sizeof(ack_message_t)/sizeof(int));
	      bip_unlock();

	      if (tmphost == host)
		{
		  p->ack_poller[host] = NULL;
		  result              = MARCEL_POLL_OK;
		  goto end;
		}
	      else 
		{
		  poller = p->ack_poller[tmphost];

		  if (!poller)
		    {
		      p->ack_was_received[tmphost] = TRUE;
		    }
		  else
		    {
		      p->ack_poller[tmphost] = NULL;
		      result                 = MARCEL_POLL_SUCCESS_FOR(poller);
		      goto end;
		    }
		}
	    }
	  else
	    bip_unlock();
	}
    }

 end:
  LOT_OUT();
  
  return result;
}

static
void *
ack_poll_func(marcel_pollid_t id,
	      unsigned        active,
	      unsigned        sleeping,
	      unsigned        blocked)
{
  unsigned                      host   =    0;
  p_mad_bip_channel_specific_t  p      = NULL;
  int                           status =    0;
  void                         *result = MARCEL_POLL_FAILED;
  marcel_pollinst_t             poller;

  LOG_IN();
  p = marcel_pollid_getspecific(id);

  if (bip_trylock())
    {
      status = bip_rtestx(p->ack_request, &host);

      if (status != -1)
	{
	  credits_received(p, host, p->ack_msg.credits);

	  p->ack_request = bip_tirecv(p->communicator + MAD_BIP_REPLY_TAG,
				      (int *)&p->ack_msg,
				      sizeof(ack_message_t)/sizeof(int));
	  bip_unlock();
	  poller = p->ack_poller[host];

	  if (!poller)
	    {
	      p->ack_was_received[host] = TRUE;
	    }
	  else
	    {
	      p->ack_poller[host] = NULL;
	      result              = MARCEL_POLL_SUCCESS_FOR(poller);
	      goto end;
	    }
	}
      else
	bip_unlock();
    }
  LOG_OUT();
  
 end:
  return result;
}
#endif // end ifdef USE_MARCEL_POLL

static __inline__
void
ack_received(p_mad_bip_channel_specific_t p,
	     int                          host_id,
	     int                          credits)
{
  LOG_IN();
  credits_received(p, host_id, credits);
  marcel_sem_V(&p->sem_ack[host_id]);
  LOG_OUT();
}

#else // else ifdef MARCEL
#define bip_lock()
#define bip_trylock()
#define bip_unlock()
#endif // end else ifdef MARCEL


static
void
bip_sync_send(int  host,
	      int  tag,
	      int *message,
	      int  size)
{
#ifdef MARCEL // ok if BASIC_POLL is defined
  int request = 0;
  int status  = 0;

  LOG_IN();
  TRACE("bip_tisend(host=%d, tag=%d, size=%d)", host, tag, size);

  bip_lock();
  request = bip_tisend(host, tag, message, size);
  status  = bip_stest(request);
  bip_unlock();

  while (!status) 
    {
      marcel_yield();

      TRACE("bip_stest...");
      bip_lock();
      status = bip_stest (request);    
      bip_unlock();
    }

  LOG_OUT();
#else // else ifndef MARCEL
  LOG_IN();
  bip_tsend(host, tag, message, size);
  LOG_OUT();
#endif // end else ifndef MARCEL
}

static
int
bip_recv_post(int  tag,
	      int *message,
	      int  size)
{
  int request = 0;

  LOG_IN();
  bip_lock();
  request = bip_tirecv(tag, message, size);
  bip_unlock();
  LOG_OUT();

  return request;
}

static
int
bip_recv_poll(p_mad_bip_channel_specific_t  p,
	      int                           request,
	      int                          *host)
{
#if defined(MARCEL) && !defined(BASIC_POLL)
  static unsigned which   = 0;
  int             tmphost = 0;
  int             status  = 0;

  LOG_IN();
  switch (which)
    {
    case 0 : 
      { /* SERVICE ou TRANSFERT */
	bip_lock();
	status = bip_rtestx(request, host);
	bip_unlock();
	
	if (status != -1)
	  {
	    LOG_OUT();
	    return status;
	  }
	break;
      }

#ifndef USE_MARCEL_POLL
    case 1 :
      { /* ACKNOWLEDGEMENT */
	bip_lock();
	status = bip_rtestx(p->ack_request, &tmphost);
	bip_unlock();

	if (status != -1)
	  {
	    ack_received(p, tmphost, p->ack_msg.credits);
	  
	    bip_lock();
	    p->ack_request = bip_tirecv(p->communicator + MAD_BIP_REPLY_TAG,
					(int *)&p->ack_msg,
					sizeof(ack_message_t)/sizeof(int));
	    bip_unlock();
	  }

	break;
      }
#else // else ifdef USE_MARCEL_POLL
    case 1 :
      {
	break;
      }
#endif // end else ifdef USE_MARCEL_POLL

    case 2 :
      { /* CREDITS (FLOW_CONTROL) */
	bip_lock();
	status = bip_rtestx(p->cred_request, &tmphost);
	bip_unlock();
	
	if (status != -1)
	  {
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
      FAILURE("invalid message");
  }

  which = (which + 1) % 3;

  LOG_OUT();
  return -1;

#else // MARCEL & !BASIC_POLL
  int status = -1;
  
  LOG_IN();
  bip_lock();
  status = bip_rtestx(request, host);
  bip_unlock();
  LOG_OUT();

  return status;
#endif // else MARCEL & !BASIC_POLL
}

static
int
bip_recv_wait(p_mad_bip_channel_specific_t  p,
	      int                           request,
	      int                          *host)
{
  int status = -1;

  LOG_IN();
#ifdef MARCEL
  for (;;)
    {
      status = bip_recv_poll(p, request, host);
      
      if (status != -1) 
	break;
      
      marcel_yield();
    }
#else // MARCEL
  status =  bip_rwaitx(request, host);
#endif // else MARCEL
  LOG_OUT();

  return status;
}

static __inline__
int
bip_sync_recv (p_mad_bip_channel_specific_t  p,
	       int                           tag,
	       int                          *message,
	       int                           size,
	       int                          *host)
{
  int result = 0;

  LOG_IN();
#ifdef MARCEL
  {
    int request = 0;
    
    request = bip_recv_post(tag, message, size);
    result = bip_recv_wait(p, request, host);
  }
#else // MARCEL
  result = bip_trecvx(tag, message, size, host);
#endif // MARCEL
  LOG_OUT();

  return result;
}

static __inline__
int
bip_probe_recv(p_mad_bip_channel_specific_t  p,
	       int                           tag,
	       int                          *message,
	       int                           size,
	       int                          *host)
{
  static int request = -1;
  int        status  =  0;

  LOG_IN();
  if (request == -1)
    {
      request = bip_recv_post(tag, message, size);
    }
  
  status = bip_recv_poll(p, request, host);

  if (status != -1)
    {
      request = -1; /* pour la prochaine fois */
    }
  LOG_OUT();

  return status;
}

static __inline__
void
send_ack(p_mad_bip_channel_specific_t p,
	 int                          host_id)
{
  ack_message_t msg;

  LOG_IN();
  msg.credits                  = p->credits_a_rendre[host_id];
  p->credits_a_rendre[host_id] = 0;

  bip_sync_send(host_id, p->communicator + MAD_BIP_REPLY_TAG,
		(int*)&msg, sizeof(msg)/sizeof(int));
  LOG_OUT();
}

static __inline__
void
wait_ack(p_mad_bip_channel_specific_t p,
	 int                          host_id)
{
  LOG_IN();
  TRACE("Waiting for ack\n");

#if defined(MARCEL) && !defined(BASIC_POLL)

#ifdef USE_MARCEL_POLL
  ack_poll_wait(p, host_id);
#else // USE_MARCEL_POLL
  marcel_sem_P(&p->sem_ack[host_id]);
#endif // USE_MARCEL_POLL

#else // MARCEL && !BASIC_POLL
  {
    ack_message_t msg;
    int           remote;

    bip_sync_recv(p, p->communicator+MAD_BIP_REPLY_TAG,
		  (int*)&msg, sizeof(msg)/sizeof(int), &remote);

    p->credits_disponibles[host_id] += msg.credits;
  }
#endif // else MARCEL && !BASIC_POLL
  LOG_OUT();
}

static __inline__
void
send_new(p_mad_bip_channel_specific_t p,
	 int                          host_id)
{
  new_message_t msg;

  LOG_IN();
  msg.credits                  = p->credits_a_rendre[host_id];
  p->credits_a_rendre[host_id] = 0;

  bip_sync_send(host_id, p->communicator + MAD_BIP_SERVICE_TAG,
		(int*)&msg, sizeof(msg)/sizeof(int));
  LOG_OUT();
}

static
void
wait_credits(p_mad_bip_channel_specific_t p,
	     int                          host_id)
{
  LOG_IN();
  TRACE("Checking credits\n");

#ifdef MARCEL
  marcel_mutex_lock(&p->mutex);

  while (!p->credits_disponibles[host_id])
    {
      TRACE("Thread %p is waiting for credits from host %d\n",
	    marcel_self(), host_id);
      marcel_cond_wait(&p->cond_cred[host_id], &p->mutex);
    }

  TRACE("Thread %p has %d credits to talk to host %d\n",
	marcel_self(), p->credits_disponibles[host_id], host_id);
  p->credits_disponibles[host_id]--;

  marcel_mutex_unlock(&p->mutex);
#else // MARCEL
  while (!p->credits_disponibles[host_id]) 
    {
      int            remote;
      cred_message_t msg;
      
      bip_sync_recv(p, p->communicator + MAD_BIP_FLOW_CONTROL_TAG, (int *)&msg,
		    sizeof(msg)/sizeof(int), &remote);
      
      p->credits_disponibles [remote] += msg.credits;
    }

  p->credits_disponibles[host_id]--;
#endif // else MARCEL

 LOG_OUT();
}

static __inline__
void
credits_received(p_mad_bip_channel_specific_t p,
		 int                          host_id,
		 int                          credits)
{
  LOG_IN();

#ifdef MARCEL
  if (credits) 
    {
      marcel_mutex_lock(&p->mutex);

      TRACE("Credits received from host %d\n", host_id);
    
      p->credits_disponibles[host_id] += credits;
      marcel_cond_signal(&p->cond_cred[host_id]);
      marcel_mutex_unlock(&p->mutex);
    }
#else // MARCEL
  p->credits_disponibles[host_id] += credits;
#endif // else MARCEL

  LOG_OUT();
}

static
void
give_back_credits (p_mad_bip_channel_specific_t p,
		   int                          host_id)
{
  cred_message_t msg;

  LOG_IN();

#ifdef MARCEL
  marcel_mutex_lock(&p->mutex);

  p->credits_a_rendre[host_id]++;

  if ((p->credits_a_rendre[host_id]) == (BIP_MAX_CREDITS-1))
    {
      msg.credits                  = p->credits_a_rendre[host_id];
      p->credits_a_rendre[host_id] = 0;

      marcel_mutex_unlock(&p->mutex);

      bip_sync_send(host_id, p->communicator + MAD_BIP_FLOW_CONTROL_TAG,
		    (int *)&msg, sizeof(cred_message_t)/sizeof(int));
    }
  else
    {
      marcel_mutex_unlock(&p->mutex);
      TRACE("Give_back_credits : %d credits to give, so forget it...",
	    p->credits_a_rendre[host_id]);
    }
#else // MARCEL
  p->credits_a_rendre [host_id] ++;
  if ((p->credits_a_rendre [host_id]) == (BIP_MAX_CREDITS-1)) 
    {
      msg.credits                   = p->credits_a_rendre [host_id];
      p->credits_a_rendre [host_id] = 0;

      bip_sync_send(host_id, p->communicator + MAD_BIP_FLOW_CONTROL_TAG,
		    (int *)&msg, sizeof(msg)/sizeof(int));
    }
#endif // else MARCEL

  LOG_OUT();
}

void
mad_bip_register(p_mad_driver_t driver)
{
  p_mad_driver_interface_t interface = NULL;

#ifdef DEBUG
  DEBUG_INIT(bip);
#endif // DEBUG

  LOG_IN();
  TRACE("Registering BIP driver");
  interface = driver->interface;
  
  driver->connection_type  = mad_unidirectional_connection;
  driver->buffer_alignment = 32;
  driver->name             = tbx_strdup("bip");
  
  interface->driver_init                = mad_bip_driver_init;
  interface->adapter_init               = mad_bip_adapter_init;
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
#ifdef GROUP_SMALL_PACKS
  interface->get_static_buffer          = mad_bip_get_static_buffer;
  interface->return_static_buffer       = mad_bip_return_static_buffer;
#else // GROUP_SMALL_PACKS
  interface->get_static_buffer          = NULL;
  interface->return_static_buffer       = NULL;
#endif // GROUP_SMALL_PACKS
  interface->new_message                = mad_bip_new_message;
  interface->finalize_message           = NULL;
#ifdef MAD_MESSAGE_POLLING
  interface->poll_message               = mad_bip_poll_message;
#endif /* MAD_MESSAGE_POLLING */
  interface->receive_message            = mad_bip_receive_message;
  interface->message_received           = NULL;
  interface->send_buffer                = mad_bip_send_buffer;
  interface->receive_buffer             = mad_bip_receive_buffer;
  interface->send_buffer_group          = mad_bip_send_buffer_group;
  interface->receive_sub_buffer_group   = mad_bip_receive_sub_buffer_group;
  LOG_OUT();
}

void
mad_bip_driver_init(p_mad_driver_t driver)
{
  p_mad_bip_driver_specific_t driver_specific = NULL;

  LOG_IN();
  /* Driver module initialization code
     Note: called only once, just after
     module registration */
  bip_init ();

  driver_specific  = TBX_CALLOC(1, sizeof(mad_bip_driver_specific_t));
  driver->specific = driver_specific;
  TBX_INIT_SHARED(driver_specific);

#ifdef MARCEL
  marcel_mutex_init(&_bip_mutex, NULL);
#endif
  LOG_OUT();
}

void
mad_bip_adapter_init(p_mad_adapter_t adapter)
{
  p_mad_bip_adapter_specific_t adapter_specific = NULL;
  p_tbx_string_t               parameter_string = NULL;
  
  LOG_IN();
  /* Adapter initialization code 
     Note: called once for each adapter */
  if (strcmp(adapter->dir_adapter->name, "default"))
    FAILURE("unsupported adapter");
  
  adapter_specific = TBX_MALLOC(sizeof(mad_bip_adapter_specific_t));
  adapter->specific = adapter_specific;

  adapter_specific->bip_rank = bip_mynode;
  adapter_specific->bip_size = bip_numnodes;

  parameter_string   =
    tbx_string_init_to_int(adapter_specific->bip_rank);
  adapter->parameter = tbx_string_to_c_string(parameter_string);
  tbx_string_free(parameter_string);
  parameter_string   = NULL;
  LOG_OUT();
}

void
mad_bip_channel_init(p_mad_channel_t channel)
{
  p_mad_bip_channel_specific_t channel_specific = NULL;
  p_mad_bip_adapter_specific_t adapter_specific = NULL;
  p_mad_bip_driver_specific_t  driver_specific  = NULL;
  int                          size             = 0;
  unsigned                     i                = 0;


  LOG_IN();
  /* Channel initialization code
     Note: called once for each new channel */

  adapter_specific = channel->adapter->specific;
  size             = adapter_specific->bip_size;
  driver_specific  = channel->adapter->driver->specific;
  channel_specific = TBX_CALLOC(1, sizeof(mad_bip_channel_specific_t));

  channel_specific->credits_disponibles = TBX_MALLOC (size * sizeof (int));
  channel_specific->credits_a_rendre    = TBX_MALLOC (size * sizeof (int));
  channel_specific->first_time          = 1;

#ifdef MARCEL
  channel_specific->cond_cred = TBX_MALLOC(size*sizeof(marcel_cond_t));
  channel_specific->sem_ack   = TBX_MALLOC(size*sizeof(marcel_sem_t));
  marcel_mutex_init(&channel_specific->mutex, NULL);

#ifdef USE_MARCEL_POLL
  channel_specific->ack_pollid =
    marcel_pollid_create(ack_group_func, ack_poll_func, ack_fastpoll_func,
			 MARCEL_POLL_AT_TIMER_SIG |MARCEL_POLL_AT_YIELD);

  marcel_pollid_setspecific(channel_specific->ack_pollid, channel_specific);

  channel_specific->ack_was_received = TBX_MALLOC(size * sizeof(boolean));
  channel_specific->ack_poller       =
    TBX_MALLOC(size * sizeof(marcel_pollinst_t));
#endif // USE_MARCEL_POLL
#endif // MARCEL

  for (i = 0; i < size; i++)
    {
      channel_specific->credits_a_rendre[i   ] = 0;
      channel_specific->credits_disponibles[i] = BIP_MAX_CREDITS; 

#ifdef MARCEL
      marcel_cond_init(&channel_specific->cond_cred[i], NULL);
      marcel_sem_init(&channel_specific->sem_ack[i], 0);

#ifdef USE_MARCEL_POLL
      channel_specific->ack_was_received[i] = FALSE;
      channel_specific->ack_poller[i]       = NULL;
#endif // USE_MARCEL_POLL
#endif // MARCEL
    }

  tbx_malloc_init(&(channel_specific->bip_buffer_key), MAD_BIP_SMALL_SIZE, 32);


  /*
   * Note: this part of code is not valid with MadIII
   * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   *
   * -> channels may have different size
   */
  TBX_LOCK_SHARED(driver_specific);
  channel_specific->communicator  = new_channel;
  new_channel                    += MAD_BIP_NB_OF_TAGS;
  channel_specific->message_size  = 0;
  TBX_UNLOCK_SHARED(driver_specific);
 
  channel->specific = channel_specific;

#ifdef MARCEL
  bip_lock();
  channel_specific->cred_request =
    bip_tirecv(channel_specific->communicator + MAD_BIP_FLOW_CONTROL_TAG,
	       (int *)&channel_specific->cred_msg,
	       sizeof(cred_message_t)/sizeof(int));

  channel_specific->ack_request =
    bip_tirecv(channel_specific->communicator + MAD_BIP_REPLY_TAG,
	       (int *)&channel_specific->ack_msg,
	       sizeof(ack_message_t)/sizeof(int));
  bip_unlock();
#endif // MARCEL
  LOG_OUT();
}

void
mad_bip_connection_init(p_mad_connection_t in,
			p_mad_connection_t out)
{
  
  LOG_IN();
  if (in)
    {
      p_mad_bip_connection_specific_t in_specific = NULL;

      in_specific = TBX_MALLOC(sizeof(mad_bip_connection_specific_t));

      in_specific->msg = tbx_false;
      in_specific->ack = tbx_false;
      in->specific     = in_specific;

#ifdef GROUP_SMALL_PACKS
      in->nb_link  = 3;
#else // GROUP_SMALL_PACKS
      in->nb_link  = 2;
#endif // GROUP_SMALL_PACKS
    }
  
  if (out)
    {
      p_mad_bip_connection_specific_t out_specific = NULL;

      out_specific = TBX_MALLOC(sizeof(mad_bip_connection_specific_t));

      out_specific->msg = tbx_false;
      out_specific->ack = tbx_false;
      out->specific     = out_specific;

#ifdef GROUP_SMALL_PACKS
      out->nb_link = 3;
#else // GROUP_SMALL_PACKS
      out->nb_link = 2;
#endif // GROUP_SMALL_PACKS
    }
  
  LOG_OUT();
}


void 
mad_bip_link_init(p_mad_link_t lnk)
{

  LOG_IN();
  /* Link initialization code */

#ifdef GROUP_SMALL_PACKS
  if (lnk->id == 2)
    {
      lnk->link_mode   = mad_link_mode_buffer;
      lnk->buffer_mode = mad_buffer_mode_static;
      lnk->group_mode  = mad_group_mode_aggregate;
    }
  else
    {
#endif // GROUP_SMALL_PACKS

      lnk->link_mode   = mad_link_mode_buffer;
      lnk->buffer_mode = mad_buffer_mode_dynamic;
      lnk->group_mode  = mad_group_mode_split;

#ifdef GROUP_SMALL_PACKS
    }
#endif // GROUP_SMALL_PACKS
  LOG_OUT();
}

void
mad_bip_channel_exit(p_mad_channel_t channel)
{  
  p_mad_bip_channel_specific_t channel_specific = NULL;
  
  LOG_IN();
  channel_specific = channel->specific;
  tbx_malloc_clean(channel_specific->bip_buffer_key);

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
  TBX_FREE(adapter->specific);
  LOG_OUT();
}

void
mad_bip_driver_exit(p_mad_driver_t driver)
{
  LOG_IN();
  /* Code to execute to clean up a driver */
  TBX_FREE (driver->specific);
  driver->specific = NULL;
  LOG_OUT();
}


void
mad_bip_new_message(p_mad_connection_t out)
{
  p_mad_bip_connection_specific_t out_specific = NULL;

  LOG_IN();
  out_specific = out->specific;

#ifdef MARCEL
  TRACE("Threads %p begins a new message", marcel_self());
#endif // MARCEL

  out_specific->msg = tbx_true;
  LOG_OUT();
}

#ifdef MAD_MESSAGE_POLLING
p_mad_connection_t 
mad_bip_poll_message(p_mad_channel_t channel)
{
  p_mad_bip_channel_specific_t    channel_specific = NULL;
  p_mad_connection_t              in               = NULL;
  p_mad_bip_connection_specific_t in_specific      = NULL;
  p_tbx_darray_t                  in_darray        = NULL;
  ntbx_process_lrank_t            lrank            =   -1;

  LOG_IN();
  channel_specific = channel->specific;

  if (channel_specific->first_time)
    {
      channel_specific->small_buffer =
	tbx_malloc(channel_specific->bip_buffer_key);
      channel_specific->first_time = 0;
    }

  channel_specific->message_size =
    bip_probe_recv(channel_specific,
		   channel_specific->communicator + MAD_BIP_SERVICE_TAG, 
		   (int *) channel_specific->small_buffer, 
		   BIP_SMALL_MESSAGE, &lrank);

  if (channel_specific->message_size == -1)
    goto end;

  channel_specific->first_time = 1;
  in_darray                    = channel->in_connection_darray;
  in                           = tbx_darray_get(in_darray, lrank);
  in_specific                  = in->specific;      
  in_specific->msg             = tbx_true;

 end:
  LOG_OUT();

  return connection;
}
#endif // MAD_MESSAGE_POLLING

p_mad_connection_t
mad_bip_receive_message(p_mad_channel_t channel)
{
  p_mad_bip_channel_specific_t    channel_specific = NULL;
  p_mad_connection_t              in               = NULL;
  p_mad_bip_connection_specific_t in_specific      = NULL;
  p_tbx_darray_t                  in_darray        = NULL;
  ntbx_process_lrank_t            lrank            =   -1;

  LOG_IN();
  channel_specific               = channel->specific;
  channel_specific->small_buffer =
    tbx_malloc(channel_specific->bip_buffer_key);

  channel_specific->message_size =
    bip_sync_recv(channel_specific,
		  channel_specific->communicator+MAD_BIP_SERVICE_TAG, 
		  (int *) channel_specific->small_buffer, 
		  BIP_SMALL_MESSAGE,
		  &lrank);

  in_darray        = channel->in_connection_darray;
  in               = tbx_darray_get(in_darray, lrank);
  in_specific      = in->specific;      
  in_specific->msg = tbx_true;
  LOG_OUT();

  return in;
}

p_mad_link_t
mad_bip_choice(p_mad_connection_t connection,
	       size_t             size         TBX_UNUSED,
	       mad_send_mode_t    send_mode    TBX_UNUSED,
	       mad_receive_mode_t receive_mode TBX_UNUSED)
{
  p_mad_link_t lnk = NULL;

  LOG_IN();
#ifdef GROUP_SMALL_PACKS
  if(size <= MAD_BIP_SMALL_SIZE/2) /* Un peu arbitraire... */
    {
      lnk = connection->link_array[2];
    }
  else
#endif // GROUP_SMALL_PACKS
  if (size <= MAD_BIP_SMALL_SIZE)
    {
      lnk = connection->link_array[0];
    }
  else
    {
      lnk = connection->link_array[1];
    }
  LOG_OUT();

  return lnk;
}


static
void
mad_bip_send_short_buffer(p_mad_link_t   lnk,
			  p_mad_buffer_t buffer)
{
  p_mad_connection_t              out              = NULL;
  p_mad_bip_connection_specific_t out_specific     = NULL;
  p_mad_channel_t                 channel          = NULL;
  p_mad_bip_channel_specific_t    channel_specific = NULL;

  LOG_IN();
#ifdef MARCEL
  TRACE("Thread %p sends a buffer", marcel_self());
#endif

  out              = lnk->connection;
  out_specific     = out->specific;
  channel          = out->channel;
  channel_specific = channel->specific;

  if (out_specific->msg)
    {
      TRACE("Sending short buffer. Size = %d. First int = %d\n",
	    (buffer->bytes_written), ((int*)buffer->buffer)[0]);

      bip_sync_send (out->remote_rank,
		     channel_specific->communicator + MAD_BIP_SERVICE_TAG,
		     buffer->buffer, MAD_NB_INTS(buffer->bytes_written));

      out_specific->msg = tbx_false;
      out_specific->ack = tbx_false;
    }
  else
    {
      /* On n'envoie un ACK *que* pour le second paquet... */
      if (!out_specific->ack)
	{
	  wait_ack(channel_specific, out->remote_rank);
	  out_specific->ack = tbx_true;
	}
      else
	{
	  TRACE("Skipping wait_ack");
	}

      wait_credits(channel_specific, out->remote_rank);

      bip_sync_send(out->remote_rank,
		    channel_specific->communicator + MAD_BIP_TRANSFER_TAG,
		    buffer->buffer, MAD_NB_INTS(buffer->bytes_written));

      TRACE("Sending short buffer. Size = %d. First int = %d\n",
	    (buffer->bytes_written), ((int*)buffer->buffer)[0]);
    }

  buffer->bytes_read = buffer->bytes_written;
  LOG_OUT();
}

static
void
mad_bip_receive_short_buffer(p_mad_link_t   lnk,
			     p_mad_buffer_t buffer)
{
  p_mad_connection_t              in               = NULL;
  p_mad_bip_connection_specific_t in_specific      = NULL;
  p_mad_channel_t                 channel          = NULL;
  p_mad_bip_channel_specific_t    channel_specific = NULL;
  ntbx_process_lrank_t            lrank            =   -1;
  int                             status           =   -1;

  LOG_IN();
  in               = lnk->connection;
  in_specific      = in->specific;
  channel          = in->channel;
  channel_specific = channel->specific;

  if (in_specific->msg)
    {
      /* Normalement, on n'arrive JAMAIS ici si buffer = statique ! */
      memcpy (buffer->buffer,
	      channel_specific->small_buffer,
	      channel_specific->message_size * sizeof(int));

      status = channel_specific->message_size;

      TRACE("Receiving short buffer. Size = %d. First int = %d\n",
	    channel_specific->message_size * sizeof(int),
	    ((int*)channel_specific->small_buffer)[0]);

      tbx_free(channel_specific->bip_buffer_key,
	       channel_specific->small_buffer);

      in_specific->msg = tbx_false;
      in_specific->ack = tbx_false;
    }
  else
    {
      /* On n'envoie un ACK *que* pour le second paquet... */
      if (!in_specific->ack)
	{
	  send_ack(channel_specific, in->remote_rank);
	  in_specific->ack = tbx_true;
	}
      else
	{
	  TRACE("Skipping send_ack");
	}

      status =
	bip_sync_recv(channel_specific,
		      channel_specific->communicator + MAD_BIP_TRANSFER_TAG,
		      buffer->buffer,
		      MAD_NB_INTS(buffer->length),
		      &lrank);

      TRACE("Receiving short buffer. Size = %d. First int = %d\n",
	    status * sizeof(int), ((int*)buffer->buffer)[0]);

      give_back_credits (channel_specific, in->remote_rank);
    }

  buffer->bytes_written = status * sizeof(int);
  LOG_OUT();
}

static
void
mad_bip_send_long_buffer(p_mad_link_t   lnk,
			 p_mad_buffer_t buffer)
{
  p_mad_connection_t              out              = NULL;
  p_mad_bip_connection_specific_t out_specific     = NULL;
  p_mad_channel_t                 channel          = NULL;
  p_mad_bip_channel_specific_t    channel_specific = NULL;

  LOG_IN();
  /* Code to send one buffer */
  out              = lnk->connection;
  out_specific     = out->specific;
  channel          = out->channel;
  channel_specific = channel->specific;

  if (out_specific->msg)
    {
      send_new(channel_specific, out->remote_rank);

      out_specific->msg = tbx_false;
      out_specific->ack = tbx_false;
    }

  while (mad_more_data(buffer))
    {
      size_t length = 0;

      length = min(524288, (buffer->bytes_written - buffer->bytes_read));

      wait_ack(channel_specific, out->remote_rank);
      out_specific->ack = tbx_true;

      bip_sync_send(out->remote_rank,
		    channel_specific->communicator + MAD_BIP_TRANSFER_TAG,
		    buffer->buffer + buffer->bytes_read,
		    MAD_NB_INTS(length));

      buffer->bytes_read += length;
    }  
  LOG_OUT();
}

static
void
mad_bip_receive_long_buffer(p_mad_link_t   lnk,
			    p_mad_buffer_t buffer)
{
  p_mad_connection_t              in               = NULL;
  p_mad_bip_connection_specific_t in_specific      = NULL;
  p_mad_channel_t                 channel          = NULL;
  p_mad_bip_channel_specific_t    channel_specific = NULL;
  ntbx_process_lrank_t            lrank            =   -1;
  int                             request          =   -1;

  LOG_IN();
  /* Code to receive one buffer */
  in               = lnk->connection;
  in_specific      = in->specific;
  channel          = in->channel;
  channel_specific = channel->specific;
 
  if (in_specific->msg)
    {
      credits_received(channel_specific, in->remote_rank,
		       ((new_message_t *)(channel_specific->small_buffer))->
		       credits);

      tbx_free(channel_specific->bip_buffer_key,
	       channel_specific->small_buffer);

     in_specific->msg = tbx_false;
     in_specific->ack = tbx_false;
    }

  while (!mad_buffer_full(buffer))
    {
      size_t length = 0;
 
      length = min(524288, (buffer->length - buffer->bytes_written));

      request =
	bip_recv_post(channel_specific->communicator + MAD_BIP_TRANSFER_TAG,
		      buffer->buffer, MAD_NB_INTS(length));
      
      buffer->bytes_written += length;

      send_ack(channel_specific, in->remote_rank);
      bip_recv_wait(channel_specific, request, &lrank);
    }

  if (request != -1)
    {
      in_specific->ack = tbx_true;
      in_specific->msg = tbx_false;
    }
  LOG_OUT();
}

#ifdef GROUP_SMALL_PACKS
/* Renvoie un buffer tout neuf qui sera rempli par l'application puis
   émis... */
p_mad_buffer_t
mad_bip_get_static_buffer(p_mad_link_t lnk)
{
  p_mad_bip_channel_specific_t channel_specific = NULL;
  p_mad_buffer_t               buffer           = NULL;
  
  LOG_IN();
  channel_specific      = lnk->connection->channel->specific;
  buffer                = mad_alloc_buffer_struct();
  buffer->buffer        = tbx_malloc(channel_specific->bip_buffer_key);
  buffer->length        = MAD_BIP_SMALL_SIZE;
  buffer->bytes_written = 0;
  buffer->bytes_read    = 0;
  buffer->type          = mad_static_buffer;
  LOG_OUT();

  return buffer;
}

/* Appelé par l'application lorsque les données ont bien été lues... */
void
mad_bip_return_static_buffer(p_mad_link_t   lnk,
			     p_mad_buffer_t buffer)
{
  p_mad_bip_channel_specific_t channel_specific = NULL;
  
  LOG_IN();
  channel_specific = lnk->connection->channel->specific;
  tbx_free(channel_specific->bip_buffer_key, buffer->buffer);
  buffer->buffer   = NULL;
  LOG_OUT();
}
#endif // GROUP_SMALL_PACKS

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
#ifdef GROUP_SMALL_PACKS
  else if (lnk->id == 2)
    {
      p_mad_bip_channel_specific_t channel_specific = NULL;

      channel_specific = lnk->connection->channel->specific;
      mad_bip_send_short_buffer(lnk, buffer);
      tbx_free(channel_specific->bip_buffer_key, buffer->buffer);
      buffer->buffer = NULL;
    }
#endif // GROUP_SMALL_PACKS
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
#ifdef GROUP_SMALL_PACKS
  else if (lnk->id == 2)
    {
      p_mad_bip_connection_specific_t connection_specific = NULL;
      p_mad_bip_channel_specific_t    channel_specific    = NULL;
      p_mad_buffer_t                  buf                 = NULL;

      connection_specific = lnk->connection->specific;
      channel_specific    = lnk->connection->channel->specific;

      buf             = mad_alloc_buffer_struct();
      buf->length     = MAD_BIP_SMALL_SIZE;
      buf->bytes_read = 0;
      buf->type       = mad_static_buffer;

      *buffer = buf;
  
      if (connection_specific->msg)
	{
	  buf->buffer        = channel_specific->small_buffer;
	  buf->bytes_written = channel_specific->message_size*sizeof(int);

	  connection_specific->msg = tbx_false;      
	  connection_specific->ack = tbx_false;
	}
      else
	{
	  buf->buffer = tbx_malloc(channel_specific->bip_buffer_key);
	  mad_bip_receive_short_buffer(lnk, *buffer);
	}
    }
#endif // GROUP_SMALL_PACKS
  else
    FAILURE("Invalid link ID");

  LOG_OUT();  
}

void
mad_bip_send_buffer_group(p_mad_link_t         lnk,
			  p_mad_buffer_group_t buffer_group)
{
  LOG_IN();
  if (!tbx_empty_list(&(buffer_group->buffer_list)))
    {
      tbx_list_reference_t ref;

      tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
      do
	{
	  mad_bip_send_buffer(lnk, tbx_get_list_reference_object(&ref));
	}
      while(tbx_forward_list_reference(&ref));
    }
  LOG_OUT();
}

void
mad_bip_receive_sub_buffer_group(p_mad_link_t           lnk,
				 tbx_bool_t             first_sub_group
				 TBX_UNUSED,
				 p_mad_buffer_group_t   buffer_group)
{
  LOG_IN();
  if (!tbx_empty_list(&(buffer_group->buffer_list)))
    {
      tbx_list_reference_t ref;

      tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
      do
	{
	  mad_bip_receive_buffer(lnk, tbx_get_list_reference_object(&ref));
	}
      while(tbx_forward_list_reference(&ref));
    }
  LOG_OUT();
}
