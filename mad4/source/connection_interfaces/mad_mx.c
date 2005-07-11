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
 * Mad_mx.c
 * =========
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <myriexpress.h>

#include "madeleine.h"
#include <assert.h>

/*
 * macros
 * ------
 */
#ifdef MARCEL
#define MAD_MX_POLLING_MODE \
    (MARCEL_POLL_AT_TIMER_SIG | MARCEL_POLL_AT_YIELD | MARCEL_POLL_AT_IDLE)
#endif /* MARCEL */

/*
 * local structures
 * ----------------
 */
typedef struct s_mad_mx_driver_specific{
    int dummy;
#ifdef MARCEL
    marcel_pollid_t         mx_pollid;
#endif /* MARCEL */
} mad_mx_driver_specific_t, *p_mad_mx_driver_specific_t;

typedef struct s_mad_mx_adapter_specific{
    uint64_t                nic_id;
    uint32_t                board_id;

    p_tbx_htable_t          connexion_established_htable;

    mx_endpoint_addr_t	    endpoint1_addr;
    mx_endpoint_t	    endpoint1;
    uint32_t		    endpoint1_id;

    mx_endpoint_addr_t	    endpoint2_addr;
    mx_endpoint_t	    endpoint2;
    uint32_t		    endpoint2_id;
} mad_mx_adapter_specific_t, *p_mad_mx_adapter_specific_t;

typedef struct s_mad_mx_channel_specific{
    tbx_bool_t		first_incoming_packet_flag;
    mx_segment_t        *recv_msg;
} mad_mx_channel_specific_t, *p_mad_mx_channel_specific_t;

typedef struct s_mad_mx_connection_specific{
    uint32_t		    remote_endpoint1_id;
    mx_endpoint_addr_t	    remote_endpoint1_addr;

    uint32_t		    remote_endpoint2_id;
    mx_endpoint_addr_t	    remote_endpoint2_addr;

    tbx_bool_t		    first_outgoing_packet_flag;
} mad_mx_connection_specific_t, *p_mad_mx_connection_specific_t;

typedef struct s_mad_mx_link_specific{
    int                     dummy;
} mad_mx_link_specific_t, *p_mad_mx_link_specific_t;

#ifdef MARCEL
typedef struct s_mad_mx_poll_req{
    mx_endpoint_t 	    endpoint;
    mx_request_t	   *p_request;
    mx_return_t            *p_rc;
    mx_status_t	           *p_status;
    uint32_t	           *p_result;
} mad_mx_poll_req_t, *p_mad_mx_poll_req_t;
#endif /* MARCEL */

typedef struct s_mad_mx_on_going_specific{
    mx_endpoint_t  endpoint;
    mx_request_t  *mx_request;
}mad_mx_on_going_specific_t, *p_mad_mx_on_going_specific_t;

#define MAD_MX_GATHER_SCATTER_THRESHOLD  32768
#define MAD_MX_SMALL_MSG_SIZE            32000

static p_tbx_memory_t mad_mx_unexpected_key;

/*
 * Malloc protection hooks
 * -----------------------
 */

/* Prototypes */
static
void *
mad_mx_malloc_hook(size_t len, const void *caller);

static
void *
mad_mx_memalign_hook(size_t alignment, size_t len, const void *caller);

static
void
mad_mx_free_hook(void *ptr, const void *caller);

static
void *
mad_mx_realloc_hook(void *ptr, size_t len, const void *caller);

/* Previous handlers */
static
void *
(*mad_mx_old_malloc_hook)(size_t len, const void *caller) = NULL;

static
void *
(*mad_mx_old_memalign_hook)(size_t alignment, size_t len, const void *caller) = NULL;

static
void
(*mad_mx_old_free_hook)(void *PTR, const void *CALLER) = NULL;

static
void *
(*mad_mx_old_realloc_hook)(void *PTR, size_t LEN, const void *CALLER) = NULL;

#if 0
/*
 * Malloc hooks installation is delayed to mad_mx_driver_init because
 * __malloc_initialize_hook is already defined by the MX library.
 *
 * This is expected to be safe because at the time mad_mx_driver_init is called,
 * no polling request should already be pending.
 */

/* Entry point */
void (*__malloc_initialize_hook) (void) = mad_mx_malloc_initialize_hook;
#endif /* 0 */

/*
 * static functions
 * ----------------
 */
static
inline
void
__mad_mx_hook_lock(void) {
#ifdef MARCEL
    marcel_poll_lock();


#endif /* MARCEL */
}

static
inline
void
__mad_mx_hook_unlock(void) {
#ifdef MARCEL


    marcel_poll_unlock();
#endif /* MARCEL */
}

static
void
mad_mx_install_hooks(void) {
    LOG_IN();
    mad_mx_old_malloc_hook		= __malloc_hook;
    mad_mx_old_memalign_hook	= __memalign_hook;
    mad_mx_old_free_hook		= __free_hook;
    mad_mx_old_realloc_hook		= __realloc_hook;

    if (__malloc_hook == mad_mx_malloc_hook)
        FAILURE("hooks corrupted");

    if (__memalign_hook == mad_mx_memalign_hook)
        FAILURE("hooks corrupted");

    if (__realloc_hook == mad_mx_realloc_hook)
        FAILURE("hooks corrupted");

    if (__free_hook == mad_mx_free_hook)
        FAILURE("hooks corrupted");

    __malloc_hook		= mad_mx_malloc_hook;
    __memalign_hook		= mad_mx_memalign_hook;
    __free_hook		= mad_mx_free_hook;
    __realloc_hook		= mad_mx_realloc_hook;
    LOG_OUT();
}

static
void
mad_mx_remove_hooks(void) {
    LOG_IN();
    if (__malloc_hook == mad_mx_old_malloc_hook)
        FAILURE("hooks corrupted");

    if (__memalign_hook == mad_mx_old_memalign_hook)
        FAILURE("hooks corrupted");

    if (__realloc_hook == mad_mx_old_realloc_hook)
        FAILURE("hooks corrupted");

    if (__free_hook == mad_mx_old_free_hook)
        FAILURE("hooks corrupted");

    __malloc_hook		= mad_mx_old_malloc_hook;
    __memalign_hook		= mad_mx_old_memalign_hook;
    __free_hook		= mad_mx_old_free_hook;
    __realloc_hook		= mad_mx_old_realloc_hook;
    LOG_OUT();
}

static
void *
mad_mx_malloc_hook(size_t len, const void *caller) {
    void *new_ptr = NULL;

    __mad_mx_hook_lock();
    mad_mx_remove_hooks();
    LOG_IN();
    new_ptr = malloc(len);
    LOG_OUT();
    mad_mx_install_hooks();
    __mad_mx_hook_unlock();

    return new_ptr;
}

static
void *
mad_mx_memalign_hook(size_t alignment, size_t len, const void *caller) {
    void *new_ptr = NULL;

    __mad_mx_hook_lock();
    mad_mx_remove_hooks();
    LOG_IN();
    new_ptr = memalign(alignment, len);
    LOG_OUT();
    mad_mx_install_hooks();
    __mad_mx_hook_unlock();

    return new_ptr;
}

static
void *
mad_mx_realloc_hook(void *ptr, size_t len, const void *caller) {
    void *new_ptr = NULL;

    __mad_mx_hook_lock();
    mad_mx_remove_hooks();
    LOG_IN();
    new_ptr = realloc(ptr, len);
    LOG_OUT();
    mad_mx_install_hooks();
    __mad_mx_hook_unlock();

    return new_ptr;
}

static
void
mad_mx_free_hook(void *ptr, const void *caller) {

    __mad_mx_hook_lock();
    mad_mx_remove_hooks();
    LOG_IN();
    free(ptr);
    LOG_OUT();
    mad_mx_install_hooks();
    __mad_mx_hook_unlock();
}

static
inline
void
mad_mx_lock(void) {
#ifdef MARCEL
    marcel_poll_lock();
    mad_mx_remove_hooks();


#endif /* MARCEL */
}

static
inline
void
mad_mx_unlock(void) {
#ifdef MARCEL


    mad_mx_install_hooks();
    marcel_poll_unlock();
#endif /* MARCEL */
}

static
void
mad_mx_print_status(mx_status_t s) TBX_UNUSED;

static
void
mad_mx_print_status(mx_status_t s) {
    DISP("status");

    switch (s.code) {
    case MX_STATUS_SUCCESS:
        DISP("Successful completion");
        break;
    case MX_STATUS_PENDING:
        DISP("Request still pending");
        break;
    case MX_STATUS_BUFFERED:
        DISP("Request has been buffered, but still pending");
        break;
    case MX_STATUS_REJECTED:
        DISP("Posted operation failed");
        break;
    case MX_STATUS_TIMEOUT:
        DISP("Posted operation timed out");
        break;
    case MX_STATUS_TRUNCATED:
        DISP("Operation completed, but data was truncated due to undersized buffer");
        break;
    case MX_STATUS_CANCELLED:
        DISP("Pending receive was cancelled");
        break;
    case MX_STATUS_ENDPOINT_UNKNOWN:
        DISP("Destination endpoint is unknown on the network fabric");
        break;
    case MX_STATUS_ENDPOINT_CLOSED:
        DISP("remoted endpoint is closed");
        break;
    case MX_STATUS_ENDPOINT_UNREACHABLE:
        DISP("Connectivity is broken between the source and the destination");
        /*
         * case MX_STATUS_BAD_SESSION:
         * DISP("Bad session (no mx_connect done?)"
         */
        break;
    case MX_STATUS_BAD_KEY:
        DISP("Connect failed because of bad credentials");
        break;
    }
}

static
char *
mad_mx_return_code(mx_return_t return_code) {
    char *msg = NULL;

    switch (return_code) {
    case MX_SUCCESS:
        msg = "MX_SUCCESS";
        break;

    case MX_BAD_BAD_BAD:
        msg = "MX_BAD_BAD_BAD";
        break;

    case MX_FAILURE:
        msg = "MX_FAILURE";
        break;

    case MX_ALREADY_INITIALIZED:
        msg = "MX_ALREADY_INITIALIZED";
        break;

    case MX_NOT_INITIALIZED:
        msg = "MX_NOT_INITIALIZED";
        break;

    case MX_NO_DEV:
        msg = "MX_NO_DEV";
        break;

    case MX_NO_DRIVER:
        msg = "MX_NO_DRIVER";
        break;

    case MX_NO_PERM:
        msg = "MX_NO_PERM";
        break;

    case MX_BOARD_UNKNOWN:
        msg = "MX_BOARD_UNKNOWN";
        break;

    case MX_BAD_ENDPOINT:
        msg = "MX_BAD_ENDPOINT";
        break;

    case MX_BAD_SEG_LIST:
        msg = "MX_BAD_SEG_LIST";
        break;

    case MX_BAD_SEG_MEM:
        msg = "MX_BAD_SEG_MEM";
        break;

    case MX_BAD_SEG_CNT:
        msg = "MX_BAD_SEG_CNT";
        break;

    case MX_BAD_REQUEST:
        msg = "MX_BAD_REQUEST";
        break;

    case MX_BAD_MATCH_MASK:
        msg = "MX_BAD_MATCH_MASK";
        break;

    case MX_NO_RESOURCES:
        msg = "MX_NO_RESOURCES";
        break;

    case MX_BAD_ADDR_LIST:
        msg = "MX_BAD_ADDR_LIST";
        break;

    case MX_BAD_ADDR_COUNT:
        msg = "MX_BAD_ADDR_COUNT";
        break;

    case MX_BAD_ROOT:
        msg = "MX_BAD_ROOT";
        break;

    case MX_NOT_COMPLETED:
        msg = "MX_NOT_COMPLETED";
        break;

    case MX_BUSY:
        msg = "MX_BUSY";
        break;

    case MX_BAD_INFO_KEY:
        msg = "MX_BAD_INFO_KEY";
        break;

    case MX_BAD_INFO_VAL:
        msg = "MX_BAD_INFO_VAL";
        break;

    case MX_BAD_NIC:
        msg = "MX_BAD_NIC";
        break;

    case MX_BAD_PARAM_LIST:
        msg = "MX_BAD_PARAM_LIST";
        break;

    case MX_BAD_PARAM_NAME:
        msg = "MX_BAD_PARAM_NAME";
        break;

    case MX_BAD_PARAM_VAL:
        msg = "MX_BAD_PARAM_VAL";
        break;

    case MX_BAD_HOSTNAME_ARGS:
        msg = "MX_BAD_HOSTNAME_ARGS";
        break;

    case MX_HOST_NOT_FOUND:
        msg = "MX_HOST_NOT_FOUND";
        break;

    case MX_REQUEST_PENDING:
        msg = "MX_REQUEST_PENDING";
        break;

    case MX_TIMEOUT:
        msg = "MX_TIMEOUT";
        break;

    case MX_NO_MATCH:
        msg = "MX_NO_MATCH";
        break;

    case MX_BAD_ENDPOINT_ID:
        msg = "MX_BAD_ENDPOINT_ID";
        break;

    case MX_CONNECTION_FAILED:
        msg = "MX_CONNECTION_FAILED";
        break;

    case MX_BAD_CONNECTION_KEY:
        msg = "MX_BAD_CONNECTION_KEY";
        break;

    case MX_BAD_INFO_LENGTH:
        msg = "MX_BAD_INFO_LENGTH";
        break;

    default:
        msg = "unknown";
        break;

    }

    return msg;
}

static
void
mad_mx_check_return(char *msg, mx_return_t return_code) {
    if (return_code != MX_SUCCESS) {
        TRACE("%s failed with code %s = %d/0x%x", msg, mad_mx_return_code(return_code), return_code, return_code);
        FAILURE("mx error");
    }
}

static
void
mad_mx_startup_info(void) {
    mx_return_t	 return_code	= MX_SUCCESS;
    uint32_t 	 nb_nic		= 0;
    uint64_t	*nic_id_array	= NULL;
    uint32_t	 nb_native_ep	= 0;
    uint32_t 	 i		= 0;

    LOG_IN();
    DISP("Madeleine/MX: hardware configuration");


    mad_mx_lock();
    return_code = mx_get_info(NULL, MX_NIC_COUNT, &nb_nic, sizeof(nb_nic));
    mad_mx_unlock();
    mad_mx_check_return("mad_mx_startup_info", return_code);
    DISP("NIC count: %d", nb_nic);


    nic_id_array = TBX_MALLOC((nb_nic+1) * sizeof(*nic_id_array));
    mad_mx_check_return("mad_mx_startup_info", return_code);

    mad_mx_lock();
    return_code = mx_get_info(NULL, MX_NIC_IDS, nic_id_array, (nb_nic+1) * sizeof(*nic_id_array));
    mad_mx_unlock();
    mad_mx_check_return("mad_mx_startup_info", return_code);

    for (i = 0; i < nb_nic; i++) {
        DISP("NIC %d id: %llx", i, nic_id_array[i]);
    }
    TBX_FREE(nic_id_array);
    nic_id_array = NULL;


    mad_mx_lock();
    return_code = mx_get_info(NULL, MX_MAX_NATIVE_ENDPOINTS, &nb_native_ep, sizeof(nb_native_ep));
    mad_mx_unlock();
    mad_mx_check_return("mad_mx_startup_info", return_code);
    DISP("Native endpoint count: %d", nb_native_ep);

    LOG_OUT();
}

#ifdef MARCEL
static void
mad_mx_marcel_group(marcel_pollid_t id)
{
    return;
}

static
int
mad_mx_do_poll(p_mad_mx_poll_req_t rq) {
    int success = 0;

    LOG_IN();
    mad_mx_remove_hooks();
    *(rq->p_rc)	= mx_test(rq->endpoint, rq->p_request, rq->p_status, rq->p_result);
    success		= !(*(rq->p_rc) == MX_SUCCESS && !*(rq->p_result));
    mad_mx_install_hooks();
    LOG_OUT();

    return success;
}

static void *
mad_mx_marcel_fast_poll(marcel_pollid_t id,
                        any_t           req,
                        boolean         first_call) {
    void *status = MARCEL_POLL_FAILED;

    LOG_IN();
    if (mad_mx_do_poll((p_mad_mx_poll_req_t) req)) {
        status = MARCEL_POLL_SUCCESS_FOR(id);
    }
    LOG_OUT();

    return status;
}

static void *
mad_mx_marcel_poll(marcel_pollid_t id,
                   unsigned        active,
                   unsigned        sleeping,
                   unsigned        blocked) {
    p_mad_mx_poll_req_t  req = NULL;
    void                *status = MARCEL_POLL_FAILED;

    LOG_IN();
    FOREACH_POLL(id, req) {
        if (mad_mx_do_poll((p_mad_mx_poll_req_t) req)) {
            status = MARCEL_POLL_SUCCESS(id);
            goto found;
        }
    }

 found:
    LOG_OUT();

    return status;
}
#endif /* MARCEL */

static mx_endpoint_t
mad_mx_choose_endpoint(p_mad_mx_adapter_specific_t as,
                       size_t size){
    LOG_IN();
    if(size <= MAD_MX_SMALL_MSG_SIZE){

        return as->endpoint1;
    } else {

        return as->endpoint2;
    }
    LOG_OUT();
}

static mx_endpoint_addr_t
mad_mx_choose_remote_endpoint(p_mad_mx_connection_specific_t os,
                              size_t size){
    LOG_IN();
    if(size <= MAD_MX_SMALL_MSG_SIZE){

        return os->remote_endpoint1_addr;
    } else {

        return os->remote_endpoint2_addr;
    }
    LOG_OUT();
}

static void
mad_mx_irecv(mx_endpoint_t ep,
             mx_segment_t	*seg_list,
             uint32_t		 nb_seg,
             uint64_t		 match_info,
             uint64_t		 match_mask,
             mx_request_t	*p_request){
    mx_return_t     		rc	= MX_SUCCESS;

    LOG_IN();
    mad_mx_lock();
    rc	= mx_irecv(ep,
                   seg_list, nb_seg,
                   match_info,
                   match_mask, NULL, p_request);
    mad_mx_unlock();
    mad_mx_check_return("mx_irecv", rc);
    LOG_OUT();
}

static void
mad_mx_isend(mx_endpoint_t       ep,
             mx_endpoint_addr_t  dest_address,
             mx_segment_t	*seg_list,
             uint32_t		 nb_seg,
             uint64_t		 match_info,
             mx_request_t 	*p_request){
    mx_return_t rc = MX_SUCCESS;

    LOG_IN();
    mad_mx_lock();

    rc	= mx_isend(ep,
                   seg_list, nb_seg,
                   dest_address,
                   match_info,
                   NULL, p_request);
    mad_mx_unlock();
    mad_mx_check_return("mx_isend", rc);
    LOG_OUT();
}

/*
 * Registration function
 * ---------------------
 */
char *
mad_mx_register(p_mad_driver_interface_t interface){
    LOG_IN();
    TRACE("Registering MX driver");
    interface->driver_init                = mad_mx_driver_init;
    interface->adapter_init               = mad_mx_adapter_init;
    interface->channel_init               = mad_mx_channel_init;
    interface->before_open_channel        = NULL;
    interface->connection_init            = mad_mx_connection_init;
    interface->link_init                  = mad_mx_link_init;
    interface->accept                     = mad_mx_accept;
    interface->connect                    = mad_mx_connect;
    interface->after_open_channel         = NULL;
    interface->before_close_channel       = NULL;
    interface->disconnect                 = mad_mx_disconnect;
    interface->after_close_channel        = NULL;
    interface->link_exit                  = NULL;
    interface->connection_exit            = mad_mx_connection_exit;
    interface->channel_exit               = mad_mx_channel_exit;
    interface->adapter_exit               = mad_mx_adapter_exit;
    interface->driver_exit                = mad_mx_driver_exit;
    interface->choice                     = NULL;
    interface->get_static_buffer          = NULL;
    interface->return_static_buffer       = NULL;
    interface->new_message            = mad_mx_new_message;
    interface->finalize_message  = mad_mx_finalize_message;
#ifdef MAD_MESSAGE_POLLING
    interface->poll_message               = mad_mx_poll_message;
#endif
    interface->receive_message        = mad_mx_receive_message;
    interface->message_received           = NULL;

    interface->send_iovec                 = mad_mx_send_iovec;
    interface->receive_iovec              = mad_mx_receive_iovec;

    interface->s_make_progress     = mad_mx_s_make_progress;
    interface->r_make_progress     = mad_mx_r_make_progress;

    interface->sending_track_init   = mad_mx_sending_track_init;
    interface->reception_track_init = mad_mx_reception_track_init;
    interface->sending_track_exit   = mad_mx_sending_track_exit;
    interface->reception_track_exit = mad_mx_reception_track_exit;

    interface->need_rdv             = mad_mx_need_rdv;
    interface->limit_size           = mad_mx_limit_size;

    interface->create_areas        = mad_mx_create_areas;

    LOG_OUT();
    return "mx";
}


void
mad_mx_driver_init(p_mad_driver_t d, int *argc, char ***argv) {
    p_mad_mx_driver_specific_t	ds		= NULL;
    mx_return_t			return_code	= MX_SUCCESS;

    LOG_IN();
    d->connection_type  = mad_bidirectional_connection;
    d->buffer_alignment = 32;
    
    TRACE("Initializing MX driver");
#ifdef MARCEL
    if (!mad_mx_malloc_hooked) {
        mad_mx_malloc_initialize_hook();
    }
#endif /* MARCEL */
    ds  		= TBX_MALLOC(sizeof(mad_mx_driver_specific_t));

    d->specific	= ds;

    mad_mx_lock();
    mx_set_error_handler(MX_ERRORS_RETURN);
    mad_mx_unlock();

    mad_mx_lock();
    return_code	= mx_init();
    mad_mx_unlock();
    mad_mx_check_return("mx_init", return_code);

    /* debug only */
#ifndef MX_NO_STARTUP_INFO
    mad_mx_startup_info();
#endif // MX_NO_STARTUP_INFO

#ifdef MARCEL
    if (!mad_mx_ev_server_started) {
        marcel_ev_server_start(&mad_mx_ev_server);
        mad_mx_ev_server_started = tbx_true;
    }
#endif /* MARCEL */
    LOG_OUT();
}

static uint32_t
initialize_endpoint(mx_endpoint_t *e, uint64_t nic_id){
    uint32_t                    e_id     	= 1;
    const uint32_t		e_key		= 0xFFFFFFFF;
    mx_return_t			return_code	= MX_SUCCESS;

    LOG_IN();
    mad_mx_lock();
    do {
        return_code	= mx_open_endpoint(nic_id,
                                           e_id,
                                           e_key,
                                           NULL,
                                           0,
                                           e);
    } while (return_code == MX_BUSY && e_id++);
    mad_mx_unlock();
    mad_mx_check_return("mx_open_endpoint", return_code);
    LOG_OUT();

    return e_id;
}

static mx_endpoint_addr_t
mad_mx_get_endpoint_addr(mx_endpoint_t e){
    mx_endpoint_addr_t		e_addr;
    mx_return_t			return_code	= MX_SUCCESS;

    LOG_IN();
    mad_mx_lock();
    return_code	= mx_get_endpoint_addr(e, &e_addr);
    mad_mx_unlock();
    mad_mx_check_return("mx_get_endpoint", return_code);
    LOG_OUT();

    return e_addr;
}


void
mad_mx_adapter_init(p_mad_adapter_t a) {
    p_mad_mx_adapter_specific_t	as	  = NULL;
    mx_endpoint_t			e1, e2;
    uint32_t                            e1_id           = 0;
    uint32_t                            e2_id           = 0;
    mx_endpoint_addr_t		        e1_addr, e2_addr;

    LOG_IN();
    as		= TBX_MALLOC(sizeof(mad_mx_adapter_specific_t));

    if (strcmp(a->dir_adapter->name, "default")) {
        FAILURE("unsupported adapter");
    } else {
        as->board_id	= MX_ANY_NIC;
    }

    as->connexion_established_htable = tbx_htable_empty_table();

    /* Initialize first endpoint */
    e1_id   = initialize_endpoint(&e1, as->board_id);
    e1_addr = mad_mx_get_endpoint_addr(e1);

    /* Initialize second endpoint */
    e2_id   = initialize_endpoint(&e2, as->board_id);
    e2_addr = mad_mx_get_endpoint_addr(e2);

    as->endpoint1		         = e1;
    as->endpoint1_id		         = e1_id;
    as->endpoint1_addr                   = e1_addr;
    as->endpoint2		         = e2;
    as->endpoint2_id		         = e2_id;
    as->endpoint2_addr                   = e2_addr;

    tbx_malloc_init(&mad_mx_unexpected_key,  MAD_MX_SMALL_MSG_SIZE, 512, "unexpected");
    //tbx_malloc_init(&mad_mx_unexpected_key,  MAD_MX_SMALL_MSG_SIZE, 512);

    a->specific	= as;
    a->parameter	= tbx_strdup("-");
    LOG_OUT();
}


void
mad_mx_channel_init(p_mad_channel_t ch) {
    p_mad_mx_adapter_specific_t 	as = NULL;
    p_tbx_string_t			param_str	= NULL;
    p_mad_mx_channel_specific_t         chs = NULL;

    LOG_IN();
    as = ch->adapter->specific;

    param_str = tbx_string_init_to_int(as->endpoint1_id);
    tbx_string_append_char(param_str, ' ');
    tbx_string_append_int (param_str, as->endpoint2_id);
    ch->parameter	= tbx_string_to_cstring(param_str);
    tbx_string_free(param_str);
    param_str	= NULL;

    chs = TBX_MALLOC(sizeof(mad_mx_channel_specific_t));
    chs->first_incoming_packet_flag	= tbx_false;

    chs->recv_msg = TBX_MALLOC(sizeof(mx_segment_t));
    chs->recv_msg->segment_ptr
        = tbx_malloc(mad_mx_unexpected_key);
    chs->recv_msg->segment_length = MAD_MX_SMALL_MSG_SIZE;

    ch->specific = chs;
    LOG_OUT();
}

void
mad_mx_connection_init(p_mad_connection_t in,
                       p_mad_connection_t out) {
    p_mad_mx_connection_specific_t	cs	= NULL;

    LOG_IN();
    cs	= TBX_MALLOC(sizeof(mad_mx_connection_specific_t));
    cs->remote_endpoint1_id	        = 0;
    cs->remote_endpoint2_id             = 1;

    cs->first_outgoing_packet_flag	= tbx_false;

    in->specific		= cs;
    in->nb_link		        = 1;

    out->specific		= cs;
    out->nb_link		= 1;
    LOG_OUT();
}

static
p_mad_track_t
mad_mx_track_cons(int nb_elm) {
    p_mad_track_t track = NULL;

    LOG_IN();
    track = TBX_MALLOC(sizeof(mad_track_t));
    track->nb_elm    = nb_elm;
    track->pipelines = TBX_CALLOC(track->nb_elm, sizeof(p_mad_pipeline_t));
    LOG_OUT();

    return track;
}

static
p_mad_pipeline_t
mad_mx_pipeline_cons(int nb_elm) {
    p_mad_pipeline_t      pipeline = NULL;

    LOG_IN();
    pipeline	= TBX_MALLOC(sizeof(mad_pipeline_t));

    pipeline->nb_elm		=  nb_elm;
    pipeline->index_first	= -1;
    pipeline->index_last	= -1;
    pipeline->on_goings		= TBX_CALLOC(pipeline->nb_elm, sizeof(p_mad_on_going_t));
    LOG_OUT();

    return pipeline;
}

static
p_mad_on_going_t
mad_mx_ongoing_cons(void) {
    p_mad_on_going_t			og 	= NULL;
    p_mad_mx_on_going_specific_t	og_s	= NULL;

    LOG_IN();
    og   = TBX_MALLOC(sizeof(mad_on_going_t));

    og->area_id		=   -1;
    og->lnk		= NULL;
    og->mad_iovec	= NULL;
    og->status		=    0;

    og_s = TBX_MALLOC(sizeof(mad_mx_on_going_specific_t));
    og_s->endpoint	= NULL;
    og_s->mx_request	= TBX_MALLOC(sizeof(mx_request_t));

    og->specific = og_s;

    LOG_OUT();

    return og;
}

static
p_mad_iovec_t
mad_mx_unexpected_iovec_cons(void) {
    p_mad_iovec_t	 mad_iovec	= NULL;
    void 		*buffer		= NULL;

    LOG_IN();
    mad_iovec	= mad_iovec_create(0, 0);
    buffer	= tbx_malloc(mad_mx_unexpected_key);
    mad_iovec_add_data(mad_iovec, buffer, MAD_MX_SMALL_MSG_SIZE, 0);
    LOG_OUT();

    return mad_iovec;
}


void
mad_mx_sending_track_init(p_mad_adapter_t adapter){
    p_mad_pipeline_t	pipeline	= NULL;
    p_mad_track_t	track		= NULL;

    LOG_IN();
    pipeline = mad_mx_pipeline_cons(2);

    pipeline->on_goings[0] = mad_mx_ongoing_cons();
    pipeline->on_goings[1] = mad_mx_ongoing_cons();

    track = mad_mx_track_cons(1);
    track->pipelines[0] = pipeline;

    adapter->s_track = track;
    LOG_OUT();
}

void
mad_mx_reception_track_init(p_mad_adapter_t adapter){
    p_mad_mx_adapter_specific_t	as		= NULL;
    p_mad_on_going_t		og0		= NULL;
    p_mad_on_going_t		og1		= NULL;
    p_mad_pipeline_t		pipeline0	= NULL;
    p_mad_pipeline_t		pipeline1	= NULL;
    p_mad_track_t		track		= NULL;

    LOG_IN();
    as = adapter->specific;

    /* partie pour les pré-postés */
    og0   = mad_mx_ongoing_cons();
    og0->mad_iovec	= mad_mx_unexpected_iovec_cons();
    og0->area_id	= 0;

    og1   = mad_mx_ongoing_cons();
    og1->mad_iovec	= mad_mx_unexpected_iovec_cons();
    og1->area_id	= 0;

    pipeline0 = mad_mx_pipeline_cons(2);
    pipeline0->on_goings[0] = og0;
    pipeline0->on_goings[1] = og1;

    /* partie pour les gros */
    pipeline1 = mad_mx_pipeline_cons(2);
    pipeline1->on_goings[0] = mad_mx_ongoing_cons();
    pipeline1->on_goings[1] = mad_mx_ongoing_cons();

    /* on regroupe */
    track = mad_mx_track_cons(2);
    track->pipelines[0] = pipeline0;
    track->pipelines[1] = pipeline1;

    adapter->r_track = track;
    LOG_OUT();
}

void
mad_mx_link_init(p_mad_link_t lnk) {
    LOG_IN();

    lnk->link_mode   = mad_link_mode_buffer_group;
    lnk->buffer_mode = mad_buffer_mode_dynamic;
    lnk->group_mode  = mad_group_mode_split;
    LOG_OUT();
}

static mx_endpoint_addr_t
mad_mx_connect_endpoint(mx_endpoint_t *e, uint64_t remote_nic_id, uint32_t remote_endpoint_id){
    mx_endpoint_addr_t re_addr;
    const uint32_t     e_key		= 0xFFFFFFFF;
    mx_return_t        return_code	= MX_SUCCESS;

    LOG_IN();
    mad_mx_lock();
    return_code	= mx_connect(*e,
                             remote_nic_id,
                             remote_endpoint_id,
                             e_key,
                             MX_INFINITE,
                             &re_addr);
    mad_mx_unlock();
    mad_mx_check_return("mx_connect", return_code);
    LOG_OUT();

    return re_addr;
}

static
void
mad_mx_accept_connect(p_mad_connection_t   cnx,
                      p_mad_adapter_info_t ai) {
    p_mad_adapter_t                      a              = NULL;
    p_mad_mx_adapter_specific_t          as             = NULL;
    p_mad_mx_connection_specific_t	 cs		= NULL;
    p_mad_mx_channel_specific_t	         chs		= NULL;
    mx_return_t			         return_code	= MX_SUCCESS;
    uint64_t			         r_nic_id	= 0;
    char				*r_hostname	= NULL;
    size_t				 r_hostname_len	= 0;
    mx_endpoint_addr_t                   *mx_endpoint_addr_tab;

    LOG_IN();
    a   = cnx->channel->adapter;
    as  = a->specific;
    cs	= cnx->specific;
    chs	= cnx->channel->specific;

    r_hostname_len	= strlen(ai->dir_node->name) + 2;
    r_hostname	        = TBX_MALLOC(r_hostname_len + 1);
    strcpy(r_hostname, ai->dir_node->name);

    {
        char *tmp = NULL;

        tmp = strchr(r_hostname, '.');
        if (tmp) {
            *tmp = '\0';
        }
    }

    strcat(r_hostname, ":0");

    {
        /* Extraction des id des 2 endpoints à partir du channel_parameter*/
        char *endpoint1_id     = NULL;
        char *end_endpoint1_id = NULL;
        char *endpoint2_id     = NULL;
        char *ptr              = NULL;

        endpoint1_id = ai->channel_parameter;
        end_endpoint1_id = strchr(endpoint1_id, ' ');
        if(end_endpoint1_id){
            *end_endpoint1_id = '\0';
            endpoint2_id = end_endpoint1_id + 1;
        }


        cs->remote_endpoint1_id = strtol(endpoint1_id, &ptr, 0);
        if (endpoint1_id == ptr)
            FAILURE("invalid channel parameter");

        cs->remote_endpoint2_id = strtol(endpoint2_id, &ptr, 0);
        if (endpoint1_id == ptr)
            FAILURE("invalid channel parameter");
    }

    mad_mx_lock();
    return_code	= mx_hostname_to_nic_id(r_hostname, &r_nic_id);
    mad_mx_unlock();
    mad_mx_check_return("mx_hostname_to_nic_id", return_code);



    mx_endpoint_addr_tab = tbx_htable_get(as->connexion_established_htable,
                                          r_hostname);

    if(!mx_endpoint_addr_tab){
        mx_endpoint_addr_tab = TBX_MALLOC(2 * sizeof(mx_endpoint_addr_t));
        mx_endpoint_addr_tab[0]   = mad_mx_connect_endpoint(&(as->endpoint1), r_nic_id, cs->remote_endpoint1_id);
        cs->remote_endpoint1_addr = mx_endpoint_addr_tab[0];

        mx_endpoint_addr_tab[1]   = mad_mx_connect_endpoint(&(as->endpoint2), r_nic_id, cs->remote_endpoint2_id);
        cs->remote_endpoint2_addr = mx_endpoint_addr_tab[1];

        tbx_htable_add(as->connexion_established_htable,
                       r_hostname,
                       mx_endpoint_addr_tab);
    } else {
        cs->remote_endpoint1_addr = mx_endpoint_addr_tab[0];
        cs->remote_endpoint2_addr = mx_endpoint_addr_tab[1];
    }

    TBX_FREE(r_hostname);
    r_hostname	= NULL;
    r_hostname_len	= 0;
    LOG_OUT();
}

void
mad_mx_accept(p_mad_connection_t   in,
              p_mad_adapter_info_t ai) {
    LOG_IN();
    mad_mx_accept_connect(in, ai);
    LOG_OUT();
}

void
mad_mx_connect(p_mad_connection_t   out,
               p_mad_adapter_info_t ai) {
    LOG_IN();
    mad_mx_accept_connect(out, ai);
    LOG_OUT();

}

void
mad_mx_disconnect(p_mad_connection_t c) {
    p_mad_mx_connection_specific_t cs = NULL;

    LOG_IN();
    cs = c->specific;
    LOG_OUT();
}

void
mad_mx_connection_exit(p_mad_connection_t in,
                       p_mad_connection_t out) {
    p_mad_mx_connection_specific_t cs = NULL;

    LOG_IN();
    cs = in->specific;
    TBX_FREE(cs);
    in->specific	= NULL;
    out->specific	= NULL;
    LOG_OUT();
}

void
mad_mx_channel_exit(p_mad_channel_t ch) {
    p_mad_mx_channel_specific_t	chs		= NULL;

    LOG_IN();
    chs = ch->specific;

    tbx_free(mad_mx_unexpected_key, chs->recv_msg->segment_ptr);
    TBX_FREE(chs->recv_msg);

    TBX_FREE(chs);
    ch->specific	= NULL;
    LOG_OUT();
}

void
mad_mx_adapter_exit(p_mad_adapter_t a) {
    p_mad_mx_adapter_specific_t as = NULL;
    mx_return_t			return_code	= MX_SUCCESS;

    LOG_IN();
    as		= a->specific;
    mad_mx_lock();
    return_code	= mx_close_endpoint(as->endpoint1);
    mad_mx_unlock();
    mad_mx_check_return("mx_close_endpoint 1", return_code);

    mad_mx_lock();
    return_code	= mx_close_endpoint(as->endpoint2);
    mad_mx_unlock();
    mad_mx_check_return("mx_close_endpoint 2", return_code);

    {
        p_tbx_slist_t list = tbx_htable_get_key_slist(as->connexion_established_htable);

        tbx_slist_ref_to_head(list);
        do{
            char * key = tbx_slist_ref_get(list);
            if(!key)
                break;
            tbx_htable_extract(as->connexion_established_htable, key);
        }while(tbx_slist_ref_forward(list));
        tbx_htable_free(as->connexion_established_htable);
    }

    TBX_FREE(as);
    a->specific	= NULL;
    a->parameter	= NULL;
    LOG_OUT();
}

void
mad_mx_driver_exit(p_mad_driver_t d) {
    p_mad_mx_driver_specific_t	ds		= NULL;
    mx_return_t			return_code	= MX_SUCCESS;

    LOG_IN();
    TRACE("Finalizing MX driver");
    ds	= d->specific;
    TBX_FREE(ds);
    d->specific	= NULL;

    mad_mx_lock();
    return_code	= mx_finalize();
    mad_mx_unlock();
    mad_mx_check_return("mx_finalize", return_code);
    LOG_OUT();
}

static void
mad_mx_track_free(p_mad_track_t	track){
    LOG_IN();
    TBX_FREE(track->pipelines);
    TBX_FREE(track);
    LOG_OUT();
}

static void
mad_mx_pipeline_free(p_mad_pipeline_t pipeline){
    LOG_IN();
    TBX_FREE(pipeline->on_goings);
    TBX_FREE(pipeline);
    LOG_OUT();
}

static void
mad_mx_ongoing_free(p_mad_on_going_t og){
    p_mad_mx_on_going_specific_t og_s	= NULL;

    LOG_IN();
    og_s = og->specific;
    TBX_FREE(og_s->mx_request);
    TBX_FREE(og_s);

    TBX_FREE(og);
    LOG_OUT();
}

static void
mad_mx_unexpected_iovec_free(p_mad_iovec_t mad_iovec){
    void *buffer	= NULL;

    LOG_IN();
    buffer = mad_iovec->data[0].iov_base;
    tbx_free(mad_mx_unexpected_key, buffer);
    mad_iovec_free(mad_iovec, tbx_false);
    LOG_OUT();
}

void
mad_mx_sending_track_exit(p_mad_adapter_t adapter){
    p_mad_pipeline_t	pipeline	= NULL;
    p_mad_track_t	track		= NULL;
    p_mad_on_going_t    on_going = NULL;
    LOG_IN();
    track = adapter->s_track;
    pipeline = track->pipelines[0];

    on_going = pipeline->on_goings[0];
    mad_mx_ongoing_free(on_going);

    on_going = pipeline->on_goings[1];
    mad_mx_ongoing_free(on_going);

    mad_mx_pipeline_free(pipeline);
    mad_mx_track_free(track);
    LOG_OUT();
}


void
mad_mx_reception_track_exit(p_mad_adapter_t adapter){
    p_mad_on_going_t		og		= NULL;
    p_mad_pipeline_t		pipeline	= NULL;
    p_mad_track_t		track		= NULL;

    LOG_IN();
    track = adapter->r_track;
    pipeline = track->pipelines[0];

    /* partie pour les pré-postés */
    og   = pipeline->on_goings[0];
    mad_mx_unexpected_iovec_free(og->mad_iovec);
    mad_mx_ongoing_free(og);

    og = pipeline->on_goings[1];
    mad_mx_unexpected_iovec_free(og->mad_iovec);
    mad_mx_ongoing_free(og);

    mad_mx_pipeline_free(pipeline);

    /* partie pour les gros */
    pipeline = track->pipelines[1];

    og   = pipeline->on_goings[0];
    mad_mx_ongoing_free(og);

    og   = pipeline->on_goings[1];
    mad_mx_ongoing_free(og);

    mad_mx_pipeline_free(pipeline);

    mad_mx_track_free(track);
    LOG_OUT();
}

void
mad_mx_new_message(p_mad_connection_t out){
    p_mad_mx_connection_specific_t	os	= NULL;

    LOG_IN();
    os	= out->specific;
    os->first_outgoing_packet_flag	= tbx_true;
    LOG_OUT();

}

void
mad_mx_finalize_message(p_mad_connection_t out){
    p_mad_mx_connection_specific_t os = NULL;
    p_mad_mx_adapter_specific_t as = NULL;
    p_mad_channel_t ch = NULL;
    mx_endpoint_t ep;
    mx_endpoint_addr_t                  dest_address;
    uint64_t                            match_info	       = 0;
    mx_return_t rc = MX_SUCCESS;

    LOG_IN();
    ch = out->channel;
    as = ch->adapter->specific;
    os = out->specific;
    ep = as->endpoint1;
    dest_address  = os->remote_endpoint1_addr;
    match_info    = ((uint64_t)ch->id) <<32;
    if (os->first_outgoing_packet_flag) {
        mx_request_t request;
        mx_status_t status;
        uint32_t result;

        rc = mx_isend(ep,
                      NULL, 0,
                      dest_address,
                      match_info,
                      NULL, &request);
        do {
            rc = mx_test(ep, &request, &status, &result);
        } while (rc == MX_SUCCESS && !result);

        mad_mx_check_return("mx_test", rc);
    }
    LOG_OUT();
}

p_mad_connection_t
mad_mx_receive_message(p_mad_channel_t ch) {
    p_mad_mx_channel_specific_t	chs		= NULL;
    p_mad_connection_t		in		= NULL;
    p_tbx_darray_t			in_darray = NULL;
    uint64_t 			match_info	= 0;
    mx_segment_t 		*s;
    p_mad_adapter_t             a  = NULL;
    p_mad_mx_adapter_specific_t as = NULL;

    mx_endpoint_t               ep;
    mx_request_t                request;
    mx_status_t                 status;
    uint32_t                    result;
    mx_return_t                 rc = MX_SUCCESS;

    LOG_IN();
    chs		= ch->specific;
    in_darray	= ch->in_connection_darray;
    match_info	= ((uint64_t)ch->id + 1) <<32;
    s = chs->recv_msg;
    a = ch->adapter;
    as = a->specific;
    ep = as->endpoint1;

    rc	= mx_irecv(ep,
                   s, 1,
                   match_info,
                   MX_MATCH_MASK_BC, NULL, &request);

    do {
        rc = mx_test(ep, &request, &status, &result);
    } while (rc == MX_SUCCESS && !result);
    mad_mx_check_return("mx_test", rc);

    match_info = status.match_info;


    in	= tbx_darray_get(in_darray, match_info & 0xFFFFFFFF);

    if(!in){
        FAILURE("mad_mx_receive_message : connection not found");
    }

    chs-> first_incoming_packet_flag		= tbx_true;
    TRACE("rm: first_packet_flag = true");
    LOG_OUT();

    return in;
}



/*************************************************************************************************/
/*************************************************************************************************/
/*************************************************************************************************/
tbx_bool_t
mad_mx_need_rdv(p_mad_iovec_t mad_iovec){
    LOG_IN();
    if(mad_iovec->length > MAD_MX_SMALL_MSG_SIZE)
        return tbx_true;
    LOG_OUT();

    return tbx_false;
}


unsigned int
mad_mx_limit_size(void){
    return MAD_MX_SMALL_MSG_SIZE;
}

void
mad_mx_create_areas(p_mad_iovec_t mad_iovec){
    unsigned int  *area_nb_seg = NULL;
    unsigned int total_nb_seg = 0;
    unsigned int offset = 0;
    unsigned int i = 0;
    unsigned int area_id = 0;
    unsigned int current_nb_areas = 0;
    LOG_IN();
    //DISP("-->mad_mx_create_areas");

    area_nb_seg = mad_iovec->area_nb_seg;
    total_nb_seg = mad_iovec->total_nb_seg;

    //DISP_VAL(" total_nb_seg ",  total_nb_seg );

    for(i = 0; i < total_nb_seg; i++){
        if(offset + mad_iovec->data[i].iov_len > MAD_MX_SMALL_MSG_SIZE){
            mad_iovec->area_nb_seg[area_id] = current_nb_areas;
            area_id++;
            current_nb_areas = 0;

            //DISP_VAL("area_id", area_id);
            //DISP_VAL("area_nb_seg[area_id -1]", mad_iovec->area_nb_seg[area_id -1]);

        } else {
            offset += mad_iovec->data[i].iov_len;
            current_nb_areas++;
        }
    }
    mad_iovec->area_nb_seg[area_id] = current_nb_areas;

    //DISP("<--mad_mx_create_areas");
    LOG_OUT();
}


static
void
fill_and_send(p_mad_adapter_t adapter,
              p_mad_pipeline_t pipeline){
    p_mad_iovec_t                mad_iovec    = NULL;
    p_mad_on_going_t            *on_goings        = NULL;
    p_mad_on_going_t             last_og       = NULL;
    unsigned int                 nb_elm     = -1;
    unsigned int                 i = 0;

    p_mad_channel_t    channel = NULL;
    p_mad_connection_t connection = NULL;

    int idx_first = -1;
    int idx_last = -1;

    LOG_IN();
    on_goings  = pipeline->on_goings;
    nb_elm     = pipeline->nb_elm;

    if(pipeline->index_first == -1 && pipeline->index_last == -1){

        if(adapter->sending_list->length){

            mad_iovec = tbx_slist_extract(adapter->sending_list);
            //*DISP_VAL("fill_and_send : EXTRACTION : adapter->sending_list -len", tbx_slist_get_length(adapter->sending_list));
        } else {
            //DISP_VAL("fill_and_send : driver->s_msg_slist -len", tbx_slist_get_length(adapter->driver->s_msg_slist));
            mad_iovec = mad_s_optimize(adapter, on_goings[i]);
            //DISP_VAL("fill_and_send : driver->s_msg_slist -len", tbx_slist_get_length(adapter->driver->s_msg_slist));
        }

        if(!mad_iovec)
            goto end;

        pipeline->index_first = 0;
        pipeline->index_last  = 0;

        idx_first = pipeline->index_first;


        on_goings[idx_first]->area_id  = 0;
        on_goings[idx_first]->mad_iovec = mad_iovec;


        channel = mad_iovec->channel;
        connection = tbx_darray_get(channel->out_connection_darray,
                                    mad_iovec->remote_rank);
        if(!connection){

            FAILURE("fill_and_send : connection not found");
        }

        on_goings[idx_first]->lnk = connection->link_array[0];

        mad_mx_send_iovec(on_goings[idx_first]->lnk,
                          pipeline,
                          on_goings[idx_first]);
    } else {
        idx_first = pipeline->index_first;
        idx_last = (idx_first + 1) %2;

        last_og = on_goings[idx_first];
        mad_iovec = last_og->mad_iovec;
        if(mad_iovec->area_nb_seg[last_og->area_id + 1] == 0){


            if(adapter->sending_list->length){

                mad_iovec = tbx_slist_extract(adapter->sending_list);
                //*DISP_VAL("fill_and_send : EXTRACTION : adapter->sending_list -len", tbx_slist_get_length(adapter->sending_list));
            } else {
                mad_iovec = mad_s_optimize(adapter, on_goings[idx_last]);
            }

            if(!mad_iovec)
                goto end;

            on_goings[idx_last]->area_id  = 0;
            on_goings[idx_last]->mad_iovec = mad_iovec;

            channel = mad_iovec->channel;
            connection = tbx_darray_get(channel->out_connection_darray,
                                    mad_iovec->remote_rank);
            if(!connection){
                FAILURE("2- fill_and_send : connection not found");
            }

            on_goings[idx_last]->lnk = connection->link_array[0];

            pipeline->index_last = idx_last;
            mad_mx_send_iovec(on_goings[idx_last]->lnk, pipeline, on_goings[idx_last]);
        } else {

            on_goings[idx_last]->area_id   = last_og->area_id + 1;
            on_goings[idx_last]->mad_iovec = mad_iovec;
            on_goings[idx_last]->lnk       = last_og->lnk;

            pipeline->index_last = idx_last;
            mad_mx_send_iovec(on_goings[idx_last]->lnk,
                              pipeline,
                              on_goings[idx_last]);
        }
    }

 end:
    LOG_OUT();
}


void
mad_mx_send_iovec(p_mad_link_t lnk,
                  p_mad_pipeline_t pipeline,
                  p_mad_on_going_t to_send){
    p_mad_adapter_t adapter = NULL;
    p_mad_mx_adapter_specific_t  as                     = NULL;
    p_mad_connection_t		 out		       = NULL;
    p_mad_mx_connection_specific_t os		       = NULL;
    p_mad_channel_t	ch	               = NULL;
    p_mad_mx_channel_specific_t	chs	               = NULL;

    uint64_t                    match_info	       = 0;
    mx_segment_t                *segment_vec            = NULL;
    unsigned int                nb_seg                 = 0;
    mx_endpoint_t               ep;
    mx_endpoint_addr_t          dest_address;
    mx_request_t                *p_request;

    unsigned int shift = 0;
    unsigned int i = 0;

    p_mad_mx_on_going_specific_t to_send_specific = NULL;
    mx_return_t rc = MX_SUCCESS;

    LOG_IN();
    out = lnk->connection;
    os  = out->specific;
    ch  = out->channel;
    chs = ch->specific;
    adapter = ch->adapter;
    as = adapter->specific;

    ep            = mad_mx_choose_endpoint(as, to_send->mad_iovec->length);
    dest_address  = mad_mx_choose_remote_endpoint(os, to_send->mad_iovec->length);

    nb_seg = to_send->mad_iovec->area_nb_seg[to_send->area_id];

    assert(nb_seg);

    //DISP_VAL("mad_mx_send_iovec - nb_seg", nb_seg);
    //mad_iovec_print(to_send->mad_iovec);

    for(i = 0; i < to_send->area_id; i++){
        shift += to_send->mad_iovec->area_nb_seg[i];
    }
    segment_vec   = (mx_segment_t *)&to_send->mad_iovec->data[shift];



    if (os->first_outgoing_packet_flag) {
        mx_request_t request;
        mx_status_t status;
        uint32_t result;

        match_info =  ((uint64_t)ch->id +1) << 32
            | ch->process_lrank;


        rc	= mx_isend(ep,
                           segment_vec, nb_seg,
                           dest_address,
                           match_info,
                           NULL, &request);
        do {
            rc = mx_test(ep, &request, &status, &result);
        } while (rc == MX_SUCCESS && !result);

        mad_mx_check_return("mx_test", rc);

        if(to_send->mad_iovec->area_nb_seg[to_send->area_id + 1] == 0){
            mad_iovec_s_check(out->packs_list,
                              to_send->mad_iovec->remote_rank,
                              MAD_MX_SMALL_MSG_SIZE,
                              to_send->mad_iovec);

            //mad_iovec_get_smaller(out->packs_list,
            //                      to_send->mad_iovec->channel_id,
            //                      to_send->mad_iovec->remote_rank,
            //                      to_send->mad_iovec->sequence,
            //                      MAD_MX_SMALL_MSG_SIZE);
           //*DISP_VAL("mad_mx_send_iovec : EXTRACTION : out->packs_list -len", tbx_slist_get_length(out->packs_list));

            if(to_send->mad_iovec->length < MAD_MX_SMALL_MSG_SIZE){
                mad_iovec_free(to_send->mad_iovec, tbx_true);
            } else {
                mad_iovec_free(to_send->mad_iovec, tbx_false);
            }
        }

        os->first_outgoing_packet_flag = tbx_false;

        if(pipeline->index_first == pipeline->index_last){
            pipeline->index_first = -1;
            pipeline->index_last = pipeline->index_first;
        } else {
            pipeline->index_first = pipeline->index_last;
        }
    } else {
        match_info    = ((uint64_t)ch->id) <<32;

        to_send_specific = to_send->specific;
        to_send_specific->endpoint = ep;
        p_request     = to_send_specific->mx_request;;

        mad_mx_isend(ep, dest_address,
                     segment_vec, nb_seg,
                     match_info, p_request);
    }
    LOG_OUT();
}


void
mad_mx_receive_iovec(p_mad_link_t lnk,
                     p_mad_on_going_t to_receive) {
    p_mad_connection_t		   in	          = NULL;
    p_mad_mx_connection_specific_t is	          = NULL;
    p_mad_channel_t		   ch	          = NULL;
    p_mad_mx_channel_specific_t    chs            = NULL;
    uint64_t                       match_info     = 0;
    p_mad_adapter_t a = NULL;
    p_mad_mx_adapter_specific_t                as              = NULL;
    mx_segment_t                  *segment_vec    = NULL;
    unsigned int                   nb_seg         = 0;
    unsigned int		   i	          = 0;
    mx_endpoint_t                  ep;
    mx_request_t                  *p_request;
    unsigned int decalage = 0;
    p_mad_mx_on_going_specific_t to_receive_specific = NULL;

    LOG_IN();
    in	                    = lnk->connection;
    is	                    = in->specific;
    ch	                    = in->channel;
    chs	                    = ch->specific;
    a                       = ch->adapter;
    as                      = a->specific;
    match_info  = ((uint64_t)ch->id) <<32;

    nb_seg = to_receive->mad_iovec->area_nb_seg[to_receive->area_id];
    if(nb_seg == 0)
        return;

     for(i= 0; i < to_receive->area_id; i++){
        decalage += to_receive->mad_iovec->area_nb_seg[i];
    }

     segment_vec   = (mx_segment_t *)&to_receive->mad_iovec->data[decalage];

     ep 	   = mad_mx_choose_endpoint(as, to_receive->mad_iovec->length);

     to_receive_specific = to_receive->specific;

     to_receive_specific->endpoint = ep;

     p_request = to_receive_specific->mx_request;
     mad_mx_irecv(ep, segment_vec, nb_seg, match_info, MX_MATCH_MASK_NONE, p_request);
     LOG_OUT();
}

static int
mad_mx_test(mx_endpoint_t ep, mx_request_t *request){
    mx_return_t  rc         = MX_SUCCESS;
    mx_status_t  status;
    uint32_t     result	= 0;
    unsigned int real_len = 0;

    LOG_IN();
    if(request == NULL)
        goto end;

    rc = mx_test(ep, request, &status, &result);
    mad_mx_check_return("mx_test failed", rc);

    if(rc == MX_SUCCESS && result){
        real_len = status.msg_length;
    }
 end:
    LOG_OUT();
    return real_len;
}


void
mad_mx_s_make_progress(p_mad_adapter_t adapter){
    p_mad_connection_t            connection = NULL;
    p_mad_track_t                 s_track = NULL;
    p_mad_pipeline_t              s_pipeline = NULL;
    p_mad_on_going_t              s_og     = NULL;
    p_mad_mx_on_going_specific_t  s_og_s     = NULL;
    unsigned int                  status      = -1;

    LOG_IN();
    s_track    = adapter->s_track;
    s_pipeline = s_track->pipelines[0];

    if(s_pipeline->index_first == -1){
        goto fill;
    }

    s_og = s_pipeline->on_goings[s_pipeline->index_first];
    s_og_s = s_og->specific;

    status = mad_mx_test(s_og_s->endpoint, s_og_s->mx_request);
    if(!status){
        return;
    }

    connection = s_og->lnk->connection;

    if(connection->packs_list){
        if(s_og->mad_iovec->area_nb_seg[s_og->area_id + 1] == 0){
            if(s_og->mad_iovec->length > MAD_MX_SMALL_MSG_SIZE){
                mad_iovec_get(connection->packs_list,
                              s_og->mad_iovec->channel_id,
                              s_og->mad_iovec->remote_rank,
                              s_og->mad_iovec->sequence);
                //*DISP_VAL("mad_mx_s_mkp : EXTRACTION : cnx->packs_list -len", tbx_slist_get_length(connection->packs_list));
            } else {
                mad_iovec_s_check(connection->packs_list,
                                  s_og->mad_iovec->remote_rank,
                                  MAD_MX_SMALL_MSG_SIZE,
                                  s_og->mad_iovec);
            }

            if(s_og->mad_iovec->length < MAD_MX_SMALL_MSG_SIZE){
                mad_iovec_free(s_og->mad_iovec, tbx_true);
            } else {
                mad_iovec_free(s_og->mad_iovec, tbx_false);
            }
        }
    }

    if(s_pipeline->index_first == s_pipeline->index_last){
        s_pipeline->index_first = -1;
        s_pipeline->index_last = s_pipeline->index_first;
    } else {
        s_pipeline->index_first = s_pipeline->index_last;
    }

 fill:
    fill_and_send(adapter, s_pipeline);
    LOG_OUT();
}


static
void
fill_and_post_reception(p_mad_adapter_t adapter,
                        p_mad_pipeline_t pipeline){
    p_mad_iovec_t     mad_iovec    = NULL;
    p_mad_on_going_t *on_goings        = NULL;
    p_mad_on_going_t  last_og       = NULL;
    unsigned int      nb_elm     = -1;
    unsigned int      i = 0;
    p_mad_mx_on_going_specific_t ogs = NULL;
    p_tbx_slist_t receiving_list = NULL;
    p_mad_mx_adapter_specific_t as = NULL;
    p_mad_channel_t    channel = NULL;
    p_mad_connection_t connection = NULL;
    int idx_first = -1;
    int idx_last = -1;

    LOG_IN();
    on_goings        = pipeline->on_goings;
    nb_elm           = pipeline->nb_elm;
    receiving_list   = adapter->receiving_list;
    as = adapter->specific;

    if(pipeline->index_first == -1 && pipeline->index_last == -1){
        if(!receiving_list->length)
            goto end;

        pipeline->index_first = 0;
        pipeline->index_last  = 0;

        idx_first = pipeline->index_first;

        mad_iovec = tbx_slist_extract(receiving_list);
       //*DISP_VAL("fill_and_post_reception : EXTRACTION : adapter->receiving_list -len", tbx_slist_get_length(adapter->receiving_list));

        on_goings[idx_first]->area_id   = 0;
        on_goings[idx_first]->mad_iovec = mad_iovec;

        ogs = on_goings[idx_first]->specific;
        ogs->endpoint = as->endpoint2;

        channel = mad_iovec->channel;
        connection = tbx_darray_get(channel->in_connection_darray,
                                    mad_iovec->remote_rank);
        on_goings[idx_first]->lnk = connection->link_array[0];

        mad_mx_receive_iovec(on_goings[idx_first]->lnk, on_goings[i]);
    }

    idx_first = pipeline->index_first;
    idx_last = idx_first + 1 %2;

    last_og = on_goings[idx_first];
    mad_iovec = last_og->mad_iovec;
    if(mad_iovec->area_nb_seg[last_og->area_id + 1] == 0){
        if(!receiving_list->length)
            goto end;

        mad_iovec = tbx_slist_extract(receiving_list);
       //*DISP_VAL("fill_and_post_reception : EXTRACTION : adapter->receiving_list -len", tbx_slist_get_length(adapter->receiving_list));

        on_goings[idx_last]->area_id   = 0;
        on_goings[idx_last]->mad_iovec = mad_iovec;

        ogs = on_goings[idx_last]->specific;
        ogs->endpoint = as->endpoint2;

        channel = mad_iovec->channel;
        connection = tbx_darray_get(channel->in_connection_darray,
                                    mad_iovec->remote_rank);
        on_goings[idx_last]->lnk = connection->link_array[0];

        pipeline->index_last = idx_last;
        mad_mx_receive_iovec(on_goings[idx_last]->lnk, on_goings[idx_last]);
    } else {
        on_goings[idx_last]->area_id   = last_og->area_id + 1;
        on_goings[idx_last]->mad_iovec = mad_iovec;

        ogs = on_goings[idx_last]->specific;
        ogs->endpoint = as->endpoint2;

        pipeline->index_last = idx_last;
        mad_mx_receive_iovec(on_goings[idx_last]->lnk, on_goings[idx_last]);
    }
 end:
    LOG_OUT();
}

static void
pre_post_irecv(p_mad_adapter_t adapter,
               p_mad_pipeline_t r_pipeline){
    p_mad_mx_adapter_specific_t as =NULL;
    p_mad_on_going_t	*on_goings	= NULL;
    unsigned int	 nb_elm		= -1;
    unsigned int	 index_last	= -1;
    mx_request_t	*p_request	=NULL;

    int			idx_first	= -1;
    int			idx_last	= -1;

    LOG_IN();
    as = adapter->specific;
    on_goings        = r_pipeline->on_goings;
    nb_elm           = r_pipeline->nb_elm;
    index_last       = r_pipeline->index_last;

    if(r_pipeline->index_first == -1 && r_pipeline->index_last == -1){
        p_mad_on_going_t		og  = NULL;
        p_mad_mx_on_going_specific_t	ogs = NULL;

        r_pipeline->index_first = 0;
        r_pipeline->index_last  = 0;

        og	= on_goings[r_pipeline->index_first];
        ogs	= og->specific;

        ogs->endpoint	= as->endpoint1;
        p_request	=  ogs->mx_request;

        mad_mx_irecv(ogs->endpoint,
                     (mx_segment_t *)og->mad_iovec->data,
                     1,
                     0,
                     MX_MATCH_MASK_NONE,
                     p_request);

        og->status = 1;
    }

    idx_first	= r_pipeline->index_first;
    idx_last	= (idx_first + 1) %2;

    if(!on_goings[idx_last]->status){
        p_mad_on_going_t 		og  = NULL;
        p_mad_mx_on_going_specific_t	ogs = NULL;

        og	= on_goings[idx_last];
        ogs	= og->specific;

        ogs->endpoint	= as->endpoint1;
        p_request	= ogs->mx_request;

        mad_mx_irecv(ogs->endpoint,
                     (mx_segment_t *)og->mad_iovec->data,
                     1,
                     0,
                     MX_MATCH_MASK_NONE,
                     p_request);

        og->status = 1;
    }

    r_pipeline->index_first = idx_first;
    r_pipeline->index_last  = idx_last ;
    LOG_OUT();
}

static
void
mad_mx_shift_pipeline(p_mad_pipeline_t r_pipeline){
    unsigned int t = 0;

    LOG_IN();
    assert(r_pipeline->index_first != r_pipeline->index_last);

    t		= r_pipeline->index_first;
    r_pipeline->index_first	= r_pipeline->index_last;
    r_pipeline->index_last	= t;

    r_pipeline->on_goings[r_pipeline->index_last]->status = 0;
    LOG_OUT();
}

void
mad_mx_r_make_progress(p_mad_adapter_t adapter, p_mad_channel_t channel){
    p_mad_mx_channel_specific_t  chs        = NULL;
    p_mad_track_t                r_track    = NULL;
    p_mad_pipeline_t             r_pipeline = NULL;
    p_mad_on_going_t             r_og       = NULL;
    p_mad_mx_on_going_specific_t r_og_s     = NULL;
    int                          status     = -1;


    LOG_IN();
    chs = channel->specific;
    r_track    = adapter->r_track;
    r_pipeline = r_track->pipelines[0];


    if(chs->first_incoming_packet_flag){
        TRACE("**********Traitement du recv_msg");
        mad_iovec_exploit_msg(adapter, chs->recv_msg->segment_ptr);
        chs->first_incoming_packet_flag = tbx_false;
    }

    /*** On traite les réceptions pre-postées ***/
    if(r_pipeline->index_first == -1 && r_pipeline->index_last == -1){
        pre_post_irecv(adapter, r_pipeline);
        goto large;
    }

    r_og  = r_pipeline->on_goings[r_pipeline->index_first];
    r_og_s = r_og->specific;

    status  = mad_mx_test(r_og_s->endpoint, r_og_s->mx_request);

    if(status){
        //DISP("RECEPTION d'un petit");

        mad_iovec_exploit_og(adapter, r_og);
        mad_mx_shift_pipeline(r_pipeline);
        pre_post_irecv(adapter, r_pipeline);
    }

 large:

    /*** On traite la partie qui reçoit les gros messages ***/
    r_pipeline = r_track->pipelines[1];

    if(r_pipeline->index_last == -1){
        fill_and_post_reception(adapter, r_pipeline);
        goto unexpected;
    }

    r_og   = r_pipeline->on_goings[r_pipeline->index_first];
    r_og_s = r_og->specific;

    status      = mad_mx_test(r_og_s->endpoint, r_og_s->mx_request);
    if(!status){
        goto unexpected;
    }


    //DISP("RECEPTION d'un msg de grande taille");

    if(channel->unpacks_list){
        if(r_og->mad_iovec->area_nb_seg[r_og->area_id + 1] == 0){
            mad_iovec_get(adapter->driver->r_msg_slist,
                          r_og->mad_iovec->channel_id,
                          r_og->mad_iovec->remote_rank,
                          r_og->mad_iovec->sequence);
           //*DISP_VAL("mad_mx_r_mkp : EXTRACTION : driver->r_msg_list -len", tbx_slist_get_length(adapter->driver->r_msg_slist));

            channel = r_og->lnk->connection->channel;
            mad_iovec_get(channel->unpacks_list,
                          r_og->mad_iovec->channel_id,
                          r_og->mad_iovec->remote_rank,
                          r_og->mad_iovec->sequence);
           //*DISP_VAL("mad_mx_r_mkp :  EXTRACTION : channel->unpacks_list -len", tbx_slist_get_length(channel->unpacks_list));

            mad_iovec_free(r_og->mad_iovec, tbx_false);
        }
    }


    if(r_pipeline->index_first == r_pipeline->index_last){
        r_pipeline->index_first = -1;
        r_pipeline->index_last = r_pipeline->index_first;
    } else {
        r_pipeline->index_first = r_pipeline->index_last;
    }
    fill_and_post_reception(adapter, r_pipeline);


 unexpected:
    /*** on traite les unexpected ***/
    {
        p_tbx_slist_t waiting_list = NULL;
        p_mad_iovec_t cur = NULL;
        p_mad_iovec_t searched = NULL;
        tbx_bool_t ref_forward = tbx_true;

        waiting_list = adapter->waiting_list;

        if(waiting_list->length){
            tbx_slist_ref_to_head(waiting_list);
            while(1){
                cur = tbx_slist_ref_get(waiting_list);
                if(cur == NULL)
                    break;


                //DISP_VAL("mad_mx_r_mkp : unexpected :  EXTRACTION : adapter->driver->r_msg_slist -len", tbx_slist_get_length(adapter->driver->r_msg_slist));

                searched = mad_iovec_get(adapter->driver->r_msg_slist,
                                         cur->channel_id,
                                         cur->remote_rank,
                                         cur->sequence);
                //DISP_VAL("mad_mx_r_mkp : unexpected :  EXTRACTION : adapter->driver->r_msg_slist -len", tbx_slist_get_length(adapter->driver->r_msg_slist));

                if(searched){
                    void * data           = cur->data[0].iov_base;
                    tbx_slist_index_t idx = 0;


                    mad_iovec_get(channel->unpacks_list,
                                  cur->channel_id,
                                  cur->remote_rank,
                                  cur->sequence);

                    memcpy(searched->data[0].iov_base, data, searched->data[0].iov_len);

                    idx = tbx_slist_ref_get_index(waiting_list);
                    ref_forward = tbx_slist_ref_forward(waiting_list);
                    tbx_slist_index_extract(waiting_list, idx);

                    //*DISP_VAL("r_mkp : u : EXTRACTION : waiting_list -len", tbx_slist_get_length(waiting_list));

                    mad_iovec_free(cur, tbx_false);
                    mad_iovec_free(searched, tbx_false);

                    if(!ref_forward)
                        break;
                } else {
                    if(!tbx_slist_ref_forward(waiting_list))
                        break;
                }
            }
        }
    }

    LOG_OUT();

}
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
