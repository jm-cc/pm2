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

#ifndef MARCEL
#error This driver needs the MARCEL module.
#endif

#ifndef USE_MARCEL_POLL
#error This driver MUST be compiled using the USE_MARCEL_POLL flag.
#endif

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

enum {
  MAD_BIP_TRANSFER_TAG,
  _MAD_BIP_NB_OF_TAGS
};

#define MAD_BIP_SERVICE_TAG      0
#define MAD_BIP_FIRST_CHANNEL    1

// Credits for regular (transfer) small messages
#define BIP_MAX_CREDITS          10

// Credits for service (NEW) messages
#define BIP_MAX_NEW_CREDITS      2

// Max number of integers within a small message
#define BIP_SMALL_MESSAGE        BIPSMALLSIZE

// Max number of bytes within a small message
#define MAD_BIP_SMALL_SIZE       (BIP_SMALL_MESSAGE*sizeof(int))
#define MAD_BIP_SMALL_PDU        (MAD_BIP_SMALL_SIZE - sizeof(new_message_t))

// Max number of bytes that may be sent using a single bip_send transaction
#define MAD_BIP_MAX_SIZE         (512 * 1024)

#define MAD_BIP_SIZE_ALIGNMENT_MASK  (0x0003)


#define MAD_NB_INTS(bytes)    ((bytes) % sizeof(int) ? \
			       (bytes)/sizeof(int)+1 : \
                               (bytes)/sizeof(int))

/******************************************************************/

// 'the_channels' is a dynamic array allowing the channel-specific 
// structures to be accessed using the channel number as an index.
static p_tbx_darray_t the_channels;

static unsigned new_channel = MAD_BIP_FIRST_CHANNEL;
static unsigned next_channel_num = 0;

// header inserted in the first chunk of a message
typedef enum { NEW_TYPE, ACK_TYPE, CRED_TYPE, NEW_CRED_TYPE } msg_type_t;

#define MSG_TYPE_BITS                      2
#define MSG_HEADER_BUILD(type, channel)    ((unsigned)(type) | \
                                            ((unsigned)(channel) << MSG_TYPE_BITS))
#define MSG_HEADER_TYPE(header)            ((unsigned)(header) & \
                                            ((1 << MSG_TYPE_BITS) - 1))
#define MSG_HEADER_CHANNEL(header)         ((unsigned)(header) >> MSG_TYPE_BITS)

#define MSG_CREDITS_BITS                   16
#define MSG_CREDITS_GET_SHARED(credits)    ((unsigned)(credits) >> MSG_CREDITS_BITS)
#define MSG_CREDITS_GET_PRIVATE(credits)   ((unsigned)(credits) & \
					    ((1 << MSG_CREDITS_BITS) - 1))
#define MSG_CREDITS_BUILD(shared, private) ((unsigned)(private) | \
					    ((unsigned)(shared) << MSG_CREDITS_BITS))

typedef struct
{
  unsigned header;
  int credits;
} new_message_t, *p_new_message_t;

// Polling request type for "header" messages (new_message)
static marcel_pollid_t     service_recv_pollid;
// Global buffer used to receive next incoming "header" message
static p_tbx_memory_t      bip_buffer_key;
static int                *service_buffer;
// BIP request for service messages
static int                 service_request = -1;
// Credits for service messages
static int                *credits_to_give_back;
static int                *credits_owned;
static marcel_pollinst_t  *cred_poller;

typedef struct
{
  msg_type_t type;
  int credits;
} ctrl_message_t, *p_ctrl_message_t;

typedef struct s_mad_bip_driver_specific
{
  TBX_SHARED;
  int nb_adapter;
} mad_bip_driver_specific_t, *p_mad_bip_driver_specific_t;

typedef struct s_mad_bip_adapter_specific
{
  int bip_rank;
  int bip_size;
} mad_bip_adapter_specific_t, *p_mad_bip_adapter_specific_t;

typedef struct s_mad_bip_channel_specific
{
  unsigned           id;
  int                base_tag;  // It s a BIP tag! 
  int               *small_buffer;
  int               *send_buffer;
  int                message_size;
  int                message_origin;
  marcel_pollinst_t  recv_poller;
  tbx_bool_t         msg_was_received;


  int               *credits_a_rendre;
  int               *credits_disponibles;
  tbx_bool_t        *ack_was_received;
  
  tbx_bool_t         first_time;

  marcel_mutex_t    *send_mutex;

  marcel_pollinst_t *ack_poller, *cred_poller;
  marcel_pollid_t    send_pollid;
} mad_bip_channel_specific_t, *p_mad_bip_channel_specific_t;

typedef struct s_mad_bip_connection_specific
{
  tbx_bool_t msg;
  tbx_bool_t ack;
} mad_bip_connection_specific_t, *p_mad_bip_connection_specific_t;

typedef struct s_mad_bip_link_specific
{
  int dummy;
} mad_bip_link_specific_t, *p_mad_bip_link_specific_t;


// =============================================================
// Global mutexes
static marcel_mutex_t _bip_mutex, _cred_mutex, _new_msg_mutex, _send_mutex;

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

// =============================================================
// Receive polling functions
typedef struct 
{
  msg_type_t msg_type;
  p_mad_bip_channel_specific_t channel_specific;
  int host;
} recv_poll_arg_t, *p_recv_poll_arg_t;

static void recv_msg_group_func(marcel_pollid_t id)
{}

static void *recv_msg_fastpoll_func(marcel_pollid_t id,
				    any_t           arg,
				    tbx_bool_t      first_call)
{
  int                           status  = 0;
  unsigned                      rhost   = 0;
  void                         *result  = MARCEL_POLL_FAILED;
  p_recv_poll_arg_t             parg    = arg;
  p_mad_bip_channel_specific_t  p       = parg->channel_specific;
  msg_type_t                    expected_type = parg->msg_type;

  LOG_IN();

  // Let's first have a look on what happened in the past
  switch(expected_type) {
  case NEW_TYPE:
    {
      if(p->msg_was_received) {
	p->msg_was_received = tbx_false;
	result = MARCEL_POLL_OK;
	goto end;
      }
      if(first_call)
	p->recv_poller = GET_CURRENT_POLLINST(id);
      break;
    }
  case NEW_CRED_TYPE:
    {
      if(credits_owned[parg->host]) {
	credits_owned[parg->host]--;
	result = MARCEL_POLL_OK;
	goto end;
      }
      if(first_call)
	cred_poller[parg->host] = GET_CURRENT_POLLINST(id);
      break;
    }
  case CRED_TYPE:
    {
      if(p->credits_disponibles[parg->host]) {
	p->credits_disponibles[parg->host]--;
	result = MARCEL_POLL_OK;
	goto end;
      }
      if(first_call)
	p->cred_poller[parg->host] = GET_CURRENT_POLLINST(id);
    }
  case ACK_TYPE:
    {
      if(p->ack_was_received[parg->host]) {
	p->ack_was_received[parg->host] = tbx_false;
	result = MARCEL_POLL_OK;
	goto end;
      }
      if(first_call)
	p->ack_poller[parg->host] = GET_CURRENT_POLLINST(id);
    }
  }

  // Let's perform a BIP check...
  if (bip_trylock()) {
    status = bip_rtestx(service_request, &rhost);

    if (status != -1) {
      new_message_t *msg_header = (new_message_t *)service_buffer;
      unsigned chan = MSG_HEADER_CHANNEL(msg_header->header);
      p_mad_bip_channel_specific_t chan_spec = tbx_darray_get(the_channels, chan);
      unsigned shared_credits, private_credits;

      shared_credits = MSG_CREDITS_GET_SHARED(msg_header->credits);
      private_credits = MSG_CREDITS_GET_PRIVATE(msg_header->credits);

      if(shared_credits)
	credits_owned[rhost] += shared_credits;

      if(private_credits)
	chan_spec->credits_disponibles[rhost] += private_credits;

      switch(MSG_HEADER_TYPE(msg_header->header)) {
      case NEW_TYPE:
	{
	  unsigned *tempo;

	  chan_spec->message_size = status;
	  chan_spec->message_origin = rhost;

	  // Exchange of small_buffer and service_buffer
	  tempo = chan_spec->small_buffer;
	  chan_spec->small_buffer = service_buffer;
	  service_buffer = tempo;

	  // Prepare next receive transaction
	  service_request = bip_tirecv(MAD_BIP_SERVICE_TAG,
				       (int *)service_buffer, BIP_SMALL_MESSAGE);

	  // Let's see if we can satisfy the calling thread
	  if(expected_type == NEW_TYPE && chan_spec == p) {
	    chan_spec->recv_poller = NULL;
	    result = MARCEL_POLL_OK;
	  } else {
	    chan_spec->msg_was_received = tbx_true;

	    if(expected_type == NEW_CRED_TYPE && credits_owned[parg->host]) {
	      credits_owned[parg->host]--;
	      cred_poller[parg->host] = NULL;
	      result = MARCEL_POLL_OK;
	    } else if(expected_type == CRED_TYPE && p->credits_disponibles[parg->host]) {
	      p->credits_disponibles[parg->host]--;
	      p->cred_poller[parg->host] = NULL;
	      result = MARCEL_POLL_OK;
	    }
	  }

	  break;
	}
      case CRED_TYPE:
      case NEW_CRED_TYPE: 
	{
	  // Prepare next receive transaction
	  service_request = bip_tirecv(MAD_BIP_SERVICE_TAG,
				       (int *)service_buffer, BIP_SMALL_MESSAGE);

	  // Let's see if we can satisfy the calling thread
	  if(expected_type == NEW_CRED_TYPE && credits_owned[parg->host]) {
	    credits_owned[parg->host]--;
	    cred_poller[parg->host] = NULL;
	    result = MARCEL_POLL_OK;
	  } else if(expected_type == CRED_TYPE && p->credits_disponibles[parg->host]) {
	    p->credits_disponibles[parg->host]--;
	    p->cred_poller[parg->host] = NULL;
	    result = MARCEL_POLL_OK;
	  }

	  break;
	}
      case ACK_TYPE:
	{
	  // Prepare next receive transaction
	  service_request = bip_tirecv(MAD_BIP_SERVICE_TAG,
				       (int *)service_buffer, BIP_SMALL_MESSAGE);

	  // Let's see if we can satisfy the calling thread
	  if(expected_type == ACK_TYPE && chan_spec == p && parg->host == rhost) {
	    chan_spec->ack_poller[parg->host] = NULL;
	    result = MARCEL_POLL_OK;
	  } else {
	    chan_spec->ack_was_received[rhost] = tbx_true;

	    if(expected_type == NEW_CRED_TYPE && credits_owned[parg->host]) {
	      credits_owned[parg->host]--;
	      cred_poller[parg->host] = NULL;
	      result = MARCEL_POLL_OK;
	    } else if(expected_type == CRED_TYPE && p->credits_disponibles[parg->host]) {
	      p->credits_disponibles[parg->host]--;
	      p->cred_poller[parg->host] = NULL;
	      result = MARCEL_POLL_OK;
	    }
	  }

	  break;
	}
      default:
	FAILURE("mad_bip ERROR: message with unknown tag received.");
      } // switch

    } // if(status != -1)

    bip_unlock();

  } // bip_trylock

 end:
  LOG_OUT();

  return result;
}

static void *recv_msg_poll_func(marcel_pollid_t id,
				unsigned        active,
				unsigned        sleeping,
				unsigned        blocked)
{
  int                           status  = 0;
  unsigned                      rhost   = 0;
  void                         *result  = MARCEL_POLL_FAILED;
  p_recv_poll_arg_t             parg    = NULL;
  p_mad_bip_channel_specific_t  p;
  msg_type_t                    expected_type;

  LOG_IN();

  FOREACH_POLL(id) { GET_ARG(id, parg);

    p = parg->channel_specific;
    expected_type = parg->msg_type;

    // Let's first have a look on what happened in the past
    switch(expected_type) {
    case NEW_TYPE:
      {
	if(p->msg_was_received) {
	  p->msg_was_received = tbx_false;
	  p->recv_poller = NULL;
	  result = MARCEL_POLL_SUCCESS(id);
	  goto end;
	}
	break;
      }
    case NEW_CRED_TYPE:
      {
	if(credits_owned[parg->host]) {
	  credits_owned[parg->host]--;
	  cred_poller[parg->host] = NULL;
	  result = MARCEL_POLL_SUCCESS(id);
	  goto end;
	}
	break;
      }
    case CRED_TYPE:
      {
	if(p->credits_disponibles[parg->host]) {
	  p->credits_disponibles[parg->host]--;
	  p->cred_poller[parg->host] = NULL;
	  result = MARCEL_POLL_SUCCESS(id);
	  goto end;
	}
      }
    case ACK_TYPE:
      if(p->ack_was_received[parg->host]) {
	p->ack_was_received[parg->host] = tbx_false;
	p->ack_poller[parg->host] = NULL;
	result = MARCEL_POLL_SUCCESS(id);
	goto end;
      }
    }
  } // FOREACH


  // Let's perform a BIP check...
  if (bip_trylock()) {
    status = bip_rtestx(service_request, &rhost);

    if (status != -1) {
      new_message_t *msg_header = (new_message_t *)service_buffer;
      unsigned chan = MSG_HEADER_CHANNEL(msg_header->header);
      p_mad_bip_channel_specific_t chan_spec = tbx_darray_get(the_channels, chan);
      unsigned shared_credits, private_credits;

      shared_credits = MSG_CREDITS_GET_SHARED(msg_header->credits);
      private_credits = MSG_CREDITS_GET_PRIVATE(msg_header->credits);

      if(shared_credits)
	credits_owned[rhost] += shared_credits;

      if(private_credits)
	chan_spec->credits_disponibles[rhost] += private_credits;

      switch(MSG_HEADER_TYPE(msg_header->header)) {
      case NEW_TYPE:
	{
	  unsigned *tempo;

	  chan_spec->message_size = status;
	  chan_spec->message_origin = rhost;

	  // Exchange of small_buffer and service_buffer
	  tempo = chan_spec->small_buffer;
	  chan_spec->small_buffer = service_buffer;
	  service_buffer = tempo;

	  // Prepare next receive transaction
	  service_request = bip_tirecv(MAD_BIP_SERVICE_TAG,
				       (int *)service_buffer, BIP_SMALL_MESSAGE);

	  // Let's see if we can satisfy some thread
	  if(chan_spec->recv_poller) {
	    result = MARCEL_POLL_SUCCESS_FOR(chan_spec->recv_poller);
	    chan_spec->recv_poller = NULL;
	  } else {
	    chan_spec->msg_was_received = tbx_true;

	    if(shared_credits && cred_poller[rhost]) {
	      credits_owned[rhost]--;
	      result = MARCEL_POLL_SUCCESS_FOR(cred_poller[rhost]);
	      cred_poller[rhost] = NULL;
	    } else if(private_credits && chan_spec->cred_poller[rhost]) {
	      chan_spec->credits_disponibles[rhost]--;
	      result = MARCEL_POLL_SUCCESS_FOR(chan_spec->cred_poller[rhost]);
	      chan_spec->cred_poller[rhost] = NULL;
	    }
	  }

	  break;
	}
      case CRED_TYPE:
      case NEW_CRED_TYPE:
	{
	  // Prepare next receive transaction
	  service_request = bip_tirecv(MAD_BIP_SERVICE_TAG,
				       (int *)service_buffer, BIP_SMALL_MESSAGE);

	  // Let's see if we can satisfy some thread
	  if(shared_credits && cred_poller[rhost]) {
	    credits_owned[rhost]--;
	    result = MARCEL_POLL_SUCCESS_FOR(cred_poller[rhost]);
	    cred_poller[rhost] = NULL;
	  } else if(private_credits && chan_spec->cred_poller[rhost]) {
	    chan_spec->credits_disponibles[rhost]--;
	    result = MARCEL_POLL_SUCCESS_FOR(chan_spec->cred_poller[rhost]);
	    chan_spec->cred_poller[rhost] = NULL;
	  }

	  break;
	}
      case ACK_TYPE:
	{
	  // Prepare next receive transaction
	  service_request = bip_tirecv(MAD_BIP_SERVICE_TAG,
				       (int *)service_buffer, BIP_SMALL_MESSAGE);

	  // Let's see if we can satisfy some thread
	  if(chan_spec->ack_poller[rhost]) {
	    result = MARCEL_POLL_SUCCESS_FOR(chan_spec->ack_poller[rhost]);
	    chan_spec->ack_poller[rhost] = NULL;
	  } else {
	    chan_spec->ack_was_received[rhost] = tbx_true;

	    if(shared_credits && cred_poller[rhost]) {
	      credits_owned[rhost]--;
	      result = MARCEL_POLL_SUCCESS_FOR(cred_poller[rhost]);
	      cred_poller[rhost] = NULL;
	    } else if(private_credits && chan_spec->cred_poller[rhost]) {
	      chan_spec->credits_disponibles[rhost]--;
	      result = MARCEL_POLL_SUCCESS_FOR(chan_spec->cred_poller[rhost]);
	      chan_spec->cred_poller[rhost] = NULL;
	    }
	  }

	  break;
	}
      default:
	FAILURE("mad_bip ERROR: message with unknown tag received.");
      } // switch

    } // if(status != -1)

    bip_unlock();

  } // bip_trylock

end:
  LOG_OUT();

  return result;
}

static void recv_poll(p_mad_bip_channel_specific_t p)
{
  recv_poll_arg_t arg;

  LOG_IN();

  arg.msg_type = NEW_TYPE;
  arg.host = -1;
  arg.channel_specific = p;
  marcel_poll(service_recv_pollid, &arg);

  LOG_OUT();
}

static void shared_credits_poll(int host)
{
  recv_poll_arg_t arg;

  LOG_IN();

  arg.msg_type = NEW_CRED_TYPE;
  arg.host = host;
  arg.channel_specific = NULL;
  marcel_poll(service_recv_pollid, &arg);

  LOG_OUT();
}

static void private_credits_poll(p_mad_bip_channel_specific_t p, int host)
{
  recv_poll_arg_t arg;

  LOG_IN();

  arg.msg_type = CRED_TYPE;
  arg.channel_specific = p;
  arg.host = host;
  marcel_poll(service_recv_pollid, &arg);

  LOG_OUT();
}

static void ack_poll(p_mad_bip_channel_specific_t p, int host)
{
  recv_poll_arg_t arg;

  LOG_IN();

  arg.msg_type = ACK_TYPE;
  arg.channel_specific = p;
  arg.host = host;
  marcel_poll(service_recv_pollid, &arg);

  LOG_OUT();
}

// =============================================================

typedef struct {
  int request;
} send_poll_arg_t, *p_send_poll_arg_t;

static void send_poll_wait(p_mad_bip_channel_specific_t p,
			   int  host,
			   int  tag,
			   int *message,
			   int  size)
{
  send_poll_arg_t arg;

  LOG_IN();

  bip_lock();
  arg.request = bip_tisend(host, tag, message, size);
  bip_unlock();

  marcel_poll(p->send_pollid, &arg);

  LOG_OUT();
}

static void send_group_func(marcel_pollid_t id)
{}

static void *send_fastpoll_func(marcel_pollid_t id,
				any_t           arg,
				tbx_bool_t      first_call)
{
  p_send_poll_arg_t              parg    = NULL;
  int                           request =    0;
  int                           status  =    0;
  void                         *result  = MARCEL_POLL_FAILED;

  LOG_IN();

  parg = arg;
  request = parg->request;

  if (bip_trylock()) 
    {
      status  = bip_stest(request);

      if (status != -1)
	{
	  result = MARCEL_POLL_OK;
	}

      bip_unlock();
    }

  LOG_OUT();

  return result;
}

static void *send_poll_func(marcel_pollid_t id,
			    unsigned        active,
			    unsigned        sleeping,
			    unsigned        blocked)
{
  p_send_poll_arg_t arg;
  int status;

  FOREACH_POLL(id) { GET_ARG(id, arg);

    if (bip_trylock()) 
      {
	status = bip_stest(arg->request);
	bip_unlock();

	if (status != -1) {
	  return MARCEL_POLL_SUCCESS(id);
	}
      }
    }

  return MARCEL_POLL_FAILED;
}

// =============================================================

static
void
bip_sync_send(p_mad_bip_channel_specific_t  p,
	      int  host,
	      int  tag,
	      int *message,
	      int  size)
{
  LOG_IN();

  if(tag == MAD_BIP_SERVICE_TAG)
    marcel_mutex_lock(&_send_mutex);

  send_poll_wait(p, host, tag, message, size);

  if(tag == MAD_BIP_SERVICE_TAG)
    marcel_mutex_unlock(&_send_mutex);

  LOG_OUT();
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
  TRACE("bip_tirecv(tag=%d, size=%d)", tag, size);
  request = bip_tirecv(tag, message, size);
  bip_unlock();

  LOG_OUT();

  return request;
}

static
int
bip_recv_poll(int                           request,
	      int                          *host)
{
  int status = -1;

  //LOG_IN();

  bip_lock();
  status = bip_rtestx(request, host);
  bip_unlock();

  //LOG_OUT();

  return status;
}

static
int
bip_recv_wait(int                           request,
	      int                          *host)
{
  int status = -1;

  LOG_IN();

  for (;;)
    {
      status = bip_recv_poll(request, host);
      
      if (status != -1) 
	break;
      
      marcel_yield();
    }

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
  int request = 0;

  LOG_IN();

  request = bip_recv_post(tag, message, size);
  result = bip_recv_wait(request, host);

  LOG_OUT();

  return result;
}

static __inline__
void
_give_back_credits(new_message_t *header,
		   p_mad_bip_channel_specific_t p,
		   int host)
{
  unsigned shared, private;

  shared = credits_to_give_back[host];
  credits_to_give_back[host] = 0;

  private = p->credits_a_rendre[host];
  p->credits_a_rendre[host] = 0;

  header->credits = MSG_CREDITS_BUILD(shared, private);
}

static __inline__
void
try_to_give_back_credits(new_message_t *header,
			 p_mad_bip_channel_specific_t p,
			 int host)
{
  marcel_mutex_lock(&_cred_mutex);

  _give_back_credits(header, p, host);

  marcel_mutex_unlock(&_cred_mutex);
}

static __inline__
void
update_shared_credits(p_mad_bip_channel_specific_t p, int host)
{
  new_message_t msg;

  marcel_mutex_lock(&_cred_mutex);

  credits_to_give_back[host]++;

  if (credits_to_give_back[host] == BIP_MAX_NEW_CREDITS - 1) {
    // It's time to react!
    msg.header = MSG_HEADER_BUILD(NEW_CRED_TYPE, p->id);

    _give_back_credits(&msg, p, host);

    bip_sync_send(p, host, MAD_BIP_SERVICE_TAG,
		    (int *)&msg, sizeof(msg)/sizeof(int));
  }

  marcel_mutex_unlock(&_cred_mutex);
}

static __inline__
void
update_private_credits(p_mad_bip_channel_specific_t p, int host)
{
  new_message_t msg;

  marcel_mutex_lock(&_cred_mutex);

  p->credits_a_rendre[host]++;

  if (p->credits_a_rendre[host] == BIP_MAX_CREDITS - 1) {

    msg.header = MSG_HEADER_BUILD(CRED_TYPE, p->id);

    _give_back_credits(&msg, p, host);

    bip_sync_send(p, host, MAD_BIP_SERVICE_TAG,
		  (int *)&msg, sizeof(msg)/sizeof(int));
    }

  marcel_mutex_unlock(&_cred_mutex);
}

static __inline__
void
send_ack(p_mad_bip_channel_specific_t p,
	 int                          host_id)
{
  new_message_t msg;

  LOG_IN();

  msg.header = MSG_HEADER_BUILD(ACK_TYPE, p->id);

  // Give credits back
  try_to_give_back_credits(&msg, p, host_id);

  bip_sync_send(p, host_id, MAD_BIP_SERVICE_TAG,
		(int*)&msg, sizeof(msg)/sizeof(int));

  LOG_OUT();
}

static __inline__
void
send_new(p_mad_bip_channel_specific_t p,
	 int                          host_id)
{
  new_message_t msg;

  LOG_IN();

  marcel_mutex_lock(&_new_msg_mutex);

  // We must enforce flow-control for global (NEW) messages
  shared_credits_poll(host_id);

  msg.header = MSG_HEADER_BUILD(NEW_TYPE, p->id);

  // Give credits back
  try_to_give_back_credits(&msg, p, host_id);

  bip_sync_send(p, host_id, MAD_BIP_SERVICE_TAG,
		(int*)&msg, sizeof(msg)/sizeof(int));

  marcel_mutex_unlock(&_new_msg_mutex);

  LOG_OUT();
}

void
mad_bip_register(p_mad_driver_t driver)
{
  p_mad_driver_interface_t interface = NULL;

#ifdef PM2DEBUG
  DEBUG_INIT(bip);
#endif

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
  interface->finalize_message           = mad_bip_finalize_message;
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
  int i;

  LOG_IN();
  /* Driver module initialization code
     Note: called only once, just after
     module registration */
  bip_init ();

  driver_specific  = TBX_CALLOC(1, sizeof(mad_bip_driver_specific_t));
  driver->specific = driver_specific;
  TBX_INIT_SHARED(driver_specific);

  the_channels = tbx_darray_init();

  marcel_mutex_init(&_bip_mutex, NULL);
  marcel_mutex_init(&_cred_mutex, NULL);
  marcel_mutex_init(&_new_msg_mutex, NULL);
  marcel_mutex_init(&_send_mutex, NULL);

  // Fast allocator initialization
  tbx_malloc_init(&bip_buffer_key, MAD_BIP_SMALL_SIZE, 64);

  // Pre-allocate buffer for receiving service messages
  service_buffer = tbx_malloc(bip_buffer_key);

  // Pre-post a BIP_RECV operation on the service TAG
  bip_lock(); // not really necessary at this point...
  service_request = bip_tirecv(MAD_BIP_SERVICE_TAG,
			       (int *)service_buffer, BIP_SMALL_MESSAGE);
  bip_unlock(); // not really necessary at this point...

  // Allocate polling ID for service messages
  service_recv_pollid =
    marcel_pollid_create(recv_msg_group_func,
			 recv_msg_poll_func,
			 recv_msg_fastpoll_func,
			 MARCEL_POLL_AT_TIMER_SIG | MARCEL_POLL_AT_YIELD);

  // Global credits (NEW messages)
  cred_poller = TBX_MALLOC(bip_numnodes * sizeof(marcel_pollinst_t));
  credits_to_give_back = TBX_MALLOC (bip_numnodes * sizeof (int));
  credits_owned = TBX_MALLOC (bip_numnodes * sizeof (int));
  for (i = 0; i < bip_numnodes; i++) {
    credits_to_give_back[i] = 0;
    credits_owned[i] = BIP_MAX_NEW_CREDITS;
    cred_poller[i] = NULL;
  }
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
  adapter->parameter = tbx_string_to_cstring(parameter_string);
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

  // Identifier of the channel
  channel_specific->id = next_channel_num++;

  // Insertion in global channel list
  tbx_darray_expand_and_set(the_channels, channel_specific->id, channel_specific);

  // Credit-based flow-control management
  channel_specific->credits_disponibles = TBX_MALLOC (size * sizeof (int));
  channel_specific->credits_a_rendre    = TBX_MALLOC (size * sizeof (int));
  channel_specific->first_time          = 1;
  channel_specific->ack_was_received    = TBX_MALLOC(size * sizeof(tbx_bool_t));

  // Send buffer to store first chunk (header + data) of messages
  channel_specific->small_buffer = tbx_malloc(bip_buffer_key);
  channel_specific->send_buffer = tbx_malloc(bip_buffer_key);

  channel_specific->msg_was_received = tbx_false;
  channel_specific->recv_poller = NULL;
  channel_specific->message_size = -1;
  channel_specific->message_origin = -1;

  channel_specific->send_mutex  = TBX_MALLOC(size*sizeof(marcel_mutex_t));

  channel_specific->send_pollid =
    marcel_pollid_create(send_group_func, send_poll_func,
			 send_fastpoll_func,
			 MARCEL_POLL_AT_TIMER_SIG | MARCEL_POLL_AT_YIELD);

  marcel_pollid_setspecific(channel_specific->send_pollid, channel_specific);

  channel_specific->ack_poller       =
    TBX_MALLOC(size * sizeof(marcel_pollinst_t));
  channel_specific->cred_poller       =
    TBX_MALLOC(size * sizeof(marcel_pollinst_t));

  for (i = 0; i < size; i++)
    {
      channel_specific->credits_a_rendre[i]    = 0;
      channel_specific->credits_disponibles[i] = BIP_MAX_CREDITS; 
      channel_specific->ack_was_received[i]    = tbx_false;

      marcel_mutex_init(&channel_specific->send_mutex[i], NULL);

      channel_specific->ack_poller[i]       = NULL;
      channel_specific->cred_poller[i]       = NULL;
    }

  /*
   * Note: this part of code is not valid with MadIII
   * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   *
   * -> channels may have different size
   */
  TBX_LOCK_SHARED(driver_specific);
  channel_specific->base_tag  = new_channel;
  new_channel                    += _MAD_BIP_NB_OF_TAGS;
  channel_specific->message_size  = 0;
  TBX_UNLOCK_SHARED(driver_specific);
 
  channel->specific = channel_specific;

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

  TBX_FREE(channel_specific);
  channel->specific = NULL;
  LOG_OUT();
}

void
mad_bip_adapter_exit(p_mad_adapter_t adapter)
{
  LOG_IN();

  TBX_FREE(adapter->parameter);
  adapter->parameter = NULL;

  TBX_FREE(adapter->specific);
  adapter->specific = NULL;

  tbx_malloc_clean(bip_buffer_key);

  LOG_OUT();
}

void
mad_bip_driver_exit(p_mad_driver_t driver)
{
  LOG_IN();

  TBX_FREE (driver->specific);
  driver->specific = NULL;

  tbx_darray_free(the_channels);

  LOG_OUT();
}


void
mad_bip_new_message(p_mad_connection_t out)
{
  p_mad_bip_connection_specific_t out_specific     = NULL;
  p_mad_channel_t                 channel          = NULL;
  p_mad_bip_channel_specific_t    channel_specific = NULL;

  LOG_IN();

  out_specific     = out->specific;
  channel          = out->channel;
  channel_specific = channel->specific;

  TRACE("Threads %p begins a new message", marcel_self());

  out_specific->msg = tbx_true;

  // We must ensure that only one thread can access an outgoing
  // connection (per channel)
  marcel_mutex_lock(&channel_specific->send_mutex[out->remote_rank]);

  LOG_OUT();
}

void
mad_bip_finalize_message(p_mad_connection_t out)
{
  p_mad_channel_t                 channel          = NULL;
  p_mad_bip_channel_specific_t    channel_specific = NULL;

  LOG_IN();

  channel          = out->channel;
  channel_specific = channel->specific;

  marcel_mutex_unlock(&channel_specific->send_mutex[out->remote_rank]);

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

  FAILURE("NOT IMPLEMENTED");

  LOG_OUT();

  return in;
}
#endif // MAD_MESSAGE_POLLING

p_mad_connection_t
mad_bip_receive_message(p_mad_channel_t channel)
{
  p_mad_bip_channel_specific_t    channel_specific = channel->specific;
  p_tbx_darray_t                  in_darray        = channel->in_connection_darray;
  p_mad_connection_t              in               = NULL;
  p_mad_bip_connection_specific_t in_specific      = NULL;
  ntbx_process_lrank_t            lrank            =   -1;

  LOG_IN();

  recv_poll(channel_specific);
  lrank = channel_specific->message_origin;

  // We have just received a "service message", so update corresponding credits
  update_shared_credits(channel_specific, lrank);

  // Get connection-specific data
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
  if(size <= MAD_BIP_SMALL_PDU/2) /* Un peu arbitraire... */
    {
      lnk = connection->link_array[2];
    }
  else
#endif // GROUP_SMALL_PACKS
  if (size <= MAD_BIP_SMALL_PDU)
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

  TRACE("Thread %p sends a buffer", marcel_self());

  out              = lnk->connection;
  out_specific     = out->specific;
  channel          = out->channel;
  channel_specific = channel->specific;

  if (out_specific->msg)
    {
      TRACE("Sending short buffer. Size = %d. First int = %d\n",
	    (buffer->bytes_written), ((int*)buffer->buffer)[0]);

      marcel_mutex_lock(&_new_msg_mutex);

      // We must enforce flow-control for global (NEW) messages
      shared_credits_poll(out->remote_rank);

      ((new_message_t *)channel_specific->send_buffer)->header =
	MSG_HEADER_BUILD(NEW_TYPE, channel_specific->id);

      // Give credits back
      try_to_give_back_credits((new_message_t *)(channel_specific->send_buffer), 
			       channel_specific, out->remote_rank);

      memcpy((char*)channel_specific->send_buffer + sizeof(new_message_t),
	     buffer->buffer,
	     buffer->bytes_written);

      bip_sync_send (channel_specific, out->remote_rank,
		     MAD_BIP_SERVICE_TAG,
		     channel_specific->send_buffer,
		     MAD_NB_INTS(buffer->bytes_written + sizeof(new_message_t)));

      marcel_mutex_unlock(&_new_msg_mutex);

      out_specific->msg = tbx_false;
      out_specific->ack = tbx_false;
    }
  else
    {
      /* Receiver _only_ sends an ACK for the _second_ chunk... */
      if (!out_specific->ack)
	{
	  ack_poll(channel_specific, out->remote_rank);
	  out_specific->ack = tbx_true;
	}
      else
	{
	  TRACE("Skipping wait_ack");
	}

      // We must enforce flow-control for short transfer messages
      private_credits_poll(channel_specific, out->remote_rank);

      bip_sync_send(channel_specific, out->remote_rank,
		    channel_specific->base_tag + MAD_BIP_TRANSFER_TAG,
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
      memcpy (buffer->buffer,
	      (char*)channel_specific->small_buffer + sizeof(new_message_t),
	      channel_specific->message_size * sizeof(int) - sizeof(new_message_t));

      status = channel_specific->message_size - sizeof(new_message_t)/sizeof(int);

      TRACE("Receiving short buffer. Size = %d\n",
	    status * sizeof(int));

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
		      channel_specific->base_tag + MAD_BIP_TRANSFER_TAG,
		      buffer->buffer,
		      MAD_NB_INTS(buffer->length),
		      &lrank);

      TRACE("Receiving short buffer. Size = %d\n",
	    status * sizeof(int));

      // We have just received a "short transfer message", so update corresponding credits
      update_private_credits(channel_specific, lrank);
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

      length = tbx_min(MAD_BIP_MAX_SIZE, (buffer->bytes_written - buffer->bytes_read));

      ack_poll(channel_specific, out->remote_rank);
      out_specific->ack = tbx_true;

      bip_sync_send(channel_specific, out->remote_rank,
		    channel_specific->base_tag + MAD_BIP_TRANSFER_TAG,
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
     in_specific->msg = tbx_false;
     in_specific->ack = tbx_false;
    }

  while (!mad_buffer_full(buffer))
    {
      size_t length = 0;
 
      length = tbx_min(MAD_BIP_MAX_SIZE, (buffer->length - buffer->bytes_written));

      request =
	bip_recv_post(channel_specific->base_tag + MAD_BIP_TRANSFER_TAG,
		      buffer->buffer + buffer->bytes_written, MAD_NB_INTS(length));
      
      buffer->bytes_written += length;

      send_ack(channel_specific, in->remote_rank);
      bip_recv_wait(request, &lrank);
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
  buffer->buffer        = tbx_malloc(bip_buffer_key);
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
  tbx_free(bip_buffer_key, buffer->buffer);
  buffer->buffer   = NULL;
  LOG_OUT();
}
#endif // GROUP_SMALL_PACKS

void
mad_bip_send_buffer(p_mad_link_t   lnk,
		    p_mad_buffer_t buffer)
{
  LOG_IN();
  if ((buffer->bytes_written - buffer->bytes_read) &
      MAD_BIP_SIZE_ALIGNMENT_MASK)
    FAILURE("BIP requires buffer length to be aligned on 4B");

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
      tbx_free(bip_buffer_key, buffer->buffer);
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
      buf->buffer     = tbx_malloc(bip_buffer_key);

      mad_bip_receive_short_buffer(lnk, buf);

      *buffer = buf;
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
