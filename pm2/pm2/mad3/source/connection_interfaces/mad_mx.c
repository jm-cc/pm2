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


#include "madeleine.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <myriexpress.h>

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
typedef struct s_mad_mx_driver_specific
{
        int dummy;
#ifdef MARCEL
        marcel_pollid_t     mx_pollid;
#endif /* MARCEL */
} mad_mx_driver_specific_t, *p_mad_mx_driver_specific_t;

typedef struct s_mad_mx_adapter_specific
{
        uint64_t	id;
} mad_mx_adapter_specific_t, *p_mad_mx_adapter_specific_t;

typedef struct s_mad_mx_channel_specific
{
        mx_endpoint_addr_t	 endpoint_addr;
        mx_endpoint_t		 endpoint;
        uint32_t		 endpoint_id;
        char			*first_packet;
        uint32_t		 first_packet_length;
} mad_mx_channel_specific_t, *p_mad_mx_channel_specific_t;

typedef struct s_mad_mx_connection_specific
{
        tbx_bool_t		first_incoming_packet_flag;
        tbx_bool_t		first_outgoing_packet_flag;
        uint32_t		remote_endpoint_id;
        mx_endpoint_addr_t	remote_endpoint_addr;
} mad_mx_connection_specific_t, *p_mad_mx_connection_specific_t;

typedef struct s_mad_mx_link_specific
{
        int dummy;
} mad_mx_link_specific_t, *p_mad_mx_link_specific_t;

#ifdef MARCEL
typedef struct s_mad_mx_poll_req {
        mx_endpoint_t 	endpoint;
        mx_request_t	*p_request;
        mx_return_t     *p_rc;
        mx_status_t	*p_status;
        uint32_t	*p_result;
} mad_mx_poll_req_t, *p_mad_mx_poll_req_t;
#endif /* MARCEL */

#define GATHER_SCATTER_THRESHOLD	32768
#define FIRST_PACKET_THRESHOLD		32768


/*
 * Malloc protection hooks
 * -----------------------
 *
 * Note: this driver is incompatible with any other driver also using
 * malloc hooks.
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

static
void
mad_mx_malloc_initialize_hook(void);


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

/* Entry point */
#if MAD_MX_MEMORY_CACHE
void (*__malloc_initialize_hook) (void) = mad_gm_malloc_initialize_hook;
#endif // MAD_MX_MEMORY_CACHE

/* Flag to prevent multiple hooking */
static
int mad_mx_malloc_hooked = 0;



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
        //DISP("<mx hook LOCK>");
        //DISP(".");
#endif /* MARCEL */
}

static
inline
void
__mad_mx_hook_unlock(void) {
#ifdef MARCEL
        //DISP("<mx hook UNLOCK>");
        //DISP(".");
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
void
mad_mx_malloc_initialize_hook(void) {
        mad_mx_malloc_hooked = 1;
        mad_mx_install_hooks();
}


static
inline
void
mad_mx_lock(void) {
#ifdef MARCEL
        marcel_poll_lock();
        mad_mx_remove_hooks();
        //DISP("<mx LOCK>");
        //DISP(".");
#endif /* MARCEL */
}

static
inline
void
mad_mx_unlock(void) {
#ifdef MARCEL
        //DISP("<mx UNLOCK>");
        //DISP(".");
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
                DISP("Panding receive was cancelled");
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
                DISP("%s failed with code %s = %d/0x%x", msg, mad_mx_return_code(return_code), return_code, return_code);
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

        // NICs count
        mad_mx_lock();
        return_code = mx_get_info(NULL, MX_NIC_COUNT, &nb_nic, sizeof(nb_nic));
        mad_mx_unlock();
        mad_mx_check_return("mad_mx_startup_info", return_code);
        DISP("NIC count: %d", nb_nic);

        // NIC ids
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

        // Number of native endpoints
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

static void
mad_mx_blocking_test(p_mad_channel_t	 ch,
                     mx_request_t	*p_request,
                     mx_status_t	*p_status){
        p_mad_mx_channel_specific_t	chs	= NULL;
        mx_endpoint_t			ep	= NULL;
        mx_status_t	 		status;
        uint32_t	 		result	= 0;
        mx_return_t 	 		rc 	= MX_SUCCESS;

        LOG_IN();
        chs	= ch->specific;
        ep	= chs->endpoint;

        if (!p_status) {
                p_status = &status;
        }

#ifdef MARCEL
 {
         p_mad_mx_driver_specific_t	ds	= NULL;
         marcel_pollid_t     		pollid	= 0;
         mad_mx_poll_req_t 		req	=
                 {
                         .endpoint 	=  ep,
                         .p_request	=  p_request,
                         .p_rc		= &rc,
                         .p_status	=  p_status,
                         .p_result	= &result
                 };

         ds	= ch->adapter->driver->specific;
         pollid	= ds->mx_pollid;

         marcel_poll(pollid, &req);
 }
#else /* MARCEL */
        do {
                rc = mx_test(chs->endpoint, p_request, p_status, &result);
        } while (rc == MX_SUCCESS && !result);
#endif /* MARCEL */

        mad_mx_check_return("mx_test", rc);
        LOG_OUT();
}

static void
mad_mx_irecv(p_mad_channel_t	 ch,
             mx_segment_t	*seg_list,
             uint32_t		 nb_seg,
             uint64_t		*p_match_info,
             uint64_t		 match_mask,
             mx_request_t	*p_request){
        p_mad_mx_channel_specific_t	chs	= NULL;
        mx_endpoint_t			ep	= NULL;
        mx_return_t     		rc	= MX_SUCCESS;

        LOG_IN();
        chs	= ch->specific;
        ep	= chs->endpoint;

        mad_mx_lock();
        rc	= mx_irecv(ep,
                      seg_list, nb_seg,
                      *p_match_info,
                      match_mask, NULL, p_request);
        mad_mx_unlock();
        mad_mx_check_return("mx_irecv", rc);
        LOG_OUT();
}

static void
mad_mx_isend(p_mad_connection_t	 out,
             mx_segment_t	*seg_list,
             uint32_t		 nb_seg,
             uint64_t		 match_info,
             mx_request_t 	*p_request){
        p_mad_mx_channel_specific_t	chs	= NULL;
        p_mad_mx_connection_specific_t	os	= NULL;
        mx_endpoint_t			ep	= NULL;
        mx_endpoint_addr_t		dest_address;
        mx_return_t  			rc	= MX_SUCCESS;

        LOG_IN();
        os 	= out->specific;
        chs	= out->channel->specific;
        ep 	= chs->endpoint;

        dest_address	= os->remote_endpoint_addr;

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

static void
mad_mx_send(p_mad_connection_t	 out,
            mx_segment_t	*seg_list,
            uint32_t		 nb_seg,
            uint64_t		 match_info){
        mx_request_t	request	= 0;

        LOG_IN();
        mad_mx_isend(out,
                     seg_list, nb_seg,
                     match_info, &request);

        mad_mx_blocking_test(out->channel, &request, NULL);
        LOG_OUT();
}

static uint32_t
mad_mx_recv(p_mad_channel_t	 ch,
            mx_segment_t	*seg_list,
            uint32_t		 nb_seg,
            uint64_t		*p_match_info,
            uint64_t		 match_mask){
        mx_request_t	request;
        mx_status_t	status;
        uint32_t	length	= 0;

        LOG_IN();
        mad_mx_irecv(ch, seg_list, nb_seg, p_match_info, match_mask, &request);
        mad_mx_blocking_test(ch, &request, &status);

        *p_match_info	= status.match_info;
        length		= status.msg_length;
        LOG_OUT();

        return length;
}

/*
 * Registration function
 * ---------------------
 */


void
mad_mx_register(p_mad_driver_t driver) {
        p_mad_driver_interface_t interface = NULL;

        LOG_IN();
        TRACE("Registering MX driver");
        interface = driver->interface;

        driver->connection_type  = mad_bidirectional_connection;
        driver->buffer_alignment = 32;
        driver->name             = tbx_strdup("mx");

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
        interface->connection_exit            = NULL;
        interface->channel_exit               = NULL;
        interface->adapter_exit               = NULL;
        interface->driver_exit                = NULL;
        interface->choice                     = NULL;
        interface->get_static_buffer          = NULL;
        interface->return_static_buffer       = NULL;
        interface->new_message                = mad_mx_new_message;
        interface->finalize_message           = NULL;
#ifdef MAD_MESSAGE_POLLING
        interface->poll_message               = mad_mx_poll_message;
#endif // MAD_MESSAGE_POLLING
        interface->receive_message            = mad_mx_receive_message;
        interface->message_received           = NULL;
        interface->send_buffer                = mad_mx_send_buffer;
        interface->receive_buffer             = mad_mx_receive_buffer;
        interface->send_buffer_group          = mad_mx_send_buffer_group;
        interface->receive_sub_buffer_group   = mad_mx_receive_sub_buffer_group;
        LOG_OUT();
}


void
mad_mx_driver_init(p_mad_driver_t d) {
        p_mad_mx_driver_specific_t	ds		= NULL;
        mx_return_t			return_code	= MX_SUCCESS;

        LOG_IN();
        TRACE("Initializing MX driver");
#ifdef MARCEL
        if (!mad_mx_malloc_hooked) {
                mad_mx_malloc_initialize_hook();
        }
#endif /* MARCEL */
        ds  		= TBX_MALLOC(sizeof(mad_mx_driver_specific_t));
#ifdef MARCEL
        ds->mx_pollid =
                marcel_pollid_create(mad_mx_marcel_group,
                                     mad_mx_marcel_poll,
                                     mad_mx_marcel_fast_poll,
                                     MAD_MX_POLLING_MODE);
#endif /* MARCEL */
        d->specific	= ds;

        mad_mx_lock();
        mx_set_error_handler(MX_ERRORS_RETURN);
        mad_mx_unlock();

        mad_mx_lock();
        return_code	= mx_init();
        mad_mx_unlock();
        mad_mx_check_return("mx_init", return_code);

        /* debug only */
        mad_mx_startup_info();

        LOG_OUT();
}

void
mad_mx_adapter_init(p_mad_adapter_t a) {
        p_mad_mx_adapter_specific_t as = NULL;

        LOG_IN();
        as		= TBX_MALLOC(sizeof(mad_mx_adapter_specific_t));

        if (strcmp(a->dir_adapter->name, "default")) {
                FAILURE("unsupported adapter");
        } else {
                as->id	= MX_ANY_NIC;
        }

        a->specific	= as;
        a->parameter	= tbx_strdup("-");
        LOG_OUT();
}

void
mad_mx_channel_init(p_mad_channel_t ch) {
        p_mad_mx_adapter_specific_t 	as = NULL;
        p_mad_mx_channel_specific_t	chs		= NULL;
        p_tbx_string_t			param_str	= NULL;
        mx_return_t			return_code	= MX_SUCCESS;
        uint32_t                        e_id     	= 1;
        const uint32_t			e_key		= 0xFFFFFFFF;
        mx_endpoint_t			e;
        mx_endpoint_addr_t		e_addr;

        LOG_IN();
        as =	ch->adapter->specific;

        mad_mx_lock();
        do {
                return_code	= mx_open_endpoint(as->id,
                                                   e_id,
                                                   e_key,
                                                   NULL,
                                                   0,
                                                   &e);
        } while (return_code == MX_BUSY && e_id++);
        mad_mx_unlock();
        mad_mx_check_return("mx_open_endpoint", return_code);

        mad_mx_lock();
        return_code	= mx_get_endpoint_addr(e, &e_addr);
        mad_mx_unlock();
        mad_mx_check_return("mx_get_endpoint", return_code);

        chs			= TBX_MALLOC(sizeof(mad_mx_channel_specific_t));
        chs->endpoint			= e;
        chs->endpoint_id		= e_id;
        chs->first_packet		= TBX_MALLOC(FIRST_PACKET_THRESHOLD);
        chs->first_packet_length	= 0;
        ch->specific			= chs;

        param_str	= tbx_string_init_to_int(e_id);
        ch->parameter	= tbx_string_to_cstring(param_str);
        tbx_string_free(param_str);
        param_str	= NULL;
        LOG_OUT();
}

void
mad_mx_connection_init(p_mad_connection_t in,
                       p_mad_connection_t out) {
        p_mad_mx_connection_specific_t	cs	= NULL;

        LOG_IN();
        cs			= TBX_MALLOC(sizeof(mad_mx_connection_specific_t));
        cs->remote_endpoint_id	= 0;
        cs->first_incoming_packet_flag	= tbx_false;
        cs->first_outgoing_packet_flag	= tbx_false;

        in->specific		= cs;
        in->nb_link		= 1;

        out->specific		= cs;
        out->nb_link		= 1;
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

static
void
mad_mx_accept_connect(p_mad_connection_t   cnx,
                      p_mad_adapter_info_t ai) {
        p_mad_mx_connection_specific_t	 cs		= NULL;
        p_mad_mx_channel_specific_t	 chs		= NULL;
        mx_return_t			 return_code	= MX_SUCCESS;
        uint32_t			 nb_r_nic_ids	= 1;
        uint64_t			 r_nic_id	= 0;
        const uint32_t			 e_key		= 0xFFFFFFFF;
        char				*r_hostname	= NULL;
        size_t				 r_hostname_len	= 0;
        mx_endpoint_addr_t 		 re_addr;

        LOG_IN();
        cs	= cnx->specific;
        chs	= cnx->channel->specific;

        r_hostname_len	= strlen(ai->dir_node->name) + 2;
        r_hostname	= TBX_MALLOC(r_hostname_len + 1);
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
                char *ptr = NULL;

                cs->remote_endpoint_id = strtol(ai->channel_parameter, &ptr, 0);
                if (ai->channel_parameter == ptr)
                        FAILURE("invalid channel parameter");
        }

        mad_mx_lock();
        return_code	= mx_hostname_to_nic_ids(r_hostname, &r_nic_id, &nb_r_nic_ids);
        mad_mx_unlock();
        mad_mx_check_return("mx_hostname_to_nic_ids", return_code);

        TBX_FREE(r_hostname);
        r_hostname	= NULL;
        r_hostname_len	= 0;

        mad_mx_lock();
        return_code	= mx_connect(chs->endpoint,
                                     r_nic_id,
                                     cs->remote_endpoint_id,
                                     e_key,
                                     MX_INFINITE,
                                     &re_addr);
        mad_mx_unlock();
        mad_mx_check_return("mx_connect", return_code);
        cs->remote_endpoint_addr	= re_addr;
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
connection_exit(p_mad_connection_t in,
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
        mx_return_t			return_code	= MX_SUCCESS;

        LOG_IN();
        chs = ch->specific;
        mad_mx_lock();
        return_code	= mx_close_endpoint(chs->endpoint);
        mad_mx_unlock();
        mad_mx_check_return("mx_close_endpoint", return_code);
        TBX_FREE(chs->first_packet);
        chs->first_packet = NULL;

        TBX_FREE(chs);
        ch->specific	= NULL;
        LOG_OUT();
}

void
mad_mx_adapter_exit(p_mad_adapter_t a) {
        p_mad_mx_adapter_specific_t as = NULL;

        LOG_IN();
        as		= a->specific;
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

void
mad_mx_new_message(p_mad_connection_t out){
        p_mad_mx_connection_specific_t	os	= NULL;

        LOG_IN();
        os	= out->specific;
        os->first_outgoing_packet_flag	= tbx_true;
        TRACE("nm: first_packet_flag = true");
        LOG_OUT();
}

p_mad_connection_t
mad_mx_receive_message(p_mad_channel_t ch) {
        p_mad_mx_channel_specific_t	chs		= NULL;
        p_mad_connection_t		in		= NULL;
        p_mad_mx_connection_specific_t	is		= NULL;
        p_tbx_darray_t			in_darray	= NULL;
        uint64_t 			match_info	= 0;
        mx_segment_t 			s;

        LOG_IN();
        chs		= ch->specific;
        in_darray	= ch->in_connection_darray;
        match_info	= (uint64_t)ch->id <<32;

        s.segment_ptr	= chs->first_packet;
        s.segment_length	= FIRST_PACKET_THRESHOLD;

        chs->first_packet_length = mad_mx_recv(ch, &s, 1, &match_info, MX_MATCH_MASK_BC);
        TRACE("rm: first_packet_length = %x", chs->first_packet_length);

        in	= tbx_darray_get(in_darray, match_info & 0xFFFFFFFF);
        is	= in->specific;

        is-> first_incoming_packet_flag		= tbx_true;
        TRACE("rm: first_packet_flag = true");
        LOG_OUT();

        return in;
}

void
mad_mx_send_buffer(p_mad_link_t     lnk,
                   p_mad_buffer_t   b) {
        p_mad_connection_t		out		= NULL;
        p_mad_mx_connection_specific_t	os		= NULL;
        uint64_t			match_info	= 0;
        mx_segment_t 			s;

        LOG_IN();
        out	= lnk->connection;
        os	= out->specific;

        match_info	= (uint64_t)out->channel->id <<32 | out->channel->process_lrank;

        if (os->first_outgoing_packet_flag) {
                uint32_t length = 0;

                os->first_outgoing_packet_flag = tbx_false;

                length	= tbx_min(b->bytes_written - b->bytes_read,
                                  FIRST_PACKET_THRESHOLD);

                s.segment_ptr		= b->buffer + b->bytes_read;
                s.segment_length	= length;

                TRACE("sb: first packet length: %x", length);
                mad_mx_send(out, &s, 1, match_info);

                b->bytes_read += length;

                if (!mad_more_data(b))
                        goto no_more_data;
        }

        s.segment_ptr		= b->buffer + b->bytes_read;
        s.segment_length	= b->bytes_written - b->bytes_read;

        b->bytes_read += s.segment_length;

        TRACE("sb: packet length: %x", s.segment_length);
        mad_mx_send(out, &s, 1, match_info);

 no_more_data:
        ;
        LOG_OUT();
}

void
mad_mx_receive_buffer(p_mad_link_t    lnk,
                      p_mad_buffer_t *_buffer) {
        p_mad_buffer_t			b	  	= *_buffer;
        p_mad_connection_t		in		= NULL;
        p_mad_mx_connection_specific_t	is		= NULL;
        p_mad_channel_t			ch		= NULL;
        p_mad_mx_channel_specific_t	chs		= NULL;
        uint64_t			match_info	= 0;
        mx_segment_t			s;

        LOG_IN();
        in	= lnk->connection;
        is	= in->specific;
        ch	= in->channel;
        chs	= ch->specific;

        if (is->first_incoming_packet_flag) {
                void *          data_ptr	= NULL;
                uint32_t	data_length	= 0;

                is->first_incoming_packet_flag = tbx_false;

                data_ptr	= b->buffer + b->bytes_written;
                data_length	= tbx_min(b->length - b->bytes_written, FIRST_PACKET_THRESHOLD);


                TRACE("rb: first packet length: %x", chs->first_packet_length);
                TRACE("rb: expected first packet length: %x", data_length);

                if (chs->first_packet_length != data_length)
                        FAILURE("invalid first packet length");

                memcpy(data_ptr, chs->first_packet, data_length);
                b->bytes_written	+= data_length;

                if (mad_buffer_full(b))
                        goto no_more_data;
        }

        s.segment_ptr		= b->buffer + b->bytes_written;
        s.segment_length	= b->length - b->bytes_written;

        TRACE("rb: segment length: %x", s.segment_length);
        b->bytes_written += s.segment_length;

        match_info = (uint64_t)in->channel->id <<32 | in->remote_rank;

        mad_mx_recv(ch, &s, 1, &match_info, MX_MATCH_MASK_NONE);

 no_more_data:
        ;
        LOG_OUT();
}

void
mad_mx_send_buffer_group_1(p_mad_link_t         lnk,
                           p_mad_buffer_group_t buffer_group) {
        LOG_IN();
        if (!tbx_empty_list(&(buffer_group->buffer_list))) {
                tbx_list_reference_t ref;

                tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
                do {
                        p_mad_buffer_t b = NULL;

                        b = tbx_get_list_reference_object(&ref);
                        mad_mx_send_buffer(lnk, b);
                } while(tbx_forward_list_reference(&ref));
        }
        LOG_OUT();
}

void
mad_mx_receive_sub_buffer_group_1(p_mad_link_t         lnk,
                                  p_mad_buffer_group_t buffer_group) {
        LOG_IN();
        if (!tbx_empty_list(&(buffer_group->buffer_list))) {
                tbx_list_reference_t ref;

                tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
                do {
                        p_mad_buffer_t b = NULL;

                        b = tbx_get_list_reference_object(&ref);
                        mad_mx_receive_buffer(lnk, &b);
                } while(tbx_forward_list_reference(&ref));
        }
        LOG_OUT();
}




void
mad_mx_send_buffer_group_2(p_mad_link_t         lnk,
                           p_mad_buffer_group_t buffer_group) {
        p_mad_connection_t		out		= NULL;
        p_mad_mx_connection_specific_t	os		= NULL;
        p_mad_mx_channel_specific_t	chs		= NULL;
        uint64_t                        match_info	= 0;
        p_tbx_list_t 			buffer_list	= NULL;

        LOG_IN();
        out	= lnk->connection;
        os	= out->specific;
        chs	= out->channel->specific;
        match_info = (uint64_t)out->channel->id <<32 | out->channel->process_lrank;

        buffer_list = &buffer_group->buffer_list;

        if (!tbx_empty_list(buffer_list)) {
                mx_segment_t		seg_list[buffer_list->length];
                tbx_list_reference_t	ref;
                uint32_t		offset	= 0;
                unsigned int		i	= 0;

                TRACE_VAL("sbg: group length", buffer_list->length);

                tbx_list_reference_init(&ref, &(buffer_group->buffer_list));

                do {
                        p_mad_buffer_t	b		= NULL;
                        void *          data_ptr	= NULL;
                        uint32_t	data_length	= 0;

                        b = tbx_get_list_reference_object(&ref);

                        if (os->first_outgoing_packet_flag){
                                mx_segment_t s;

                                os->first_outgoing_packet_flag = 0;

                                // on envoie une premiere partie en guise de new_msg
                                data_ptr	= b->buffer + b->bytes_read;
                                data_length	= tbx_min(b->bytes_written - b->bytes_read, FIRST_PACKET_THRESHOLD);

                                TRACE("sbg: first packet length: %x", data_length);
                                b->bytes_read		+= data_length;

                                s.segment_ptr		 = data_ptr;
                                s.segment_length	 = data_length;

                                mad_mx_send(out, &s, 1, match_info);

                                if(!mad_more_data(b))
                                        goto no_more_data;
                        }

                        TRACE("sbg: offset: %x", offset);
                        data_ptr	 = b->buffer        + b->bytes_read;
                        data_length	 =
                                i?tbx_min(b->bytes_written - b->bytes_read,
                                          GATHER_SCATTER_THRESHOLD - offset)
                                :b->bytes_written - b->bytes_read;

                        TRACE("sbg: segment length: %x", data_length);

                        b->bytes_read	+= data_length;

                        seg_list[i].segment_ptr		= data_ptr;
                        seg_list[i].segment_length	= data_length;

                        i++;
                        offset += data_length;

                        if (offset >= GATHER_SCATTER_THRESHOLD){
                                TRACE("sbg: seg_list full, flushing");
                                mad_mx_send(out, seg_list, i, match_info);

                                if (mad_more_data(b)) {
                                        mx_segment_t s;

                                        data_ptr	+= data_length;
                                        data_length	 = b->bytes_written - b->bytes_read;

                                        b->bytes_read	+= data_length;

                                        TRACE("sbg: sending large block: %x", data_length);

                                        s.segment_ptr		= data_ptr;
                                        s.segment_length	= data_length;

                                        mad_mx_send(out, &s, 1, match_info);
                                }

                                i = 0;
                                offset = 0;
                        }

                no_more_data:
                        ;

                        TRACE("sbg: while");

                } while (tbx_forward_list_reference(&ref));

                if (offset) {
                        TRACE("sbg: remaining offset: %x", offset);
                        mad_mx_send(out, seg_list, i, match_info);
                }
        }
        LOG_OUT();
}

void
mad_mx_receive_sub_buffer_group_2(p_mad_link_t         lnk,
                                  p_mad_buffer_group_t buffer_group) {
        p_mad_connection_t		in	    = NULL;
        p_mad_mx_connection_specific_t	is	    = NULL;
        p_mad_channel_t			ch	    = NULL;
        p_mad_mx_channel_specific_t	chs	    = NULL;
        uint64_t                        match_info  = 0;
        uint64_t                        mi	    = 0;
        p_tbx_list_t                    buffer_list = NULL;

        LOG_IN();
        in	= lnk->connection;
        is	= in->specific;
        ch	= in->channel;
        chs	= ch->specific;
        match_info = (uint64_t)in->channel->id <<32 | in->remote_rank;

        buffer_list = &buffer_group->buffer_list;

        if (!tbx_empty_list(buffer_list)) {
                mx_segment_t		seg_list[buffer_list->length];
                mx_request_t		req_list[buffer_list->length*2];
                tbx_list_reference_t	ref;
                uint32_t		offset	= 0;
                unsigned int		i	= 0;
                unsigned int		j	= 0;
                unsigned int		k	= 0;

                TRACE_VAL("rbg: group length", buffer_list->length);
                tbx_list_reference_init(&ref, &(buffer_group->buffer_list));

                do {
                        p_mad_buffer_t b = NULL;
                        void *          data_ptr	= NULL;
                        uint32_t	data_length	= 0;

                        b = tbx_get_list_reference_object(&ref);

                        if (is->first_incoming_packet_flag){


                                is->first_incoming_packet_flag = 0;

                                //on a deja recu une premiere partie dans receive_message
                                data_ptr	= b->buffer + b->bytes_written;
                                data_length	= tbx_min(b->length - b->bytes_written, FIRST_PACKET_THRESHOLD);

                                TRACE("rbg: first packet length: %x", chs->first_packet_length);
                                TRACE("rbg: expected first packet length: %x", data_length);

                                if (chs->first_packet_length != data_length)
                                        FAILURE("invalid first packet length");

                                memcpy(data_ptr, chs->first_packet, data_length);

                                b->bytes_written += data_length;

                                if (mad_buffer_full(b))
                                        goto no_more_data;
                        }

                        TRACE("rbg: offset: %x", offset);
                        data_ptr	= b->buffer + b->bytes_written;
                        data_length	=
                                i?tbx_min(b->length - b->bytes_written,
                                          GATHER_SCATTER_THRESHOLD - offset)
                                :b->length - b->bytes_written;

                        TRACE("rbg: segment length: %x", data_length);

                        b->bytes_written	+= data_length;

                        seg_list[i].segment_ptr		= data_ptr;
                        seg_list[i].segment_length	= data_length;

                        i++;
                        offset += data_length;

                        if (offset >= GATHER_SCATTER_THRESHOLD){
                                TRACE("rbg: seg_list full, flushing");
                                mi	= match_info;
                                mad_mx_irecv(ch, seg_list, i, &mi, MX_MATCH_MASK_NONE, req_list+j);
                                j++;

                                if (!mad_buffer_full(b)) {
                                        mx_segment_t s;

                                        data_ptr	+= data_length;
                                        data_length	 = b->length - b->bytes_written;

                                        b->bytes_written	+= data_length;

                                        TRACE("rbg: receiving large block: %x", data_length);
                                        s.segment_ptr		= data_ptr;
                                        s.segment_length	= data_length;

                                        mi	= match_info;
                                        mad_mx_irecv(ch, &s, 1, &mi, MX_MATCH_MASK_NONE, req_list+j);
                                        j++;
                                }

                                i = 0;
                                offset = 0;
                        }

                no_more_data:
                        ;

                        TRACE("rbg: while");

                } while(tbx_forward_list_reference(&ref));

                if (offset) {
                        TRACE("rbg: remaining offset: %x", offset);
                        mi 	= match_info;
                        mad_mx_irecv(ch, seg_list, i, &mi, MX_MATCH_MASK_NONE, req_list+j);
                        j++;
                }

                for(k = 0; k < j; k++){
                        TRACE("rbg: %d/%d...", k+1, j);
                        mad_mx_blocking_test(ch, req_list+k, NULL);
                        TRACE("rbg: %d/%d.", k+1, j);
                }
        }
        LOG_OUT();
}

void
mad_mx_send_buffer_group(p_mad_link_t         lnk,
                         p_mad_buffer_group_t buffer_group) {
        LOG_IN();
        mad_mx_send_buffer_group_2(lnk, buffer_group);
        LOG_OUT();
}

void
mad_mx_receive_sub_buffer_group(p_mad_link_t         lnk,
                                tbx_bool_t           first_sub_group
                                __attribute__ ((unused)),
                                p_mad_buffer_group_t buffer_group) {
        LOG_IN();
        mad_mx_receive_sub_buffer_group_2(lnk, buffer_group);
        LOG_OUT();
}
