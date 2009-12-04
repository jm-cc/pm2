/* 
 * This file has been imported from PVFS2 implementation of BMI
 *
 * (C) 2001 Clemson University and The University of Chicago
 *
 * See COPYING in top-level directory.
 *
 * NewMadeleine
 * Copyright (C) 2009 (see AUTHORS file)
 */

/** \file
 *  \ingroup bmiint
 *
 *  Top-level BMI network interface routines.
 */

#include <nm_public.h>

/* BMI is only available for 'huge tags' (ie. at least 64 bits) */
#ifdef NM_TAGS_AS_INDIRECT_HASH

#include <errno.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <inttypes.h>

#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <poll.h>
#include <sys/ioctl.h>

#include "bmi.h"
#include "bmi-types.h"
#include "reference-list.h"
#include "op-list.h"
#include "str-utils.h"

#include "nmad_bmi_interface.h"

#include <nm_sendrecv_interface.h>

#define DEBUG 1

#define SERVER 0

#define NM_BMI_REQUEST_PREALLOC 128

p_tbx_memory_t nm_bmi_mem;
static int bmi_initialized_count = 0;

/*
 * List of BMI addrs currently managed.
 */
static ref_list_p cur_ref_list = NULL;
/* 
 * List of BMI addr, indexed by p_gates 
 */
static ref_list_p gate_ref_list = NULL;

/* array to keep up with active contexts */
static int context_array[BMI_MAX_CONTEXTS] = { 0 };

#ifdef MARCEL
static marcel_mutex_t bmi_initialize_mutex = MARCEL_MUTEX_INITIALIZER;
static marcel_mutex_t context_mutex = MARCEL_MUTEX_INITIALIZER;
static marcel_mutex_t ref_mutex = MARCEL_MUTEX_INITIALIZER;
#endif

struct forget_item{
    struct tbx_fast_list_head link;
    BMI_addr_t addr;
};

#ifdef MARCEL
static marcel_mutex_t active_method_count_mutex = MARCEL_MUTEX_INITIALIZER;
#endif

static const int usage_iters_starvation = 100000;
static const int usage_iters_active = 10000;
static int server = 0;


nm_session_t p_core;
const char*local_session_url = NULL;


static inline
BMI_addr_t __BMI_get_ref(BMI_addr_t peer){
    BMI_addr_t ret = NULL;
#ifdef MARCEL
    marcel_mutex_lock(&ref_mutex);
#endif
    ret = ref_list_search_addr(cur_ref_list, peer);
#ifdef MARCEL
    marcel_mutex_unlock(&ref_mutex);
#endif
    return ret;
}


static void
bnm_create_match( struct bnm_ctx* ctx){
    int      connect = 0;
    uint64_t type    = (uint64_t) ctx->nmc_msg_type;
    uint64_t id      = 0ULL;
    uint64_t tag     = (uint64_t) ctx->nmc_tag;
    
    if (ctx->nmc_msg_type == BNM_MSG_CONN_REQ || 
	ctx->nmc_msg_type == BNM_MSG_CONN_ACK) {
	connect = 1;
    }
    
    if ((ctx->nmc_type == BNM_REQ_TX && connect == 0) ||
	(ctx->nmc_type == BNM_REQ_RX && connect == 1)) {
	id = (uint64_t) ctx->nmc_peer->nmp_tx_id;
    } else if ((ctx->nmc_type == BNM_REQ_TX && connect == 1) ||
	       (ctx->nmc_type == BNM_REQ_RX && connect == 0)) {
	id = (uint64_t) ctx->nmc_peer->nmp_rx_id;
    } 
    
#ifdef DEBUG
    fprintf(stderr, " (%"PRIx64" << %d) | (%"PRIx64" << %d) | %"PRIx64"\n",type, BNM_MSG_SHIFT,id, BNM_ID_SHIFT, tag);
    fprintf(stderr,"Tag match: %"PRIx64"\n",  (type << BNM_MSG_SHIFT) | (id << BNM_ID_SHIFT) | tag);
#endif	/* DEBUG */
    ctx->nmc_match = (type << BNM_MSG_SHIFT) | (id << BNM_ID_SHIFT) | tag;
    
    return;
}

static inline void 
__bmi_peer_init(struct bnm_peer *p_peer)
{
    memset(p_peer, 0, sizeof(struct bnm_peer));
}

static int max_tx_id = 1;
static int local_tx_id = -1;

static void __bmi_launcher_addr_send(int sock, const char*url)
{
  int len = strlen(url) + 1 ;
  int rc = send(sock, &len, sizeof(len), 0);
  if(rc < 0)
  {
      fprintf(stderr, "#launcher: send erro (%s)\n", strerror(errno));
      abort();
  }
  if(rc != sizeof(len))
    {
      fprintf(stderr, "# launcher: cannot send address to peer.\n");
      abort();
    }
  rc = send(sock, url, len, 0);
  if(rc < 0)
  {
      fprintf(stderr, "#launcher: send erro (%s)\n", strerror(errno));
      abort();
  }
  if(rc != len)
    {
      fprintf(stderr, "# launcher: cannot send address to peer.\n");
      abort();
    }
}

static void __bmi_launcher_addr_recv(int sock, char**p_url)
{
  int len = -1;
  int rc = recv(sock, &len, sizeof(len), MSG_WAITALL);
  if(rc < 0)
  {
      fprintf(stderr, "#launcher: send erro (%s)\n", strerror(errno));
      abort();
  }
  if(rc != sizeof(len))
    {
      fprintf(stderr, "# launcher: cannot get address from peer.\n");
      abort();
    }
  char*url = TBX_MALLOC(len);
  rc = recv(sock, url, len, MSG_WAITALL);
  if(rc < 0)
  {
      fprintf(stderr, "#launcher: send erro (%s)\n", strerror(errno));
      abort();
  }
  if(rc != len)
    {
      fprintf(stderr, "# launcher: cannot get address from peer.\n");
      abort();
    }
  *p_url = url;
}

/* connect to a peer and fill all the necessary structures */
void __bmi_connect_accept(BMI_addr_t addr, char* remote_session_url)
{
    int err = nm_session_connect(p_core, &addr->p_gate, remote_session_url);
    if (err != NM_ESUCCESS)
    {
	fprintf(stderr, "launcher: nm_session_connect returned err = %d\n", err);
	abort();
    }
    if(addr->p_gate == NULL)
	*(int*)0=0;
    addr->p_peer = malloc(sizeof(struct bnm_peer));

    __bmi_peer_init(addr->p_peer);
    addr->p_peer->p_gate    = addr->p_gate;
    addr->p_peer->nmp_state = BNM_PEER_READY;
    addr->p_peer->nmp_rx_id = max_tx_id++;
    addr->p_peer->peername  = addr->peername;
#ifdef MARCEL
    marcel_mutex_init (&addr->p_peer->nmp_lock, NULL);
#endif	/* MARCEL */
    
    /* now that the peers are connected, we have to exchange :
     * - the local hostname
     * - the tx_id 
     */
    nm_sr_request_t rrequest1, rrequest2, srequest1, srequest2;
    struct bnm_ctx rx;
    rx.nmc_state    = BNM_CTX_PREP;
    rx.nmc_tag      = 42;
    rx.nmc_type     = BNM_REQ_TX;
    rx.nmc_msg_type = BNM_MSG_ICON_ACK;
    rx.nmc_peer     = addr->p_peer;

    bnm_create_match(&rx);

    char* local_hostname=malloc(sizeof(char)*256);
    gethostname(local_hostname, 256);

  
    nm_sr_isend(p_core, addr->p_gate, rx.nmc_match, 
		&addr->p_peer->nmp_rx_id, sizeof(addr->p_peer->nmp_rx_id), &srequest1);


    /* retrieve remote tx_id */
    nm_sr_irecv(p_core, addr->p_gate, rx.nmc_match, 
		&addr->p_peer->nmp_tx_id, sizeof(addr->p_peer->nmp_tx_id), &rrequest1);

    addr->peername=malloc(sizeof(char)*256);

    nm_sr_swait(p_core, &srequest1);
    nm_sr_rwait(p_core, &rrequest1);

    nm_sr_isend(p_core, addr->p_gate, rx.nmc_match, 
		local_hostname, strlen(local_hostname), &srequest2);

    /* retrieve remote hostname */
    nm_sr_irecv(p_core, addr->p_gate, rx.nmc_match, 
		addr->peername, sizeof(char)*256, &rrequest2);
    
    nm_sr_swait(p_core, &srequest2);
    nm_sr_rwait(p_core, &rrequest2);

    addr->p_peer->peername=malloc(sizeof(char)*256);

    strcpy((char*)addr->p_peer->peername, addr->peername);

    /* todo: fix this ! */
    ref_list_add(cur_ref_list, addr->p_peer);    
    //gate_list_add(cur_ref_list, addr->p_peer);    
    return;
}

/* Wait for an incoming connection */
void __bmi_accept(BMI_addr_t addr)
{
#ifdef DEBUG
    fprintf(stderr, "bmi_accept\n");
#endif
    /* server */
    char*remote_session_url = NULL;
    char local_launcher_url[16] = { 0 };
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    assert(server_sock > -1);
    struct sockaddr_in addr_in;
    unsigned addr_len = sizeof addr_in;
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(0);
    addr_in.sin_addr.s_addr = INADDR_ANY;
    int rc = bind(server_sock, (struct sockaddr*)&addr_in, addr_len);
    if(rc) 
    {
	fprintf(stderr, "# launcher: bind error (%s)\n", strerror(errno));
	abort();
    }
    rc = getsockname(server_sock, (struct sockaddr*)&addr_in, &addr_len);
    listen(server_sock, 255);
      
    struct ifaddrs*ifa_list = NULL;
    rc = getifaddrs(&ifa_list);
    if(rc == 0)
    {
	struct ifaddrs*i;
	for(i = ifa_list; i != NULL; i = i->ifa_next)
	{
	    if (i->ifa_addr && i->ifa_addr->sa_family == AF_INET)
	    {
		struct sockaddr_in*inaddr = (struct sockaddr_in*)i->ifa_addr;
		if(!(i->ifa_flags & IFF_LOOPBACK))
		{
		    snprintf(local_launcher_url, 16, "%08x%04x", htonl(inaddr->sin_addr.s_addr), addr_in.sin_port);
		    break;
		}
	    }
	}
    }
    if(local_launcher_url[0] == '\0')
    {
	fprintf(stderr, "# launcher: cannot get local address\n");
	abort();
    } 
    fprintf(stderr, "# laucher: local url = '%s'\n", local_launcher_url);
    int sock = accept(server_sock, (struct sockaddr*)&addr_in, &addr_len);
    assert(sock > -1);
    close(server_sock);
    __bmi_launcher_addr_send(sock, local_session_url);
    __bmi_launcher_addr_recv(sock, &remote_session_url);
    close(sock);
    __bmi_connect_accept(addr , remote_session_url);
}

static struct pollfd server_pollfd[10];
static int server_sock;
static struct sockaddr_in addr_in;
static unsigned addr_len;

/* Test for an incoming connection 
 * Return 1 if connection succeeds, 0 otherwise
 */
int __bmi_test_accept(BMI_addr_t addr)
{
    short returned_events;
    char*remote_session_url = NULL;

    server_pollfd[0].fd=server_sock;
    server_pollfd[0].events=POLLIN|POLLPRI;
    server_pollfd[0].revents=returned_events;

    int sock = poll(server_pollfd, 1, 0);
    if(sock < 0) {
	fprintf(stderr, "# launcher: poll error (%s)\n", strerror(errno));
	abort();	    
    }
    if( sock ) {
#ifdef DEBUG
	fprintf(stderr, "bmi_test_accept succeeds: someone is connecting\n");
#endif
	/* someone is trying to connect, accept the connection */
	sock = accept(server_sock,  (struct sockaddr*)&addr_in, &addr_len);
	assert(sock > -1);
#ifdef DEBUG
	fprintf(stderr, "\t exchanging session_ruls\n");
#endif
	/* exchange session url */
	__bmi_launcher_addr_send(sock, local_session_url);
#ifdef DEBUG
	fprintf(stderr, "\t\tI have just sent %s\n", local_session_url);
#endif
	__bmi_launcher_addr_recv(sock, &remote_session_url);
#ifdef DEBUG
	fprintf(stderr, "\t\tI have just received %s\n", remote_session_url);
#endif
	close(sock);
	__bmi_connect_accept(addr , remote_session_url);
#ifdef DEBUG
	fprintf(stderr, "Connection OK ! Ready to send/recv messages\n");
#endif
	return 1;
    }
    return 0;
}

/* Wait for an incoming connection */
void __bmi_wait_accept(BMI_addr_t addr)
{
    /* todo: use a blocking call here */
    while(! __bmi_test_accept(addr)) { }
}

/* non-blocking accept */
void __bmi_iaccept()
{
    /* server */
    char local_launcher_url[16] = { 0 };
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    assert(server_sock > -1);
    addr_len = sizeof addr_in;
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(0);
    addr_in.sin_addr.s_addr = INADDR_ANY;
    
    int on = 1;
    int rc = setsockopt(server_sock, SOL_SOCKET,  SO_REUSEADDR,
			(char *)&on, sizeof(on));
    if (rc < 0)
    {
	fprintf(stderr, "#launcher setsockopt error (%s)", strerror(errno));
	abort();
    }

    /* set the socket non blocking */
    rc = ioctl(server_sock, FIONBIO, (char*)&on);
    if(rc)
    {
	fprintf(stderr, "# launcher ioctl error (%s)\n", strerror(errno));
	abort();
    }
    
    rc = bind(server_sock, (struct sockaddr*)&addr_in, addr_len);
    if(rc) 
    {
	fprintf(stderr, "# launcher: bind error (%s)\n", strerror(errno));
	abort();
    }
    rc = getsockname(server_sock, (struct sockaddr*)&addr_in, &addr_len);
    listen(server_sock, 255);
      
    /* get the local url */
    struct ifaddrs*ifa_list = NULL;
    rc = getifaddrs(&ifa_list);
    if(rc == 0)
    {
	struct ifaddrs*i;
	for(i = ifa_list; i != NULL; i = i->ifa_next)
	{
	    if (i->ifa_addr && i->ifa_addr->sa_family == AF_INET)
	    {
		struct sockaddr_in*inaddr = (struct sockaddr_in*)i->ifa_addr;
		if(!(i->ifa_flags & IFF_LOOPBACK))
		{
		    snprintf(local_launcher_url, 16, "%08x%04x", htonl(inaddr->sin_addr.s_addr), addr_in.sin_port);
		    break;
		}
	    }
	}
    }
    if(local_launcher_url[0] == '\0')
    {
	fprintf(stderr, "# launcher: cannot get local address\n");
	abort();
    } 
    fprintf(stderr, "# laucher: local url = '%s'\n", local_launcher_url);
  
}

void __bmi_connect(BMI_addr_t dest, char* url)
{
      /* client */
      int sock = socket(AF_INET, SOCK_STREAM, 0);
      assert(sock > -1);
      assert(strlen(url) == 12);
      in_addr_t peer_addr;
      int peer_port;
      sscanf(url, "%08x%04x", &peer_addr, &peer_port);
      struct sockaddr_in inaddr = {
	.sin_family = AF_INET,
	.sin_port   = peer_port,
	.sin_addr   = (struct in_addr){ .s_addr = ntohl(peer_addr) }
      };
#ifdef DEBUG
      fprintf(stderr, "connecting to %s\n", url);
#endif
      int rc = connect(sock, (struct sockaddr*)&inaddr, sizeof(struct sockaddr_in));
      if(rc)
	{
	  fprintf(stderr, "# launcher: cannot connect to %s:%d\n", inet_ntoa(inaddr.sin_addr), peer_port);
	  abort();
	}
#ifdef DEBUG
      fprintf(stderr, "\tExchanging session url\n");
#endif
      char * remote_session_url;
      __bmi_launcher_addr_recv(sock, &remote_session_url);
#ifdef DEBUG
      fprintf(stderr, "\t\tI have just received %s\n",remote_session_url);
#endif
      __bmi_launcher_addr_send(sock, local_session_url);
#ifdef DEBUG
      fprintf(stderr, "\t\tI have just sent %s\n",local_session_url);
#endif
      close(sock);
      __bmi_connect_accept(dest, remote_session_url);
#ifdef DEBUG
      fprintf(stderr, "Connection OK ! Ready to send/recv messages!\n");
#endif
}

/** Initializes the BMI layer.  Must be called before any other BMI
 *  functions.
 *
 *  \param method_list a comma separated list of BMI methods to
 *         use
 *  \param listen_addr a comma separated list of addresses to listen on
 *         for each method (if needed)
 *  \param flags initialization flags
 *
 *  \return 0 on success, -errno on failure
 */
int 
BMI_initialize(const char *method_list,
	       const char *listen_addr,
    	             int   flags){

#ifdef MARCEL
    marcel_mutex_lock(&bmi_initialize_mutex);
#endif
    /* Already initialized? Just increment ref count and return. */
    ++bmi_initialized_count;
#ifdef MARCEL
    marcel_mutex_unlock(&bmi_initialize_mutex);
#endif

    if(bmi_initialized_count > 1)return 0;

    char *dummy_argv[1] = {NULL};
    int   dummy_argc    = 1;

    /* initialise le core de nmad */
    const char *label="bmi";
    int err = nm_session_create(&p_core, label);
    if (err != NM_ESUCCESS)
    {
	fprintf(stderr, "# launcher: nm_session_create returned err = %d\n", err);
	abort();
    }
    err = nm_session_init(p_core, &dummy_argc,dummy_argv, &local_session_url);
    if (err != NM_ESUCCESS)
    {
	fprintf(stderr, "# launcher: nm_session_init returned err = %d\n", err);
	abort();
    }

    nm_sr_init(p_core);

    /* make a new reference list */
    cur_ref_list = ref_list_new();

    if (!cur_ref_list){
	fprintf(stderr, "ref_list initialization failed \n");
    	return -1;
    }

    /* initialize the request allocator */
    tbx_malloc_extended_init(&nm_bmi_mem, sizeof(struct bmi_op_id), NM_BMI_REQUEST_PREALLOC, "nmad/bmi/request", 1);
    /* If we are a server, open an endpoint now.*/    
    if (flags & BMI_INIT_SERVER) {/*SERVER*/
	server = 1;
    } 
    
    __bmi_iaccept();

    return 0;
}


/** Shuts down the BMI layer.
 *
 * \return 0.
 */
int 
BMI_finalize(void){
#ifdef MARCEL
    marcel_mutex_lock(&bmi_initialize_mutex);
#endif
    --bmi_initialized_count;
    if(bmi_initialized_count > 0){
#ifdef MARCEL
        marcel_mutex_unlock(&bmi_initialize_mutex);
#endif
        return 0;
    }
#ifdef MARCEL
    marcel_mutex_unlock(&bmi_initialize_mutex);
    marcel_mutex_lock(&active_method_count_mutex);
#endif

    /* destroy list */
    ref_list_cleanup(cur_ref_list);
 
    nm_session_destroy(p_core);

    common_exit(NULL);
    return 0;
}


/** Creates a new context to be used for communication.  This can be
 *  used, for example, to distinguish between operations posted by
 *  different threads.
 *
 *  \return 0 on success, -errno on failure.
 */
int 
BMI_open_context(bmi_context_id* context_id){
    int context_index;
    int ret = 0;

#ifdef MARCEL
    marcel_mutex_lock(&context_mutex);
#endif

    /* find an unused context id */
    for(context_index=0;context_index<BMI_MAX_CONTEXTS;context_index++){
	if(context_array[context_index] == 0){
	    break;
	}
    }

    if(context_index >= BMI_MAX_CONTEXTS){
	/* we don't have any more available! */
	context_id = NULL;
#ifdef MARCEL
	marcel_mutex_unlock(&context_mutex);
#endif
	return 1;
    }
    context_array[context_index] = 1;

    *context_id = TBX_MALLOC(sizeof(struct bmi_context));
    
#ifdef MARCEL
    marcel_mutex_unlock(&context_mutex);
#endif

    return ret;
}

/** Allocates memory that can be used in native mode by the BMI layer.
 *
 *  \return Pointer to buffer on success, NULL on failure.
 */
void*
BMI_memalloc(BMI_addr_t addr,
	     bmi_size_t size,
	     enum bmi_op_type send_recv){

    return malloc(size);
}

/** Frees memory that was allocated with BMI_memalloc().
 *
 *  \return 0 on success, -errno on failure.
 */
int 
BMI_memfree(BMI_addr_t addr,
	    void *buffer,
	    bmi_size_t size,
	    enum bmi_op_type send_recv){
    free(buffer);
    return 0;
}

/** Acknowledge that an unexpected message has been
 * serviced that was returned from BMI_test_unexpected().
 *
 *  \return 0 on success, -errno on failure.
 */
int 
BMI_unexpected_free(BMI_addr_t addr,
		    void *buffer){
    return 0;
}


/** Destroys a context previous generated with BMI_open_context().
 */
void 
BMI_close_context(bmi_context_id context_id){
    /* todo: do something here :) */
    return;
}

/** Resolves the string representation of a host address into a BMI
 *  address handle.
 *
 *  \return 0 on success, -errno on failure.
 */
int 
BMI_addr_lookup(BMI_addr_t *peer,
		const char *id_string){
    int ret = 0;
    struct bnm_peer* new_peer= NULL;

    /* First we want to check to see if this host has already been
     * discovered! */
  
    new_peer = ref_list_search_str(cur_ref_list, id_string);

    if (!new_peer){

	/* create a new reference for the addr */
	new_peer= TBX_MALLOC(sizeof( struct bnm_peer));

	if (!new_peer)  {
	    goto bmi_addr_lookup_failure;
	}
	
	new_peer->peername = (char *)TBX_MALLOC(strlen(id_string) + 1);

	if (!new_peer->peername)
	    goto bmi_addr_lookup_failure;

	strcpy((char*)new_peer->peername, id_string);

	new_peer->p_gate = NULL;
	assert(p_core);
	assert(new_peer->peername);
	
	/* keep up with the reference and we are done */
#ifdef MARCEL
	marcel_mutex_lock(&ref_mutex);
#endif

	ref_list_add(cur_ref_list, new_peer);
	
#ifdef MARCEL
	marcel_mutex_unlock(&ref_mutex);
#endif
	
	peer[0]           = TBX_MALLOC(sizeof(struct BMI_addr));

	peer[0]->p_gate = new_peer->p_gate;

	peer[0]->peername = TBX_MALLOC(strlen(id_string)+1);
	memmove(peer[0]->peername,new_peer->peername,strlen(id_string)+1);

    }

#ifdef DEBUG
    fprintf(stderr, "peer %p, *peer %p\n", peer, *peer);
#endif
    assert(*peer);
    
    TBX_FREE((char*)new_peer->peername);
    TBX_FREE(new_peer);

    return ret;

  bmi_addr_lookup_failure:

    if (new_peer){
	TBX_FREE((char*)new_peer->peername);
	TBX_FREE(new_peer);
    }
    return (ret);
}


/** Submits receive operations for subsequent service.
 *
 *  \return 0 on success, -errno on failure.
 */

int 
BMI_post_recv(bmi_op_id_t         *id,
	      BMI_addr_t           src,
	      void                *buffer,
	      bmi_size_t           expected_size,
	      bmi_size_t          *actual_size,//not used
	      enum bmi_buffer_type buffer_type,//not used
	      bmi_msg_tag_t        tag,
	      void                *user_ptr,   //not used
	      bmi_context_id       context_id,
	      bmi_hint             hints){     //not used
    
    int            ret = -1;
    struct bnm_ctx rx;
#ifdef DEBUG
    fprintf(stderr, "BMI_post_recv(src %p, size %d, tag %lx)\n", src, expected_size, tag);
#endif
    /* yes i know, a malloc on the critical path is quite dangerous, but we can't avoid this */
    *id = tbx_malloc(nm_bmi_mem);
    (*id)->context_id = context_id;
    (*id)->status = RECV;
    
    context_id->status = RECV;

#if 0
    /* todo: fix this stuff */
    if(src->p_gate->status == NM_GATE_STATUS_INIT) {
	ret = nm_core_gate_accept(p_core, src->p_gate, src->p_drv, NULL);
    }
#endif

    rx.nmc_state    = BNM_CTX_PREP;
    rx.nmc_tag      = tag;
    rx.nmc_msg_type = BNM_MSG_EXPECTED;
    rx.nmc_type     = BNM_REQ_RX;
    rx.nmc_peer     = src->p_peer;

    bnm_create_match(&rx);

    ret = nm_sr_irecv(p_core, src->p_peer->p_gate, rx.nmc_match, 
		      buffer, expected_size, 
		      &(*id)->request);
    
#ifdef DEBUG
    fprintf(stderr , "\t\tRECV request  %p\n",&(*id)->request );
#endif

    return ret;
}

int 
BMI_post_send_generic(bmi_op_id_t * id,                //not used
		      BMI_addr_t dest,                 //must be used
		      const void *buffer,
		      bmi_size_t size,
		      enum bmi_buffer_type buffer_type,//not used
		      bmi_msg_tag_t tag,
		      void *user_ptr,                  //not used
		      bmi_context_id context_id,
		      bmi_hint hints,
		      enum bnm_msg_type msg_type) {                 //not used
    
    int                     ret    = -1;   
    struct bnm_ctx          tx;
    /* yes i know, a malloc on the critical path is quite dangerous, but we can't avoid this */
    *id = tbx_malloc(nm_bmi_mem);
    (*id)->context_id = context_id;
    (*id)->status = SEND;

    context_id->status = SEND;

    if((!dest->p_gate ) || (dest->p_gate->status == NM_GATE_STATUS_INIT)) {
	/* remove nm:// from the peername so that NMad establish the connection */
	char* tmp = dest->peername + 5*sizeof(char);  
	__bmi_connect(dest, tmp);
    }

    tx.nmc_state    = BNM_CTX_PREP;
    tx.nmc_type     = BNM_REQ_TX;
    tx.nmc_msg_type = msg_type;
    tx.nmc_peer     = dest->p_peer;


    if(msg_type == BNM_MSG_UNEXPECTED) {
	nm_sr_request_t size_req;
	tx.nmc_tag      = 0;
	bnm_create_match(&tx);

	nm_sr_isend(p_core, dest->p_gate, 
		    tx.nmc_match, &size, sizeof(size), 
		    &size_req);
	nm_sr_swait(p_core, &size_req);
	nm_sr_isend(p_core, dest->p_gate, 
		    tx.nmc_match, &tag, sizeof(tag), 
		    &size_req);
	nm_sr_swait(p_core, &size_req);

    }

    tx.nmc_tag      = tag;
    bnm_create_match(&tx);

    ret = nm_sr_isend(p_core, dest->p_gate, 
		      tx.nmc_match, buffer, size, 
		      &(*id)->request);
#ifdef DEBUG
    fprintf(stderr , "\t\tSEND request  %p\n",&(*id)->request );
#endif
    return ret;
}

/** Submits send operations for subsequent service.
 *
 *  \return 0 on success, -errno on failure.
 */
int 
BMI_post_send(bmi_op_id_t * id,                //not used
	      BMI_addr_t dest,                 //must be used
	      const void *buffer,
	      bmi_size_t size,
	      enum bmi_buffer_type buffer_type,//not used
	      bmi_msg_tag_t tag,
	      void *user_ptr,                  //not used
	      bmi_context_id context_id,
	      bmi_hint hints){                 //not used
#ifdef DEBUG
    fprintf(stderr, "BMI_post_send(dest %p, size %d, tag %lx)\n", dest, size, tag);
#endif

    return BMI_post_send_generic(id, dest, buffer, size, buffer_type, tag, user_ptr, context_id, hints, BNM_MSG_EXPECTED);
}
/** Submits unexpected send operations for subsequent service.
 *
 *  \return 0 on success, -errno on failure.
 */
int 
BMI_post_sendunexpected(bmi_op_id_t * id,
			BMI_addr_t dest,
			const void *buffer,
			bmi_size_t size,
			enum bmi_buffer_type buffer_type,
			bmi_msg_tag_t tag,
			void *user_ptr,
			bmi_context_id context_id,
			bmi_hint hints){
#ifdef DEBUG
    fprintf(stderr, "BMI_post_send_unexpected(dest %p, size %d, tag %lx)\n", dest, size, tag);
#endif
    return BMI_post_send_generic(id, dest, buffer, size, buffer_type, tag, user_ptr, context_id, hints, BNM_MSG_UNEXPECTED);
}

/** Checks to see if any unexpected messages have completed.
 *
 *  \return 0 on success, -errno on failure.
 */
/* todo: for now, this function is blocking since it waits for an incoming connecting */
int BMI_testunexpected(int incount,
		       int *outcount,
		       struct BMI_unexpected_info *info_array,
		       int max_idle_time_ms)
{
    info_array->addr = malloc(sizeof(struct BMI_addr));
    int ret =__bmi_test_accept(info_array->addr);
    if(ret == 1) 
	{
	    nm_sr_request_t request;
	    
	    struct bnm_ctx rx;
	    rx.nmc_state    = BNM_CTX_PREP;
	    rx.nmc_tag      = 0;
	    rx.nmc_type     = BNM_REQ_RX;
	    rx.nmc_msg_type = BNM_MSG_UNEXPECTED;
	    rx.nmc_peer     = info_array->addr->p_peer;
	    
	    bnm_create_match(&rx);
	    
	    /* retrieve remote tx_id */
	    nm_sr_irecv(p_core, info_array->addr->p_gate, rx.nmc_match, 
			&info_array->size, sizeof(info_array->size), &request);
	    nm_sr_rwait(p_core, &request);
	    nm_sr_irecv(p_core, info_array->addr->p_gate, rx.nmc_match, 
			&rx.nmc_tag, sizeof(rx.nmc_tag), &request);
	    nm_sr_rwait(p_core, &request);
	    
	    /* todo: solve the tag problem here : for now, the tag must be 0 */	    
	    info_array->buffer = malloc(info_array->size);
	    bnm_create_match(&rx);
	    nm_sr_irecv(p_core, info_array->addr->p_gate, rx.nmc_match, 
			info_array->buffer, info_array->size, &request);
	    nm_sr_rwait(p_core, &request);

	    info_array->size = nm_sr_get_size(p_core, &request, (size_t*)outcount);
	    info_array->tag = rx.nmc_tag;
	    info_array->error_code=0;
	    if(outcount){
		(*outcount)++;
	    }
	} else {
	free(info_array->addr);
    }
	return 0;
	
}


/** Checks to see if a particular message has completed.
 *
 *  \return 0 on success, -errno on failure.
 */
int 
BMI_test(bmi_op_id_t id,                
	 int *outcount,                 
	 bmi_error_code_t * error_code, //unused
	 bmi_size_t * actual_size,      
	 void **user_ptr,               //unused
	 int max_idle_time_ms,          //unused with the present implementation
	 bmi_context_id context_id){
    int ret = -1;
    if(outcount)
	*outcount=0;

    if(actual_size)
	*actual_size=0;
    if(id->status == SEND ) {
	ret = nm_sr_stest(p_core, &id->request);
	if(ret == -NM_ESUCCESS && actual_size)
	    *actual_size = id->request.req.pack.len;
    } else { //RECV
	ret = nm_sr_rtest(p_core, &id->request);
	if(ret == -NM_ESUCCESS && actual_size) {
	    nm_sr_get_size(p_core, &id->request, (size_t*)actual_size);
	}
    }

    if(ret == -NM_EAGAIN)
	ret = 0;
    else if(ret == -NM_ESUCCESS) {
	if(outcount)
	    *outcount = 1;
#ifdef DEBUG
	if(outcount && (*outcount)){
	    fprintf(stderr, "BMI_test succeeds\n");
	    fprintf(stderr, "\t outcount = %d\n", *outcount);
	}
#endif
	ret = 0;
	tbx_free(nm_bmi_mem, id);
    }
    return (ret);
}

/** Query for optional parameters.
 *
 *  \return 0 on success, -errno on failure.
 */
int 
BMI_get_info(BMI_addr_t addr,
	     int option,
	     void *inout_parameter){

    switch (option)
    {
	/* check to see if the interface is initialized */
    case BMI_CHECK_INIT:
	return bmi_initialized_count;
    case BMI_CHECK_MAXSIZE:
	*((int *) inout_parameter) = (1U << 31) - 1;
	break;
    case BMI_GET_METH_ADDR:
	/* todo: return p_core ? */
	*((void **) inout_parameter) = NULL;
	break;
    case BMI_GET_UNEXP_SIZE:
	*((int *) inout_parameter) = NM_SO_MAX_UNEXPECTED;
        break;
    default:
	return 1;
    }
    return (0);
}


/** Checks to see if any messages from the specified list have completed.
 *
 * \return 0 on success, -errno on failure.
 *
 * XXX: never used.  May want to add adaptive polling strategy of testcontext
 * if it becomes used again.
 */
int 
BMI_testsome(int incount,
	     bmi_op_id_t * id_array,
	     int *outcount,
	     int *index_array,
	     bmi_error_code_t * error_code_array,
	     bmi_size_t * actual_size_array,
	     void **user_ptr_array,
	     int max_idle_time_ms,
	     bmi_context_id context_id){

        fprintf(stderr, "BMI_testsome: not yet implemented!\n");
	abort();
	return(0);
}


/** Checks to see if any messages from the specified context have
 *  completed.
 *
 *  \returns 0 on success, -errno on failure.
 */
int 
BMI_testcontext(int incount,
		bmi_op_id_t* out_id_array,
		int *outcount,
		bmi_error_code_t * error_code_array,
		bmi_size_t * actual_size_array,
		void **user_ptr_array,
		int max_idle_time_ms,
		bmi_context_id context_id){

        fprintf(stderr, "BMI_testcontext: not yet implemented!\n");
	abort();
	return 0;
}

/** Performs a reverse lookup, returning the string (URL style)
 *  address for a given opaque address.
 *
 *  NOTE: caller must not free or modify returned string
 *
 *  \return Pointer to string on success, NULL on failure.
 */
const char* 
BMI_addr_rev_lookup(BMI_addr_t addr){
        fprintf(stderr, "BMI_addr_rev_lookup: not yet implemented!\n");
	abort();
	return NULL;
}

/** Performs a reverse lookup, returning a string
 *  address for a given opaque address.  Works on any address, even those
 *  generated unexpectedly, but only gives hostname instead of full
 *  BMI URL style address
 *
 *  NOTE: caller must not free or modify returned string
 *
 *  \return Pointer to string on success, NULL on failure.
 */
const char* 
BMI_addr_rev_lookup_unexpected(BMI_addr_t addr){
        fprintf(stderr, "BMI_addr_rev_lookup_unexpected: not yet implemented!\n");
	abort();
	return NULL;
}

/** Pass in optional parameters.
 *
 *  \return 0 on success, -errno on failure.
 */
int 
BMI_set_info(BMI_addr_t addr,
	     int option,
	     void *inout_parameter){
    fprintf(stderr, "Warning : trying to set option %d ! Setting options is not yet implemented !\n", option);
    return 0;
}


/** Given a string representation of a host/network address and a BMI
 * address handle, return whether the BMI address handle is part of the wildcard
 * address range specified by the string.
 * \return 1 on success, -errno on failure and 0 if it is not part of
 * the specified range
 */
int 
BMI_query_addr_range (BMI_addr_t addr, 
		      const char *id_string, 
		      int netmask){
        fprintf(stderr, "BMI_query_addr_range: not yet implemented!\n");
	abort();
	return 0;
}



/** Similar to BMI_post_send(), except that the source buffer is 
 *  replaced by a list of (possibly non contiguous) buffers.
 *
 *  \return 0 on success, 1 on immediate successful completion,
 *  -errno on failure.
 */
int 
BMI_post_send_list(bmi_op_id_t * id,
		   BMI_addr_t dest,
		   const void *const *buffer_list,
		   const bmi_size_t *size_list,
		   int list_count,
		   /* "total_size" is the sum of the size list */
		   bmi_size_t total_size,
		   enum bmi_buffer_type buffer_type,
		   bmi_msg_tag_t tag,
		   void *user_ptr,
		   bmi_context_id context_id,
		   bmi_hint hints){
    fprintf(stderr, "BMI_post_send_list: not yet implemented!\n");
    abort();
    return 0;
}

/** Similar to BMI_post_recv(), except that the dest buffer is 
 *  replaced by a list of (possibly non contiguous) buffers
 *
 *  \param total_expected_size the sum of the size list.
 *  \param total_actual_size the aggregate amt that was received.
 *
 *  \return 0 on success, 1 on immediate successful completion,
 *  -errno on failure.
 */
int 
BMI_post_recv_list(bmi_op_id_t * id,
		   BMI_addr_t src,
		   void *const *buffer_list,
		   const bmi_size_t *size_list,
		   int list_count,
		   bmi_size_t total_expected_size,
		   bmi_size_t * total_actual_size,
		   enum bmi_buffer_type buffer_type,
		   bmi_msg_tag_t tag,
		   void *user_ptr,
		   bmi_context_id context_id,
		   bmi_hint hints){

    fprintf(stderr, "BMI_post_recv_list: not yet implemented!\n");
    abort();
    return 0;
}


/** Similar to BMI_post_sendunexpected(), except that the source buffer is 
 *  replaced by a list of (possibly non contiguous) buffers.
 *
 *  \param total_size the sum of the size list.
 *
 *  \return 0 on success, 1 on immediate successful completion,
 *  -errno on failure.
 */
int 
BMI_post_sendunexpected_list(bmi_op_id_t * id,
			     BMI_addr_t dest,
			     const void *const *buffer_list,
			     const bmi_size_t *size_list,
			     int list_count,
			     bmi_size_t total_size,
			     enum bmi_buffer_type buffer_type,
			     bmi_msg_tag_t tag,
			     void *user_ptr,
			     bmi_context_id context_id,
			     bmi_hint hints){

    fprintf(stderr, "BMI_post_sendunexpected_list: not yet implemented!\n");
    abort();
    return 0;
}

/** Attempts to cancel a pending operation that has not yet completed.
 *  Caller must still test to gather error code after calling this
 *  function even if it returns 0.
 *
 *  \return 0 on success, -errno on failure.
 */
int 
BMI_cancel(bmi_op_id_t id, 
	   bmi_context_id context_id){

    fprintf(stderr, "BMI_cancel: not yet implemented!\n");
    abort();
    return 0;
}


/**************************************************************
 * method callback functions
 */

/* bmi_method_addr_reg_callback()
 * 
 * Used by the methods to register new addresses when they are
 * discovered.  Only call this method when the device gets an
 * unexpected receive from a new peer, i.e., if you do the equivalent
 * of a socket accept() and get a new connection.
 *
 * Do not call this function for active lookups, that is from your
 * method_addr_lookup.  BMI already knows about the address in
 * this case, since the user provided it.
 *
 * returns 0 on success, -errno on failure
 */
BMI_addr_t 
bmi_method_addr_reg_callback(bmi_method_addr_p map){

    fprintf(stderr, "bmi_method_addr_reg_callback: not yet implemented!\n");
    abort();
    return NULL;
}

int 
bmi_method_addr_forget_callback(BMI_addr_t addr){

    fprintf(stderr, "bmi_method_addr_forget_callback: not yet implemented!\n");
    abort();
    return 0;
}


#ifdef PVFS
 
int 
bmi_errno_to_pvfs(int error){
    int bmi_errno = error;

#define __CASE(err)                      \
case -err: bmi_errno = -BMI_##err; break;\
case err: bmi_errno = BMI_##err; break

    switch(error)
    {
        __CASE(EPERM);
        __CASE(ENOENT);
        __CASE(EINTR);
        __CASE(EIO);
        __CASE(ENXIO);
        __CASE(EBADF);
        __CASE(EAGAIN);
        __CASE(ENOMEM);
        __CASE(EFAULT);
        __CASE(EBUSY);
        __CASE(EEXIST);
        __CASE(ENODEV);
        __CASE(ENOTDIR);
        __CASE(EISDIR);
        __CASE(EINVAL);
        __CASE(EMFILE);
        __CASE(EFBIG);
        __CASE(ENOSPC);
        __CASE(EROFS);
        __CASE(EMLINK);
        __CASE(EPIPE);
        __CASE(EDEADLK);
        __CASE(ENAMETOOLONG);
        __CASE(ENOLCK);
        __CASE(ENOSYS);
        __CASE(ENOTEMPTY);
        __CASE(ELOOP);
        __CASE(ENOMSG);
        __CASE(ENODATA);
        __CASE(ETIME);
        __CASE(EREMOTE);
        __CASE(EPROTO);
        __CASE(EBADMSG);
        __CASE(EOVERFLOW);
        __CASE(EMSGSIZE);
        __CASE(EPROTOTYPE);
        __CASE(ENOPROTOOPT);
        __CASE(EPROTONOSUPPORT);
        __CASE(EOPNOTSUPP);
        __CASE(EADDRINUSE);
        __CASE(EADDRNOTAVAIL);
        __CASE(ENETDOWN);
        __CASE(ENETUNREACH);
        __CASE(ENETRESET);
        __CASE(ENOBUFS);
        __CASE(ETIMEDOUT);
        __CASE(ECONNREFUSED);
        __CASE(EHOSTDOWN);
        __CASE(EHOSTUNREACH);
        __CASE(EALREADY);
        __CASE(EACCES);
        __CASE(ECONNRESET);
#undef __CASE
    }
    return bmi_errno;
}
#endif

#endif /* NM_TAGS_AS_INDIRECT_HASH */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 expandtab
 */
