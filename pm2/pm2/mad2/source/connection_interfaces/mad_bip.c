
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

#define BIP_MAX_CREDITS          10

#define BIP_SMALL_MESSAGE        BIPSMALLSIZE
#define MAD_BIP_SMALL_SIZE       (BIP_SMALL_MESSAGE*sizeof(int))

#define MAD_NB_INTS(bytes)    ((bytes) % sizeof(int) ? \
			       (bytes)/sizeof(int)+1 : \
                               (bytes)/sizeof(int))

typedef struct {
  int credits;
} new_message_t;

typedef struct {
  int credits;
} cred_message_t;

typedef struct {
  int credits;
} ack_message_t;

typedef struct {
  TBX_SHARED;
  int nb_adapter;
} mad_bip_driver_specific_t, *p_mad_bip_driver_specific_t;

typedef struct {
  int dummy;
} mad_bip_adapter_specific_t, *p_mad_bip_adapter_specific_t;

typedef struct {
  int communicator ; /* It's a BIP tag! */
  int *small_buffer; /* [BIP_SMALL_MESSAGE] ; */
  int message_size ;

  int *credits_a_rendre ;
  int *credits_disponibles ;
  p_tbx_memory_t bip_buffer_key;
#ifdef MARCEL
  int            request_credits ;
  int            credits_rendus ;

  marcel_mutex_t mutex;
  marcel_cond_t *cond_cred; // tableau
  marcel_sem_t *sem_ack;    // tableau
  int cred_request;
  cred_message_t cred_msg;
  int ack_request;
  ack_message_t ack_msg;
#ifdef USE_MARCEL_POLL
  marcel_pollid_t ack_pollid, cred_pollid, reg_pollid;
  marcel_pollinst_t *ack_poller;
  boolean *ack_was_received;
#endif
#endif
} mad_bip_channel_specific_t, *p_mad_bip_channel_specific_t;

typedef struct {
  int begin_receive, ack_sent;
  int begin_send, ack_received;
} mad_bip_connection_specific_t, *p_mad_bip_connection_specific_t;

typedef struct {
} mad_bip_link_specific_t, *p_mad_bip_link_specific_t;

#ifdef MARCEL
static marcel_mutex_t _bip_mutex;

static __inline__ void bip_lock(void)
{
  marcel_mutex_lock(&_bip_mutex);
}

static __inline__ unsigned bip_trylock(void)
{
  return marcel_mutex_trylock(&_bip_mutex);
}

static __inline__ void bip_unlock(void)
{
  marcel_mutex_unlock(&_bip_mutex);
}

static __inline__ void credits_received(p_mad_bip_channel_specific_t p,
					int host_id, int credits);

#ifdef USE_MARCEL_POLL

typedef struct {
  int host;
} ack_poll_arg_t;

static void ack_poll_wait(p_mad_bip_channel_specific_t p, int host)
{
  ack_poll_arg_t arg;

  LOG_IN();

  arg.host = host;
  marcel_poll(p->ack_pollid, &arg);

  LOG_OUT();
}

static void ack_group_func(marcel_pollid_t id)
{}

static void *ack_fastpoll_func(marcel_pollid_t id,
			       any_t arg,
			       boolean first_call)
{
  unsigned host = ((ack_poll_arg_t *)arg)->host, tmphost;
  int status;
  marcel_pollinst_t poller;
  p_mad_bip_channel_specific_t p =
    (p_mad_bip_channel_specific_t)marcel_pollid_getspecific(id);

  LOG_IN();

  if(first_call) {

    if(p->ack_was_received[host]) {
      p->ack_was_received[host] = FALSE;
      LOG_OUT();
      return MARCEL_POLL_OK;
    }

    p->ack_poller[host] = GET_CURRENT_POLLINST(id);
    LOG_OUT();
    return MARCEL_POLL_FAILED;

  } else { // else if !first_call...

    if(bip_trylock()) {
      status = bip_rtestx(p->ack_request, &tmphost);

      if(status != -1) {

	credits_received(p, tmphost, p->ack_msg.credits);

	p->ack_request =
	  bip_tirecv(p->communicator + MAD_BIP_REPLY_TAG,
		     (int *)&p->ack_msg,
		     sizeof(ack_message_t)/sizeof(int));
	bip_unlock();

	if(tmphost == host) {
	  p->ack_poller[host] = NULL;
	  LOG_OUT();
	  return MARCEL_POLL_OK;
	} else {
	  poller = p->ack_poller[tmphost];
	  if(!poller) {
	    p->ack_was_received[tmphost] = TRUE;
	  } else {
	    p->ack_poller[tmphost] = NULL;
	    LOG_OUT();
	    return MARCEL_POLL_SUCCESS_FOR(poller);
	  }
	}
      } else
	bip_unlock();
    }

    LOG_OUT();
    return MARCEL_POLL_FAILED;
  }
}

static void *ack_poll_func(marcel_pollid_t id,
			   unsigned active,
			   unsigned sleeping,
			   unsigned blocked)
{
  unsigned host;
  marcel_pollinst_t poller;
  p_mad_bip_channel_specific_t p =
    (p_mad_bip_channel_specific_t)marcel_pollid_getspecific(id);
  int status;

  if(bip_trylock()) {
    status = bip_rtestx(p->ack_request, &host);

    if(status != -1) {

      credits_received(p, host, p->ack_msg.credits);

      p->ack_request =
	bip_tirecv(p->communicator + MAD_BIP_REPLY_TAG,
		   (int *)&p->ack_msg,
		   sizeof(ack_message_t)/sizeof(int));
      bip_unlock();

      poller = p->ack_poller[host];
      if(!poller) {
	p->ack_was_received[host] = TRUE;
      } else {
	p->ack_poller[host] = NULL;
	return MARCEL_POLL_SUCCESS_FOR(poller);
      }
    } else
      bip_unlock();
  }

  return MARCEL_POLL_FAILED;
}

#endif // end ifdef USE_MARCEL_POLL

static __inline__ void ack_received(p_mad_bip_channel_specific_t p,
				    int host_id, int credits)
{
  LOG_IN();

  credits_received(p, host_id, credits);
  marcel_sem_V(&p->sem_ack[host_id]);

  LOG_OUT();
}

#endif // end ifdef MARCEL


static void bip_sync_send(int host, int tag, int *message, int size)
{
#ifdef MARCEL // ok if BASIC_POLL is defined

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

#else // else ifndef MARCEL

  LOG_IN();
  bip_tsend(host, tag, message, size);
  LOG_OUT();

#endif
}

static int bip_recv_post(int tag, int *message, int size)
{
  int request;

  LOG_IN();

#ifdef MARCEL
  bip_lock();
  request = bip_tirecv(tag, message, size);
  bip_unlock();
#else
  request = bip_tirecv (tag, message, size);
#endif

  LOG_OUT();

  return request;
}

static int bip_recv_poll(p_mad_bip_channel_specific_t p, int request, int *host)
{
#if defined(MARCEL) && !defined(BASIC_POLL)

  int tmphost, status;
  static which = 0;

  LOG_IN();

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
#ifndef USE_MARCEL_POLL
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
#else // else ifdef USE_MARCEL_POLL
    case 1 : {
      break;
    }
#endif
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

  LOG_OUT();
  return -1;

#elif defined(BASIC_POLL)

  int status;

  bip_lock();
  status = bip_rtestx(request, host);
  bip_unlock();

  return status;

#else // else ifndef BASIC_POLL nor MARCEL

  LOG_IN();
  return bip_rtestx(request, host);
  LOG_OUT();

#endif
}

static int bip_recv_wait(p_mad_bip_channel_specific_t p, int request, int *host)
{
#ifdef MARCEL
  int status;

  LOG_IN();

  for(;;) {

    status = bip_recv_poll(p, request, host);

    if(status != -1) {
      LOG_OUT();
      return status;
    }

    marcel_yield();
  };

  /* Never reached */
  fprintf(stderr, "Oh my God! We should never execute that code!\n");
  return -1;
#else
  return bip_rwaitx(request, host);
#endif
}

static __inline__ int bip_sync_recv (p_mad_bip_channel_specific_t p,
				     int tag, int *message, int size, int *host)
{
  LOG_IN();

#ifdef MARCEL
  return bip_recv_wait(p, bip_recv_post(tag, message, size), host);
#else
  return bip_trecvx(tag, message, size, host);
#endif

  LOG_OUT();
}

static __inline__ int bip_probe_recv(p_mad_bip_channel_specific_t p,
				     int tag, int *message, int size, int *host)
{
  static int request = -1;
  int status;
  LOG_IN();

  if(request == -1)
    request = bip_recv_post(tag, message, size);

  status = bip_recv_poll(p, request, host);

  if(status != -1) {
    request = -1; /* pour la prochaine fois */
  }

  LOG_OUT();
  return status;

  LOG_OUT();
}

static __inline__ void send_ack(p_mad_bip_channel_specific_t p, int host_id)
{
  ack_message_t msg;

  LOG_IN();

  msg.credits = p->credits_a_rendre[host_id];
  p->credits_a_rendre[host_id] = 0;

  bip_sync_send(host_id,
		p->communicator + MAD_BIP_REPLY_TAG,
		(int*)&msg,
		sizeof(msg)/sizeof(int));

  LOG_OUT();
}

static __inline__ void wait_ack(p_mad_bip_channel_specific_t p, int host_id)
{
  LOG_IN();

  TRACE("Waiting for ack\n");

#if defined(MARCEL) && !defined(BASIC_POLL)

#ifdef USE_MARCEL_POLL
  ack_poll_wait(p, host_id);
#else
  marcel_sem_P(&p->sem_ack[host_id]);
#endif

#else
  {
    ack_message_t msg;
    int remote;

    bip_sync_recv(p, p->communicator+MAD_BIP_REPLY_TAG,
		  (int*)&msg, sizeof(msg)/sizeof(int), &remote);

    p->credits_disponibles[host_id] += msg.credits;
  }
#endif

  LOG_OUT();
}

static __inline__ void send_new(p_mad_bip_channel_specific_t p, int host_id)
{
  new_message_t msg;

  LOG_IN();

  msg.credits = p->credits_a_rendre[host_id];
  p->credits_a_rendre[host_id] = 0;

  bip_sync_send(host_id,
		p->communicator + MAD_BIP_SERVICE_TAG,
		(int*)&msg,
		sizeof(msg)/sizeof(int));

  LOG_OUT();
}

static void wait_credits(p_mad_bip_channel_specific_t p, int host_id)
{
  LOG_IN();

  TRACE("Checking credits\n");

#ifdef MARCEL

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
  while (p->credits_disponibles [host_id] == 0) {
    int remote ;
    cred_message_t msg;

    bip_sync_recv(p, p->communicator + MAD_BIP_FLOW_CONTROL_TAG, (int *)&msg,
		  sizeof(msg)/sizeof(int), &remote);

    p->credits_disponibles [remote] += msg.credits;
  }
  p->credits_disponibles[host_id]--;
#endif

 LOG_OUT();
}

static __inline__ void credits_received(p_mad_bip_channel_specific_t p,
					int host_id, int credits)
{
  LOG_IN();

#ifdef MARCEL
  if(credits) {
    marcel_mutex_lock(&p->mutex);

    TRACE("Credits received from host %d\n", host_id);

    p->credits_disponibles[host_id] += credits;
    marcel_cond_signal(&p->cond_cred[host_id]);
    marcel_mutex_unlock(&p->mutex);
  }
#else
     p->credits_disponibles[host_id] += credits;
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

  if((p->credits_a_rendre[host_id]) == (BIP_MAX_CREDITS-1)) {

    msg.credits = p->credits_a_rendre[host_id];
    p->credits_a_rendre[host_id] = 0 ;

    marcel_mutex_unlock(&p->mutex);

    bip_sync_send(host_id, p->communicator + MAD_BIP_FLOW_CONTROL_TAG,
		  (int *)&msg, sizeof(cred_message_t)/sizeof(int));
  } else {
    marcel_mutex_unlock(&p->mutex);
    TRACE("Give_back_credits : %d credits to give, so forget it...",
	  p->credits_a_rendre[host_id]);
  }

#else

  p->credits_a_rendre [host_id] ++ ;
  if((p->credits_a_rendre [host_id]) == (BIP_MAX_CREDITS-1)) {
      msg.credits = p->credits_a_rendre [host_id];
      p->credits_a_rendre [host_id] = 0 ;

      bip_sync_send(host_id, p->communicator + MAD_BIP_FLOW_CONTROL_TAG,
		    (int *)&msg, sizeof(msg)/sizeof(int));
  }

#endif

  LOG_OUT();
}

void
mad_bip_register(p_mad_driver_t driver)
{
  p_mad_driver_interface_t interface;

#ifdef DEBUG
  DEBUG_INIT(bip);
#endif

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
#ifdef GROUP_SMALL_PACKS
  interface->get_static_buffer          = mad_bip_get_static_buffer;
  interface->return_static_buffer       = mad_bip_return_static_buffer;
#else
  interface->get_static_buffer          = NULL;
  interface->return_static_buffer       = NULL;
#endif
  interface->new_message                = mad_bip_new_message;
#ifdef MAD_MESSAGE_POLLING
  interface->poll_message               = mad_bip_poll_message;
#endif /* MAD_MESSAGE_POLLING */
  interface->receive_message            = mad_bip_receive_message;
  interface->send_buffer                = mad_bip_send_buffer;
  interface->receive_buffer             = mad_bip_receive_buffer;
  interface->send_buffer_group          = mad_bip_send_buffer_group;
  interface->receive_sub_buffer_group   = mad_bip_receive_sub_buffer_group;
  interface->external_spawn_init        = mad_bip_external_spawn_init;
  interface->configuration_init         = mad_bip_configuration_init;
  interface->send_adapter_parameter     = mad_bip_send_adapter_parameter;
  interface->receive_adapter_parameter  = mad_bip_receive_adapter_parameter;

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
  p_mad_bip_driver_specific_t driver_specific =
    (p_mad_bip_driver_specific_t)channel->adapter->driver->specific;
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
  channel_specific->cond_cred = (marcel_cond_t *)TBX_MALLOC(size*sizeof(marcel_cond_t));
  channel_specific->sem_ack = (marcel_sem_t *)TBX_MALLOC(size*sizeof(marcel_sem_t));
  marcel_mutex_init(&channel_specific->mutex, NULL);

#ifdef USE_MARCEL_POLL
  channel_specific->ack_pollid = marcel_pollid_create(ack_group_func,
						      ack_poll_func,
						      ack_fastpoll_func,
						      MARCEL_POLL_AT_TIMER_SIG |
						      MARCEL_POLL_AT_YIELD);

  marcel_pollid_setspecific(channel_specific->ack_pollid,
			    channel_specific);

  channel_specific->ack_was_received =
    (boolean *)TBX_MALLOC(size*sizeof(boolean));
  channel_specific->ack_poller =
    (marcel_pollinst_t *)TBX_MALLOC(size*sizeof(marcel_pollinst_t));
#endif

#endif

  for (i=0; i < size; i++) {
    channel_specific->credits_a_rendre[i] = 0 ;
    channel_specific->credits_disponibles[i] = BIP_MAX_CREDITS ; 
#ifdef MARCEL
    marcel_cond_init(&channel_specific->cond_cred[i], NULL);
    marcel_sem_init(&channel_specific->sem_ack[i], 0);
#ifdef USE_MARCEL_POLL
    channel_specific->ack_was_received[i] = FALSE;
    channel_specific->ack_poller[i] = NULL;
#endif
#endif
  }

  tbx_malloc_init(&(channel_specific->bip_buffer_key),
		  MAD_BIP_SMALL_SIZE,
		  32);

  TBX_LOCK_SHARED(driver_specific); 

  channel_specific->communicator = new_channel ;
  new_channel += MAD_BIP_NB_OF_TAGS ;
  channel_specific->message_size = 0 ;

  TBX_UNLOCK_SHARED(driver_specific) ; 
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
  out->specific = connection_specific ;
#ifdef GROUP_SMALL_PACKS
  in->nb_link = 3;
  out->nb_link = 3;
#else
  in->nb_link = 2;
  out->nb_link = 2;
#endif

  LOG_OUT();
}


void 
mad_bip_link_init(p_mad_link_t lnk)
{

  LOG_IN();
  /* Link initialization code */

#ifdef GROUP_SMALL_PACKS
  if(lnk->id == 2) {
    lnk->link_mode   = mad_link_mode_buffer;
    lnk->buffer_mode = mad_buffer_mode_static;
    lnk->group_mode  = mad_group_mode_aggregate;
  } else {
#endif
    lnk->link_mode   = mad_link_mode_buffer;
    lnk->buffer_mode = mad_buffer_mode_dynamic;
    lnk->group_mode  = mad_group_mode_split;
#ifdef GROUP_SMALL_PACKS
  }
#endif
  LOG_OUT();
}

void
mad_bip_channel_exit(p_mad_channel_t channel)
{  
  p_mad_bip_channel_specific_t channel_specific = channel->specific;
  
  LOG_IN();

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

  driver->specific = NULL;
  LOG_OUT();
}


void
mad_bip_new_message(p_mad_connection_t connection)
{
  p_mad_bip_connection_specific_t connection_specific = connection->specific ;
#ifdef MARCEL
  TRACE("Threads %p begins a new message", marcel_self());
#endif

  LOG_IN();

  connection_specific->begin_send = 1 ;

  LOG_OUT();

  return ;
}

#ifdef MAD_MESSAGE_POLLING
p_mad_connection_t 
mad_bip_poll_message(p_mad_channel_t channel)
{
  p_mad_bip_channel_specific_t channel_specific = channel->specific;
  p_mad_connection_t    connection       = NULL;
  p_mad_bip_connection_specific_t connection_specific = NULL ;
  int host ;
  static int first_time = 1;

  LOG_IN();

  if(first_time) {
    channel_specific->small_buffer =
      (int*)tbx_malloc(channel_specific->bip_buffer_key);
    first_time = 0;
  }

  channel_specific->message_size = bip_probe_recv(
       channel_specific,
       channel_specific->communicator+MAD_BIP_SERVICE_TAG, 
       (int *) channel_specific->small_buffer, 
       BIP_SMALL_MESSAGE,
       &host);

  if(channel_specific->message_size == -1) {
    LOG_OUT();
    return NULL;
  }

  first_time = 1;

  connection = &(channel->input_connection[host]);
  connection_specific = connection->specific ;      
  connection_specific->begin_receive = 1 ;

  LOG_OUT();

  return connection;
}
#endif

p_mad_connection_t
mad_bip_receive_message(p_mad_channel_t channel)
{
  p_mad_bip_channel_specific_t channel_specific = channel->specific;
  p_mad_connection_t    connection       = NULL;
  p_mad_bip_connection_specific_t connection_specific = NULL ;
  int host ;

  LOG_IN();

  channel_specific->small_buffer =
    (int*)tbx_malloc(channel_specific->bip_buffer_key);

  channel_specific->message_size = bip_sync_recv(
       channel_specific,
       channel_specific->communicator+MAD_BIP_SERVICE_TAG, 
       (int *) channel_specific->small_buffer, 
       BIP_SMALL_MESSAGE,
       &host);

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
  int l;  

  LOG_IN();
  
  l = strlen(parameter) + 1;
  

  bip_tsend (remote_host_id, 0, (int*)&l, 1);
  bip_tsend (remote_host_id, 0, (int*)parameter, MAD_NB_INTS(l));
  

      /*
	Note: Ack Olivier


	printf ("NOT YET IMPLEMENTED call a mad_bip_send_adapter_parameter\n") ;
	fflush (stdout) ;
	
	exit (-1) ;
      */
  LOG_OUT();
}

void
mad_bip_receive_adapter_parameter(p_mad_adapter_t   spawn_adapter
				  __attribute__ ((unused)),
				  char            **parameter)
{
  int l;

  LOG_IN();

  bip_trecv (0, &l, 1);
  *parameter = TBX_MALLOC(MAD_NB_INTS(l));
  CTRL_ALLOC(*parameter);
  bip_trecv (0, (int*)*parameter, MAD_NB_INTS(l));

  /*
    Note: Ack olivier
  printf ("NOT YET IMPLEMENTED call a mad_bip_receive_adapter_parameter\n") ;
  fflush (stdout) ;
  exit (-1) ;
  */
  LOG_OUT();
}


p_mad_link_t
mad_bip_choice(p_mad_connection_t connection,
		 size_t             size         __attribute__ ((unused)),
		 mad_send_mode_t    send_mode    __attribute__ ((unused)),
		 mad_receive_mode_t receive_mode __attribute__ ((unused)))
{
#ifdef GROUP_SMALL_PACKS
  if(size <= MAD_BIP_SMALL_SIZE/2) /* Un peu arbitraire... */
    return &connection->link[2];
  else
#endif
  if (size <= MAD_BIP_SMALL_SIZE)
    return &connection->link[0];
  else
    return &connection->link[1];
}


static void
mad_bip_send_short_buffer(p_mad_link_t     lnk,
			  p_mad_buffer_t   buffer)
{
  p_mad_connection_t           connection       = lnk->connection;
  p_mad_bip_connection_specific_t connection_specific = connection->specific ;
  p_mad_channel_t              channel          = connection->channel;
  p_mad_bip_channel_specific_t channel_specific = channel->specific;

#ifdef MARCEL
  TRACE("Thread %p sends a buffer", marcel_self());
#endif

  LOG_IN();

  if (connection_specific->begin_send == 1)
    {
      TRACE("Sending short buffer. Size = %d. First int = %d\n",
	    (buffer->bytes_written), ((int*)buffer->buffer)[0]);

      bip_sync_send (connection->remote_host_id,
		     channel_specific->communicator+MAD_BIP_SERVICE_TAG,
		     buffer->buffer,
		     MAD_NB_INTS(buffer->bytes_written));

      connection_specific->begin_send = 0 ;
      connection_specific->ack_received = 0;
    }
  else
    {
      /* On n'envoie un ACK *que* pour le second paquet... */
      if(!connection_specific->ack_received) {
	wait_ack(channel_specific, connection->remote_host_id);
	connection_specific->ack_received = 1;
      } else {
	TRACE("Skipping wait_ack");
      }

      wait_credits(channel_specific, connection->remote_host_id);

      bip_sync_send(connection->remote_host_id,
		    channel_specific->communicator+MAD_BIP_TRANSFER_TAG,
		    buffer->buffer,
		    MAD_NB_INTS(buffer->bytes_written));

      TRACE("Sending short buffer. Size = %d. First int = %d\n",
	    (buffer->bytes_written), ((int*)buffer->buffer)[0]);
    }
  buffer->bytes_read = buffer->bytes_written;

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

  int  host ;
  int  status ;

  LOG_IN();

  if (connection_specific->begin_receive == 1)
    {
      /* Normalement, on n'arrive JAMAIS ici si buffer = statique ! */

      memcpy (buffer->buffer,
	      channel_specific->small_buffer,
	      channel_specific->message_size*sizeof(int)) ;

      status = channel_specific->message_size ;

      TRACE("Receiving short buffer. Size = %d. First int = %d\n",
	    channel_specific->message_size*sizeof(int),
	    ((int*)channel_specific->small_buffer)[0]);

      tbx_free(channel_specific->bip_buffer_key,
	       channel_specific->small_buffer);

      connection_specific->begin_receive = 0 ;      
      connection_specific->ack_sent = 0;
    }
  else
    {
      /* On n'envoie un ACK *que* pour le second paquet... */
      if(!connection_specific->ack_sent) {
	send_ack(channel_specific, connection->remote_host_id);
	connection_specific->ack_sent = 1;
      } else {
	TRACE("Skipping send_ack");
      }

      status = bip_sync_recv(channel_specific,
			     channel_specific->communicator+MAD_BIP_TRANSFER_TAG,
			     buffer->buffer,
			     MAD_NB_INTS(buffer->length),
			     &host);

      TRACE("Receiving short buffer. Size = %d. First int = %d\n",
	    status*sizeof(int),
	    ((int*)buffer->buffer)[0]);

      give_back_credits (channel_specific, connection->remote_host_id);
    }

  buffer->bytes_written = status*sizeof(int);

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
      send_new(channel_specific, connection->remote_host_id);

      connection_specific->begin_send = 0;
      connection_specific->ack_received = 0;
    }

  wait_ack(channel_specific, connection->remote_host_id);

  connection_specific->ack_received = 1;

  bip_sync_send(connection->remote_host_id,
		channel_specific->communicator+MAD_BIP_TRANSFER_TAG,
		buffer->buffer,
		MAD_NB_INTS(buffer->bytes_written));

  buffer->bytes_read = buffer->bytes_written;

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

  int request;
  int host, status;

  LOG_IN();
  /* Code to receive one buffer */

  if (connection_specific->begin_receive == 1)
    {
      credits_received(channel_specific,
		       connection->remote_host_id,
		       ((new_message_t *)(channel_specific->small_buffer))->credits);

      tbx_free(channel_specific->bip_buffer_key,
	       channel_specific->small_buffer);

     connection_specific->begin_receive = 0 ;
     connection_specific->ack_sent = 0;
    }

  request = bip_recv_post(channel_specific->communicator+MAD_BIP_TRANSFER_TAG,
			  buffer->buffer,
			  MAD_NB_INTS(buffer->length));

  send_ack(channel_specific, connection->remote_host_id);

  status = bip_recv_wait(channel_specific, request, &host);

  connection_specific->ack_sent = 1;
  connection_specific->begin_receive = 0 ;

  buffer->bytes_written = status * sizeof(int);

  LOG_OUT();
}

#ifdef GROUP_SMALL_PACKS

/* Renvoie un buffer tout neuf qui sera rempli par l'application puis
   émis... */
p_mad_buffer_t
mad_bip_get_static_buffer(p_mad_link_t lnk)
{
  p_mad_bip_channel_specific_t channel_specific  =
    (p_mad_bip_channel_specific_t)lnk->connection->channel->specific;
  p_mad_buffer_t               buffer;
  
  LOG_IN();
  buffer = mad_alloc_buffer_struct();

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
mad_bip_return_static_buffer(p_mad_link_t     lnk,
			     p_mad_buffer_t   buffer)
{
  p_mad_bip_channel_specific_t channel_specific =
    (p_mad_bip_channel_specific_t)lnk->connection->channel->specific;
  
  LOG_IN();

  tbx_free(channel_specific->bip_buffer_key, buffer->buffer);
  buffer->buffer = NULL;

  LOG_OUT();
}
#endif

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
      p_mad_bip_channel_specific_t channel_specific =
	(p_mad_bip_channel_specific_t)lnk->connection->channel->specific;

      mad_bip_send_short_buffer(lnk, buffer);
      tbx_free(channel_specific->bip_buffer_key, buffer->buffer);
      buffer->buffer = NULL;
    }
#endif
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
      p_mad_bip_connection_specific_t connection_specific =
	(p_mad_bip_connection_specific_t)lnk->connection->specific;
      p_mad_bip_channel_specific_t channel_specific  =
	(p_mad_bip_channel_specific_t)lnk->connection->channel->specific;
      p_mad_buffer_t buf;

      buf = mad_alloc_buffer_struct();
      *buffer = buf;

      buf->length        = MAD_BIP_SMALL_SIZE;
      buf->bytes_read    = 0;
      buf->type          = mad_static_buffer;
  
      if (connection_specific->begin_receive == 1) {
	buf->buffer        = channel_specific->small_buffer;
	buf->bytes_written = channel_specific->message_size*sizeof(int);

	connection_specific->begin_receive = 0 ;      
	connection_specific->ack_sent = 0;
      } else {
	buf->buffer        = tbx_malloc(channel_specific->bip_buffer_key);
	mad_bip_receive_short_buffer(lnk, *buffer);
      }

    }
#endif
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
				 __attribute__ ((unused)),
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
