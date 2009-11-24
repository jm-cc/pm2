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

#include <errno.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <inttypes.h>

#include "bmi.h"
#include "bmi-types.h"
//#include "bmi-method-support.h"
//#include "bmi-method-callback.h"
#include "reference-list.h"
#include "op-list.h"
#include "str-utils.h"

#include "nmad_bmi_interface.h"

#include <nm_public.h>
#include <nm_sendrecv_interface.h>

//#define DEBUG 1

#define SERVER 0

static int bmi_initialized_count = 0;

/*
 * List of BMI addrs currently managed.
 */
static ref_list_p cur_ref_list = NULL;

/* array to keep up with active contexts */
static int context_array[BMI_MAX_CONTEXTS] = { 0 };

#ifdef MARCEL
static marcel_mutex_t bmi_initialize_mutex = MARCEL_MUTEX_INITIALIZER;
static marcel_mutex_t context_mutex = MARCEL_MUTEX_INITIALIZER;
static marcel_mutex_t ref_mutex = MARCEL_MUTEX_INITIALIZER;
static marcel_mutex_t forget_list_mutex = MARCEL_MUTEX_INITIALIZER;
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
static nm_core_t p_core;

static nm_drv_t p_drv;

static puk_component_t  p_driver_load;


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

static int __bmi_connect_accept(struct BMI_addr* addr)
{
    
    addr->p_peer=malloc(sizeof(struct bnm_peer));

    __bmi_peer_init(addr->p_peer);
    addr->p_peer->p_gate = addr->p_gate;
    addr->p_peer->p_drv = p_drv;
    addr->p_peer->nmp_state = BNM_PEER_READY;
    addr->p_peer->nmp_rx_id = max_tx_id++;
    addr->p_peer->peername = addr->peername;
#ifdef MARCEL
    marcel_mutex_init (addr->p_peer->mutex, NULL);
#endif	/* MARCEL */
    
    /* now that the peers are connected, we have to exchange :
     * - the local hostname
     * - the tx_id 
     */
    nm_sr_request_t rrequest1, rrequest2, srequest1, srequest2;
    struct bnm_ctx rx;
    rx.nmc_state    = BNM_CTX_PREP;
    rx.nmc_tag      = 42;
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

    ref_list_add(cur_ref_list, addr->p_peer);    

    return 0;
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

    int   ret           = -1;
    char *dummy_argv[1] = {NULL};
    int   dummy_argc    = 1;

    /* initialise le core de nmad */
    ret = nm_core_init(&dummy_argc,dummy_argv, &p_core);
    if (ret != NM_ESUCCESS){
        fprintf(stderr,"nm_core_init returned err = %d\n", ret);
	goto failed;
    }

    /* charge le driver reseau selectionne par nmad-driver-conf 
     * attention, le driver n'est pas encore pret a etre utilise !
     */
    p_driver_load = nm_core_component_load("driver", "custom");

    /* initialise l'interface sendrecv */
    ret = nm_sr_init(p_core);
    if(ret != NM_ESUCCESS) {
	fprintf(stderr,"nm_so_pack_interface_init return err = %d\n",ret);
	goto failed;
    }

    char url[256];

    /* lance le driver selectionne*/
    ret = nm_core_driver_load_init(p_core, p_driver_load, 
				   &p_drv, (char**)&url);
    
    if (ret != NM_ESUCCESS) {
	fprintf(stderr,"nm_core_driver_init(some) returned ret = %d\n",ret);
	goto failed;
    }
    
    nm_ns_update(p_core);

    /* make a new reference list */
    cur_ref_list = ref_list_new();

    if (!cur_ref_list){
	fprintf(stderr, "ref_list initialization failed \n");
    	return -1;
    }

    /* If we are a server, open an endpoint now.*/    
    if (flags & BMI_INIT_SERVER) {/*SERVER*/
	server = 1;
    } 
    return 0;

 failed:

    if (cur_ref_list)
	ref_list_cleanup(cur_ref_list);

    return -1;
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
 
    nm_core_exit(p_core);

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
#if 0
#ifdef MARCEL
    marcel_mutex_lock(&context_mutex);
#endif

    if(!context_array[context_id->context_id]){
#ifdef MARCEL
	marcel_mutex_unlock(&context_mutex);
#endif
	return;
    }
    /* tell all of the modules to get rid of this context */
    context_array[context_id->context_id] = 0;

#ifdef MARCEL
    marcel_mutex_unlock(&context_mutex);
#endif
#endif	/* 0 */
    TBX_FREE(context_id);
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
	
	new_peer->p_drv = p_drv;
	new_peer->peername = (char *)TBX_MALLOC(strlen(id_string) + 1);

	if (!new_peer->peername)
	    goto bmi_addr_lookup_failure;

	strcpy((char*)new_peer->peername, id_string);

	ret = nm_core_gate_init(p_core, &new_peer->p_gate);

	if (ret != NM_ESUCCESS) {
	    fprintf(stderr,"nm_core_gate_init returned ret = %d\n",ret);
	    goto bmi_addr_lookup_failure;
	}

	assert(p_core);
	assert(new_peer->p_gate);
	assert(new_peer->peername);

#ifdef DEBUG	
	fprintf(stderr, "gate %p\n", new_peer->p_gate);
#endif

	if (ret != NM_ESUCCESS) {
	    fprintf(stderr,"nm_core_gate_connect returned ret = %d\n",ret);	    
	    exit(1);
	}
	
	/* keep up with the reference and we are done */
#ifdef MARCEL
	marcel_mutex_lock(&ref_mutex);
#endif

	ref_list_add(cur_ref_list, new_peer);
	
#ifdef MARCEL
	marcel_mutex_unlock(&ref_mutex);
#endif
	
	peer[0]           = TBX_MALLOC(sizeof(struct BMI_addr));
	peer[0]->p_drv    = p_drv;

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
BMI_post_recv(bmi_op_id_t         *id,         //not used
	      BMI_addr_t           src,        
	      void                *buffer,
	      bmi_size_t           expected_size,
	      bmi_size_t          *actual_size,//not used
	      enum bmi_buffer_type buffer_type,//not used
	      bmi_msg_tag_t        tag,
	      void                *user_ptr,   //not used
	      bmi_context_id       context_id,
	      bmi_hint             hints){     //not used
    
    int ret = -1;

    struct bnm_ctx          rx;
    
    context_id->status = RECV;

    if(src->p_gate->status == NM_GATE_STATUS_INIT) {
	ret = nm_core_gate_accept(p_core, src->p_gate, src->p_drv, NULL);
    }

    rx.nmc_state    = BNM_CTX_PREP;
    rx.nmc_tag      = tag;
    rx.nmc_msg_type = BNM_MSG_EXPECTED;
    rx.nmc_type     = BNM_REQ_RX;
    rx.nmc_peer     = src->p_peer;

    bnm_create_match(&rx);

    ret = nm_sr_irecv(p_core, src->p_peer->p_gate, rx.nmc_match, 
		      buffer, expected_size, 
		      &context_id->request);
    
#ifdef DEBUG
    fprintf(stderr , "\t\tRECV request  %p\n",&context_id->request );
#endif

    return ret;
}


static int 
BMI_post_send_generic(bmi_op_id_t * id,                //not used
		      BMI_addr_t dest,                 //must be used
		      const void *buffer,
		      bmi_size_t size,
		      enum bmi_buffer_type buffer_type,//not used
		      bmi_msg_tag_t tag,
		      void *user_ptr,                  //not used
		      bmi_context_id context_id,
		      bmi_hint hints, 
		      enum bnm_msg_type msg_type){                 //not used
    
    int                      ret    = -1;   
    struct bnm_ctx          tx;
    context_id->status = SEND;

    if(dest->p_gate->status == NM_GATE_STATUS_INIT) {
    
	ret = nm_core_gate_connect(p_core, dest->p_gate, dest->p_drv, dest->peername);
	__bmi_connect_accept(dest);
    }

    /* get idle tx, if available, otherwise alloc one */
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

    }

    tx.nmc_tag      = tag;
    bnm_create_match(&tx);

    ret = nm_sr_isend(p_core, dest->p_gate, 
		      tx.nmc_match, buffer, size, 
		      &context_id->request);
#ifdef DEBUG
    fprintf(stderr , "\t\tSEND request  %p\n",&context_id->request );
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
    
    /* todo: run this asynchronously ! (progression thread ?) */
    info_array->addr = malloc(sizeof(struct BMI_addr));
    nm_core_gate_init(p_core, &info_array->addr->p_gate);    

    int err = nm_core_gate_accept(p_core, info_array->addr->p_gate, p_drv, NULL);
    if(err != NM_ESUCCESS)
	return 0;
    __bmi_connect_accept(info_array->addr);

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

    /* todo: solve the tag problem here : for now, the tag must be 0 */


    info_array->buffer = malloc(info_array->size);
    nm_sr_irecv(p_core, info_array->addr->p_gate, rx.nmc_match, 
		info_array->buffer, info_array->size, &request);
    nm_sr_rwait(p_core, &request);
 
    info_array->error_code=0;
    return 1;
}


/** Checks to see if a particular message has completed.
 *
 *  \return 0 on success, -errno on failure.
 */
int 
BMI_test(bmi_op_id_t id,                //unused
	 int *outcount,                 //unused
	 bmi_error_code_t * error_code, //unused
	 bmi_size_t * actual_size,      //unused
	 void **user_ptr,               //unused
	 int max_idle_time_ms,          //unused with the present implementation
	 bmi_context_id context_id){
    int ret = -1;

    if(context_id->status == SEND )
	ret = nm_sr_stest(p_core, &context_id->request);
    else //RECV
	ret = nm_sr_rtest(p_core, &context_id->request);

    if(ret == NM_ESUCCESS && outcount)
	*outcount = context_id->request.req.pack.len;
    if(ret == NM_ESUCCESS && actual_size)
	*actual_size = context_id->request.req.pack.len;
	
    return (ret);
}


/** Checks to see if any messages from the specified list have completed.
 *
 * \return 0 on success, -errno on failure.
 *
 * XXX: never used.  May want to add adaptive polling strategy of testcontext
 * if it becomes used again.
 */
#if 0
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
    int ret = 0;
    int idle_per_method = 0;
    bmi_op_id_t* tmp_id_array;
    int i,j;
    struct method_op *query_op;
    int need_to_test;
    int tmp_outcount;
    int tmp_active_method_count;

#ifdef MARCEL
    marcel_mutex_lock(&active_method_count_mutex);
    tmp_active_method_count = active_method_count;
    marcel_mutex_unlock(&active_method_count_mutex);
#else
    tmp_active_method_count = active_method_count;
#endif
    *outcount = 0;

    if (tmp_active_method_count == 1) {
	/* shortcircuit for perhaps common case of only one method */
	ret = active_method_table[0]->testsome(
	    incount, id_array, outcount, index_array,
	    error_code_array, actual_size_array, user_ptr_array,
	    max_idle_time_ms, context_id->context_id);

	/* return 1 if anything completed */
	if (ret == 0 && *outcount > 0)
	    return (1);
	else
	    return ret;
    }

    /* TODO: do something more clever here */
    if (max_idle_time_ms)
    {
	idle_per_method = max_idle_time_ms / tmp_active_method_count;
	if (!idle_per_method)
	    idle_per_method = 1;
    }

    tmp_id_array = (bmi_op_id_t*)TBX_MALLOC(incount*sizeof(bmi_op_id_t));
    /* iterate over each active method */
    for(i=0; i<tmp_active_method_count; i++)
    {
	/* setup the tmp id array with only operations that match
	 * that method
	 */
	need_to_test = 0;
	for(j=0; j<incount; j++)
	{
	    if(id_array[j])
	    {
		/* todo: get rid of this */
		//query_op = (struct method_op*)
                //    id_gen_fast_lookup(id_array[j]);
		assert(query_op->op_id == id_array[j]);
		if(query_op->addr->method_type == i)
		{
		    tmp_id_array[j] = id_array[j];
		    need_to_test++;
		}
	    }
	}

	/* call testsome if we found any ops for this method */
	if(need_to_test)
	{
	    tmp_outcount = 0;
	    ret = active_method_table[i]->testsome(
		need_to_test, tmp_id_array, &tmp_outcount, 
		&(index_array[*outcount]),
		&(error_code_array[*outcount]),
		&(actual_size_array[*outcount]),
		user_ptr_array ? &(user_ptr_array[*outcount]) : 0,
		idle_per_method,
		context_id);
	    if(ret < 0)
	    {
		/* can't recover from this... */
		//gossip_lerr("Error: critical BMI_testsome failure.\n");
		goto out;
	    }
	    *outcount += tmp_outcount;
	}
    }

  out:
    TBX_FREE(tmp_id_array);

    if(ret == 0 && *outcount > 0)
	return(1);
    else
	return(0);
}
#endif

/*
 * If some method was recently active, poll it again for speed,
 * but be sure not to starve any method.  If multiple active,
 * poll them all.  Return idle_time per method too.
 */
#if 0
static void
construct_poll_plan(int nmeth, int *idle_time_ms){
    int i, numplan;

    numplan = 0;
    for (i=0; i<nmeth; i++) {
        ++method_usage[i].iters_polled;
        ++method_usage[i].iters_active;
        method_usage[i].plan = 0;
        if (method_usage[i].iters_active <= usage_iters_active) {
            /* recently busy, poll */
            method_usage[i].plan = 1;
            ++numplan;
            *idle_time_ms = 0;  /* busy polling */
        } else if (method_usage[i].iters_polled >= usage_iters_starvation) {
            /* starving, time to poke this one */
            method_usage[i].plan = 1;
            ++numplan;
        } 
    }

    /* if nothing is starving or busy, poll everybody */
    if (numplan == 0) {
        for (i=0; i<nmeth; i++)
            method_usage[i].plan = 1;
        numplan = nmeth;

        /* spread idle time evenly */
        if (*idle_time_ms) {
            *idle_time_ms /= numplan;
            if (*idle_time_ms == 0)
                *idle_time_ms = 1;
        }
        /* note that BMI_testunexpected is always called with idle_time 0 */
    }
}
#endif
#if 0
/** Checks to see if any unexpected messages have completed.
 *
 *  \return 0 on success, -errno on failure.
 */
int 
BMI_testunexpected(int incount,
		   int *outcount,
		   struct BMI_unexpected_info *info_array,
		   int max_idle_time_ms){
    int i = 0;
    int ret = -1;
    int position = 0;
    int tmp_outcount = 0;
    struct bmi_method_unexpected_info sub_info[incount];
    ref_st_p tmp_ref = NULL;
    int tmp_active_method_count = 0;

    /* figure out if we need to drop any stale addresses */
    bmi_check_forget_list();

#ifdef MARCEL
    marcel_mutex_lock(&active_method_count_mutex);
    tmp_active_method_count = active_method_count;
    marcel_mutex_unlock(&active_method_count_mutex);
#else
    tmp_active_method_count = active_method_count;
#endif
    *outcount = 0;

    construct_poll_plan(tmp_active_method_count, &max_idle_time_ms);

    while (position < incount && i < tmp_active_method_count)
    {
        if (method_usage[i].plan) {
            ret = active_method_table[i]->testunexpected(
                (incount - position), &tmp_outcount,
                (&(sub_info[position])), max_idle_time_ms);
            if (ret < 0)
            {
                /* can't recover from this */
                //gossip_lerr("Error: critical BMI_testunexpected failure.\n");
                return (ret);
            }
            position += tmp_outcount;
            (*outcount) += tmp_outcount;
            method_usage[i].iters_polled = 0;
            if (ret)
                method_usage[i].iters_active = 0;
        }
	i++;
    }

    for (i = 0; i < (*outcount); i++)
    {
	info_array[i].error_code = sub_info[i].error_code;
	info_array[i].buffer = sub_info[i].buffer;
	info_array[i].size = sub_info[i].size;
	info_array[i].tag = sub_info[i].tag;
#ifdef MARCEL
	marcel_mutex_lock(&ref_mutex);
#endif
	tmp_ref = ref_list_search_method_addr(
            cur_ref_list, sub_info[i].addr);
	if (!tmp_ref)
	{
	    /* yeah, right */
	    //gossip_lerr("Error: critical BMI_testunexpected failure.\n");
#ifdef MARCEL
	    marcel_mutex_unlock(&ref_mutex);
#endif
	    return 1;

	}
        if(global_flags & BMI_AUTO_REF_COUNT)
        {
            tmp_ref->ref_count++;
        }
#ifdef MARCEL
	marcel_mutex_unlock(&ref_mutex);
#endif
	info_array[i].addr = tmp_ref->bmi_addr;
    }
    /* return 1 if anything completed */
    if (ret == 0 && *outcount > 0)
    {
	return (1);
    }
    return (0);
}
#endif
#if 0
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
    int i = 0;
    int ret = -1;
    int position = 0;
    int tmp_outcount = 0;
    int tmp_active_method_count = 0;
    struct timespec ts;

#ifdef MARCEL
    marcel_mutex_lock(&active_method_count_mutex);
    tmp_active_method_count = active_method_count;
    marcel_mutex_unlock(&active_method_count_mutex);
#else
    tmp_active_method_count = active_method_count;
#endif
    *outcount = 0;

    if(tmp_active_method_count < 1)
    {
	/* nothing active yet, just snooze and return */
	if(max_idle_time_ms > 0)
	{
	    ts.tv_sec = 0;
	    ts.tv_nsec = 2000;
	    nanosleep(&ts, NULL);
	}
	return(0);
    }

    construct_poll_plan(tmp_active_method_count, &max_idle_time_ms);

    while (position < incount && i < tmp_active_method_count)
    {
        if (method_usage[i].plan) {
            ret = active_method_table[i]->testcontext(
                incount - position, 
                &out_id_array[position],
                &tmp_outcount,
                &error_code_array[position], 
                &actual_size_array[position],
                user_ptr_array ?  &user_ptr_array[position] : NULL,
                max_idle_time_ms,
                context_id);
            if (ret < 0)
            {
                /* can't recover from this */
                //gossip_lerr("Error: critical BMI_testcontext failure.\n");
                return (ret);
            }
            position += tmp_outcount;
            (*outcount) += tmp_outcount;
            method_usage[i].iters_polled = 0;
            if (ret)
                method_usage[i].iters_active = 0;
        }
	i++;
    }

    /* return 1 if anything completed */
    if (ret == 0 && *outcount > 0)
    {
	return (1);
    }
    return (0);
}
#endif 
#if 0
/** Performs a reverse lookup, returning the string (URL style)
 *  address for a given opaque address.
 *
 *  NOTE: caller must not free or modify returned string
 *
 *  \return Pointer to string on success, NULL on failure.
 */
const char* 
BMI_addr_rev_lookup(BMI_addr_t addr){
    ref_st_p tmp_ref = NULL;
    char* tmp_str = NULL;

    /* find a reference that matches this address */
#ifdef MARCEL
    marcel_mutex_lock(&ref_mutex);
#endif
    tmp_ref = ref_list_search_addr(cur_ref_list, addr);
    if (!tmp_ref)
    {
#ifdef MARCEL
	marcel_mutex_unlock(&ref_mutex);
#endif
	return (NULL);
    }
#ifdef MARCEL
    marcel_mutex_unlock(&ref_mutex);
#endif    
    tmp_str = tmp_ref->listen_addr;

    return(tmp_str);
}
#endif
#if 0
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
    ref_st_p tmp_ref = NULL;

    /* find a reference that matches this address */
#ifdef MARCEL
    marcel_mutex_lock(&ref_mutex);
#endif
    tmp_ref = ref_list_search_addr(cur_ref_list, addr);
    if (!tmp_ref)
    {
#ifdef MARCEL
	marcel_mutex_unlock(&ref_mutex);
#endif
	return ("UNKNOWN");
    }
#ifdef MARCEL
    marcel_mutex_unlock(&ref_mutex);
#endif
    
    if(!tmp_ref->interface->rev_lookup_unexpected)
    {
        return("UNKNOWN");
    }

    return(tmp_ref->interface->rev_lookup_unexpected(
        tmp_ref->method_addr));
}
#endif

#if 0
#endif
/** Pass in optional parameters.
 *
 *  \return 0 on success, -errno on failure.
 */
#if 0
int 
BMI_set_info(BMI_addr_t addr,
	     int option,
	     void *inout_parameter){
    int ret = -1;
    int i = 0;
    ref_st_p tmp_ref = NULL;

    /* if the addr is NULL, then the set_info should apply to all
     * available methods.
     */
    if (!addr)
    {
#ifdef MARCEL
	marcel_mutex_lock(&active_method_count_mutex);
#endif
	for (i = 0; i < active_method_count; i++)
	{
	    ret = active_method_table[i]->set_info(
                option, inout_parameter);
	    /* we bail out if even a single set_info fails */
	    if (ret < 0)
	    {
#ifdef MARCEL
		marcel_mutex_unlock(&active_method_count_mutex);
#endif
		return (ret);
	    }
	}
#ifdef MARCEL
	marcel_mutex_unlock(&active_method_count_mutex);
#endif
	return (0);
    }

    /* find a reference that matches this address */
#ifdef MARCEL
    marcel_mutex_lock(&ref_mutex);
#endif
    tmp_ref = ref_list_search_addr(cur_ref_list, addr);
    if (!tmp_ref)
    {
#ifdef MARCEL
	marcel_mutex_unlock(&ref_mutex);
#endif
    /* shortcut address reference counting */
    if(option == BMI_INC_ADDR_REF)
    {
	tmp_ref->ref_count++;
#ifdef MARCEL
	marcel_mutex_unlock(&ref_mutex);
#endif
	return(0);
    }
    if(option == BMI_DEC_ADDR_REF)
    {
	tmp_ref->ref_count--;
	assert(tmp_ref->ref_count >= 0);

	if(tmp_ref->ref_count == 0)
	{
            bmi_addr_drop(tmp_ref);
	}
#ifdef MARCEL
	marcel_mutex_unlock(&ref_mutex);
#endif
	return(0);
    }
#ifdef MARCEL
    marcel_mutex_unlock(&ref_mutex);
#endif

    ret = tmp_ref->interface->set_info(option, inout_parameter);

    return (ret);
}
#endif
/** Query for optional parameters.
 *
 *  \return 0 on success, -errno on failure.
 */
#if 0
int 
BMI_get_info(BMI_addr_t addr,
	     int option,
	     void *inout_parameter){
    int i = 0;
    int maxsize = 0;
    int tmp_maxsize;
    int ret = 0;
    ref_st_p tmp_ref = NULL;

    switch (option)
    {
	/* check to see if the interface is initialized */
    case BMI_CHECK_INIT:
#ifdef MARCEL
	marcel_mutex_lock(&active_method_count_mutex);
#endif
	if (active_method_count > 0)
	{
#ifdef MARCEL
	    marcel_mutex_unlock(&active_method_count_mutex);
#endif
	    return (0);
	}
	else
	{
#ifdef MARCEL
	    marcel_mutex_unlock(&active_method_count_mutex);
#endif
	}
    case BMI_CHECK_MAXSIZE:
#ifdef MARCEL
	marcel_mutex_lock(&active_method_count_mutex);
#endif
	for (i = 0; i < active_method_count; i++)
	{
	    ret = active_method_table[i]->get_info(
                option, &tmp_maxsize);
	    if (ret < 0)
	    {
		return (ret);
	    }
	    if (i == 0)
	    {
		maxsize = tmp_maxsize;
	    }
	    else
	    {
		if (tmp_maxsize < maxsize)
		    maxsize = tmp_maxsize;
	    }
	    *((int *) inout_parameter) = maxsize;
	}
#ifdef MARCEL
	marcel_mutex_unlock(&active_method_count_mutex);
#endif
	break;
    case BMI_GET_METH_ADDR:
#ifdef MARCEL
	marcel_mutex_lock(&ref_mutex);
#endif
	tmp_ref = ref_list_search_addr(cur_ref_list, addr);
	if(!tmp_ref)
	{
#ifdef MARCEL
	    marcel_mutex_unlock(&ref_mutex);
#endif
	}
#ifdef MARCEL
	marcel_mutex_unlock(&ref_mutex);
#endif
	*((void**) inout_parameter) = tmp_ref->method_addr;
	break;
    case BMI_GET_UNEXP_SIZE:
#ifdef MARCEL
        marcel_mutex_lock(&ref_mutex);
#endif
        tmp_ref = ref_list_search_addr(cur_ref_list, addr);
        if(!tmp_ref)
        {
#ifdef MARCEL
            marcel_mutex_unlock(&ref_mutex);
#endif
	}
#ifdef MARCEL
        marcel_mutex_unlock(&ref_mutex);
#endif
        ret = tmp_ref->interface->get_info(
            option, inout_parameter);
        if(ret < 0)
        {
            return ret;
        }
        break;

    default:
	return 1;
    }
    return (0);
}
#endif
#if 0
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
    int ret = -1;
    int i = 0, failed = 1;
    int provided_method_length = 0;
    char *ptr, *provided_method_name = NULL;
    ref_st_p tmp_ref = NULL;
    /* lookup the provided address */
#ifdef MARCEL
    marcel_mutex_lock(&ref_mutex);
#endif
    tmp_ref = ref_list_search_addr(cur_ref_list, addr);
    if (!tmp_ref)
    {
#ifdef MARCEL
	marcel_mutex_unlock(&ref_mutex);
#endif
    }
#ifdef MARCEL
    marcel_mutex_unlock(&ref_mutex);
#endif

    ptr = strchr(id_string, ':');
    ret = -EPROTO;
    provided_method_length = (unsigned long) ptr - (unsigned long) id_string;
    provided_method_name = (char *) calloc(provided_method_length + 1, sizeof(char));
    strncpy(provided_method_name, id_string, provided_method_length);

    /* Now we will run through each method looking for one that
     * matches the specified wildcard address. 
     */
    i = 0;
#ifdef MARCEL
    marcel_mutex_lock(&active_method_count_mutex);
#endif
    while (i < active_method_count)
    {
        const char *active_method_name = active_method_table[i]->method_name + 4;
        /* provided name matches this interface */
        if (!strncmp(active_method_name, provided_method_name, provided_method_length))
        {
            int (*meth_fnptr)(bmi_method_addr_p, const char *, int);
            failed = 0;
            if ((meth_fnptr = active_method_table[i]->query_addr_range) == NULL)
            {
                ret = -ENOSYS;
                failed = 1;
                break;
            }
            /* pass it into the specific bmi layer */
            ret = meth_fnptr(tmp_ref->method_addr, id_string, netmask);
            if (ret < 0)
                failed = 1;
            break;
        }
	i++;
    }
#ifdef MARCEL
    marcel_mutex_unlock(&active_method_count_mutex);
#endif
    TBX_FREE(provided_method_name);
    return ret;
}
#endif



/** Similar to BMI_post_send(), except that the source buffer is 
 *  replaced by a list of (possibly non contiguous) buffers.
 *
 *  \return 0 on success, 1 on immediate successful completion,
 *  -errno on failure.
 */
#if 0
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
    ref_st_p tmp_ref = NULL;
    int ret = -1;

    *id = 0;
    tmp_ref=__BMI_get_ref(dest);
    if (tmp_ref->interface->post_send_list) {
	ret = tmp_ref->interface->post_send_list(
            id, tmp_ref->method_addr, buffer_list, size_list,
            list_count, total_size, buffer_type, tag, user_ptr,
            context_id, (bmi_hint)hints);
	return (ret);
    }
}
#endif

/** Similar to BMI_post_recv(), except that the dest buffer is 
 *  replaced by a list of (possibly non contiguous) buffers
 *
 *  \param total_expected_size the sum of the size list.
 *  \param total_actual_size the aggregate amt that was received.
 *
 *  \return 0 on success, 1 on immediate successful completion,
 *  -errno on failure.
 */
#if 0
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
    ref_st_p tmp_ref = NULL;
    int ret = -1;

    *id = 0;
    tmp_ref=__BMI_get_ref(src);
    if (tmp_ref->interface->post_recv_list)
    {
	ret = tmp_ref->interface->post_recv_list(
            id, tmp_ref->method_addr, buffer_list, size_list,
            list_count, total_expected_size, total_actual_size,
            buffer_type, tag, user_ptr, context_id, (bmi_hint)hints);

	return (ret);
    }
}
#endif

/** Similar to BMI_post_sendunexpected(), except that the source buffer is 
 *  replaced by a list of (possibly non contiguous) buffers.
 *
 *  \param total_size the sum of the size list.
 *
 *  \return 0 on success, 1 on immediate successful completion,
 *  -errno on failure.
 */
#if 0
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
    ref_st_p tmp_ref = NULL;
    int ret = -1;

    *id = 0;

    tmp_ref=__BMI_get_ref(dest);

    if (tmp_ref->interface->post_send_list)
    {
	ret = tmp_ref->interface->post_sendunexpected_list(
            id, tmp_ref->method_addr, buffer_list, size_list,
            list_count, total_size, buffer_type, tag, user_ptr,
            context_id, (bmi_hint)hints);

	return (ret);
    }
}
#endif
#if 0
/** Attempts to cancel a pending operation that has not yet completed.
 *  Caller must still test to gather error code after calling this
 *  function even if it returns 0.
 *
 *  \return 0 on success, -errno on failure.
 */
int 
BMI_cancel(bmi_op_id_t id, 
	   bmi_context_id context_id){
    struct method_op *target_op = NULL;
    int ret = -1;

    /* todo: get rid of this */
    //target_op = id_gen_fast_lookup(id);
    if(target_op == NULL)
    {
        /* if we can't find the operation, then assume it has already
         * completed naturally.
         */
        return(0);
    }

    assert(target_op->op_id == id);

    if(active_method_table[target_op->addr->method_type]->cancel)
    {
	ret = active_method_table[
            target_op->addr->method_type]->cancel(
                id, context_id);
    }
    return (ret);
}
#endif
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
#if 0
BMI_addr_t 
bmi_method_addr_reg_callback(bmi_method_addr_p map){
    ref_st_p new_ref = NULL;

    /* NOTE: we are trusting the method to make sure that we really
     * don't know about the address yet.  No verification done here.
     */

    /* create a new reference structure */
    new_ref = alloc_ref_st();
    if (!new_ref)
    {
	return 0;
    }

    /*
      fill in the details; we don't have an id string for this one.
    */
    new_ref->method_addr = map;
    new_ref->id_string = NULL;
    map->parent = new_ref;

    /* check the method_type from the method_addr pointer to know
     * which interface to use */
    new_ref->interface = active_method_table[map->method_type];

    /* add the reference structure to the list */
    ref_list_add(cur_ref_list, new_ref);

    return new_ref->bmi_addr;
}
#endif
#if 0
int 
bmi_method_addr_forget_callback(BMI_addr_t addr){
    struct forget_item* tmp_item = NULL;

    tmp_item = (struct forget_item*)TBX_MALLOC(sizeof(struct forget_item));
    tmp_item->addr = addr;

    /* add to queue of items that we want the BMI control layer to consider
     * deallocating
     */
#ifdef MARCEL
    marcel_mutex_lock(&forget_list_mutex);
#endif
    list_add(&tmp_item->link, &forget_list);
#ifdef MARCEL
    marcel_mutex_unlock(&forget_list_mutex);
#endif
    return (0);
}
#endif

#if 0
/*
 * Attempt to insert this name into the list of active methods,
 * and bring it up.
 * NOTE: assumes caller has protected active_method_count with a mutex lock
 */
static int
activate_method(const char *name, 
		const char *listen_addr, 
		int flags){
    int i, ret;
    void *x;
    struct bmi_method_ops *meth;
    bmi_method_addr_p new_addr;

    /* already active? */
    for (i=0; i<active_method_count; i++)
	if (!strcmp(active_method_table[i]->method_name, name)) break;
    if (i < active_method_count)
    {
	return 0;
    }

    /* is the method known? */
    for (i=0; i<known_method_count; i++)
	if (!strcmp(known_method_table[i]->method_name, name)) break;
    if (i == known_method_count) {
	return -ENOPROTOOPT;
    }
    meth = known_method_table[i];

    /*
     * Later: try to load a dynamic module, growing the known method
     * table and search it again.
     */

    /* toss it into the active table */
    x = active_method_table;
    active_method_table = TBX_MALLOC((active_method_count + 1) 
				     * sizeof(*active_method_table));
    if (!active_method_table) {
	active_method_table = x;
	return -ENOMEM;
    }
    if (active_method_count) {
	memcpy(active_method_table, x,
	    active_method_count * sizeof(*active_method_table));
	TBX_FREE(x);
    }
    active_method_table[active_method_count] = meth;

    x = method_usage;
    method_usage = TBX_MALLOC((active_method_count + 1) * sizeof(*method_usage));
    if (!method_usage) {
        method_usage = x;
        return -ENOMEM;
    }
    if (active_method_count) {
        memcpy(method_usage, x, active_method_count * sizeof(*method_usage));
        TBX_FREE(x);
    }
    memset(&method_usage[active_method_count], 0, sizeof(*method_usage));

    ++active_method_count;

    /* initialize it */
    new_addr = 0;
    if (listen_addr) {
	new_addr = meth->method_addr_lookup(listen_addr);
	if (!new_addr) {
	    --active_method_count;
	    return -EINVAL;
	}
	/* this is a bit of a hack */
	new_addr->method_type = active_method_count - 1;
    }
    ret = meth->initialize(new_addr, active_method_count - 1, flags);
    if (ret < 0) {
	--active_method_count;
	return ret;
    }

    /* tell it about any open contexts */
    for (i=0; i<BMI_MAX_CONTEXTS; i++)
	if (context_array[i]) {
	    ret = meth->open_context(i);
	    if (ret < 0)
		break;
	}

    return ret;
}
#endif

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
/* bmi_check_forget_list()
 * 
 * Scans queue of items that methods have suggested that we forget about 
 *
 * no return value
 */
#if 0
static void 
bmi_check_forget_list(void){
    BMI_addr_t tmp_addr;
    struct forget_item* tmp_item;
    ref_st_p tmp_ref = NULL;
    
#ifdef MARCEL
    marcel_mutex_lock(&forget_list_mutex);
#endif
    while(!list_empty(&forget_list))
    {
        tmp_item = list_entry(forget_list.next, struct forget_item,
            link);     
        list_del(&tmp_item->link);
        /* item is off of the list; unlock for a moment while we work on
         * this addr 
         */
#ifdef MARCEL
        marcel_mutex_unlock(&forget_list_mutex);
#endif
        tmp_addr = tmp_item->addr;
        TBX_FREE(tmp_item);

#ifdef MARCEL
        marcel_mutex_lock(&ref_mutex);
#endif
        tmp_ref = ref_list_search_addr(cur_ref_list, tmp_addr);
        if(tmp_ref && tmp_ref->ref_count == 0)
        {
            bmi_addr_drop(tmp_ref);
        }   
#ifdef MARCEL
        marcel_mutex_unlock(&ref_mutex);
        marcel_mutex_lock(&forget_list_mutex);
#endif
     }
#ifdef MARCEL
    marcel_mutex_unlock(&forget_list_mutex);
#endif

    return;
}
#endif
#if 0
/* bmi_addr_drop
 *
 * Destroys a complete BMI address, including asking the method to clean up 
 * its portion.  Will query the method for permission before proceeding
 *
 * NOTE: must be called with ref list mutex held 
 */
static void 
bmi_addr_drop(ref_st_p tmp_ref){
    struct method_drop_addr_query query;
    query.response = 0;
    query.addr = tmp_ref->method_addr;
    int ret = 0;

    /* reference count is zero; ask module if it wants us to discard
     * the address; TCP will tell us to drop addresses for which the
     * socket has died with no possibility of reconnect 
     */
    ret = tmp_ref->interface->get_info(BMI_DROP_ADDR_QUERY,
        &query);
    if(ret == 0 && query.response == 1)
    {
        /* kill the address */
#if 0
        gossip_debug(GOSSIP_BMI_DEBUG_CONTROL,
            "[BMI CONTROL]: %s: bmi discarding address: %llu\n",
            __func__, llu(tmp_ref->bmi_addr));
#endif
        ref_list_rem(cur_ref_list, tmp_ref->bmi_addr);
        /* NOTE: this triggers request to module to free underlying
         * resources if it wants to
         */
        dealloc_ref_st(tmp_ref);
    }
    return;
}
#endif
/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 expandtab
 */
