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
        char			*buffer;
        uint32_t		 length;
} mad_mx_channel_specific_t, *p_mad_mx_channel_specific_t;

typedef struct s_mad_mx_connection_specific
{
        uint32_t		remote_endpoint_id;
        mx_endpoint_addr_t	remote_endpoint_addr;


        uint32_t               begin_packing;

} mad_mx_connection_specific_t, *p_mad_mx_connection_specific_t;

typedef struct s_mad_mx_link_specific
{
        int dummy;
} mad_mx_link_specific_t, *p_mad_mx_link_specific_t;

#ifdef MARCEL
typedef struct s_mad_mx_poll_req {
        mx_endpoint_t 	endpoint;
        mx_request_t	request;
        mx_return_t     test_return;
        mx_status_t	status;
        uint32_t	result;
} mad_mx_poll_req_t, *p_mad_mx_poll_req_t;
#endif /* MARCEL */

#define THRESHOLD 32768
#define FIRST_MSG_SIZE 32768

/*
 * static functions
 * ----------------
 */
static
inline
void
mad_mx_lock(void) {
#ifdef MARCEL
        marcel_poll_lock();
#endif /* MARCEL */
}

static
inline
void
mad_mx_unlock(void) {
#ifdef MARCEL
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
                //case MX_STATUS_BAD_SESSION:
                //DISP("Bad session (no mx_connect done?)"
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
        LOG_IN();
        rq->test_return = mx_test(rq->endpoint, &(rq->request), &(rq->status), &(rq->result));
        LOG_OUT();

        return !(rq->test_return == MX_SUCCESS && !rq->result);
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

//static void
static mx_request_t
mad_mx_send(mx_endpoint_t ep, mx_segment_t *seg_list,
            uint32_t nb_seg, mx_endpoint_addr_t dest_address, uint64_t match_info){
        mx_request_t request	= 0;
        //mx_status_t  status;
        //uint32_t     result	= 0;
        mx_return_t  rc         = MX_SUCCESS;

        //DISP("mad_mx_send-->");
        rc = mx_isend(ep,
                      seg_list, nb_seg,
                      dest_address,
                      match_info,
                      NULL, &request);
        mad_mx_check_return("mx_isend", rc);

        return request;

        //do {
        //        rc = mx_test(ep, &request, &status, &result);
        //} while (rc == MX_SUCCESS && !result);
        //mad_mx_check_status("mx_wait", rc);
        //DISP("mad_mx_send<--");
}


//static void
static mx_request_t
mad_mx_recv(mx_endpoint_t ep, mx_segment_t *seg_list,
            uint32_t nb_seg,  uint64_t match_info){
        mx_request_t	request	= 0;
        // mx_status_t	status;
        //uint32_t	result	= 0;
        mx_return_t     rc = MX_SUCCESS;

        //DISP("mad_mx_recv-->");
        rc = mx_irecv(ep,
                      seg_list, nb_seg,
                      match_info,
                      MX_MATCH_MASK_NONE, NULL, &request);
        mad_mx_check_return("mx_irecv", rc);

        return request;


//        do {
//                rc = mx_test(ep, &request, &status, &result);
//        } while (rc == MX_SUCCESS && !result);
//        mad_mx_check_status("mx_wait", rc);
        //DISP("mad_mx_recv<--");
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
        ds  		= TBX_MALLOC(sizeof(mad_mx_driver_specific_t));
#ifdef MARCEL
        ds->mx_pollid =
                marcel_pollid_create(mad_mx_marcel_group,
                                     mad_mx_marcel_poll,
                                     mad_mx_marcel_fast_poll,
                                     MAD_MX_POLLING_MODE);
#endif /* MARCEL */
        d->specific	= ds;

        /* needed? */
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
                //DISP("e_id = %u, return_code = %d", e_id, (int)return_code);
        } while (return_code == MX_BUSY && e_id++);
        mad_mx_unlock();
        mad_mx_check_return("mx_open_endpoint", return_code);

        return_code	= mx_get_endpoint_addr(e, &e_addr);
        mad_mx_check_return("mx_get_endpoint", return_code);


        chs			= TBX_MALLOC(sizeof(mad_mx_channel_specific_t));
        chs->endpoint		= e;
        chs->endpoint_id	= e_id;
        chs->buffer		= TBX_MALLOC(FIRST_MSG_SIZE);
        ch->specific		= chs;
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
        p_mad_channel_t			ch	= NULL;

        LOG_IN();
        ch = in->channel;

        cs			= TBX_MALLOC(sizeof(mad_mx_connection_specific_t));
        cs->remote_endpoint_id	= 0;

        // mettre un type booleen : tbx_bool...
        cs->begin_packing = 0;


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
        //lnk->link_mode   = mad_link_mode_buffer;
        lnk->buffer_mode = mad_buffer_mode_dynamic;
        lnk->group_mode  = mad_group_mode_split;
        LOG_OUT();
}

void
mad_mx_accept(p_mad_connection_t   in,
              p_mad_adapter_info_t ai TBX_UNUSED) {
        p_mad_mx_connection_specific_t	 is		= NULL;
        p_mad_mx_channel_specific_t	 chs		= NULL;
        mx_return_t			 return_code	= MX_SUCCESS;
        uint32_t			 nb_r_nic_ids	= 1;
        uint64_t			 r_nic_id	= 0;
        const uint32_t			 e_key		= 0xFFFFFFFF;
        char				*r_hostname	= NULL;
        size_t				 r_hostname_len	= 0;
        mx_endpoint_addr_t 		 re_addr;

        LOG_IN();
        is	= in->specific;
        chs	= in->channel->specific;

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

                is->remote_endpoint_id = strtol(ai->channel_parameter, &ptr, 0);
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
                                     is->remote_endpoint_id,
                                     e_key,
                                     MX_INFINITE,
                                     &re_addr);
        mad_mx_unlock();
        mad_mx_check_return("mx_connect", return_code);
        is->remote_endpoint_addr	= re_addr;
        LOG_OUT();
}

void
mad_mx_connect(p_mad_connection_t   out,
               p_mad_adapter_info_t ai) {
        p_mad_mx_connection_specific_t	 os		= NULL;
        p_mad_mx_channel_specific_t	 chs		= NULL;
        mx_return_t			 return_code	= MX_SUCCESS;
        uint32_t			 nb_r_nic_ids	= 1;
        uint64_t			 r_nic_id	= 0;
        const uint32_t			 e_key		= 0xFFFFFFFF;
        char				*r_hostname	= NULL;
        size_t				 r_hostname_len	= 0;
        mx_endpoint_addr_t 		 re_addr;

        LOG_IN();
        os	= out->specific;
        chs	= out->channel->specific;

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

                os->remote_endpoint_id = strtol(ai->channel_parameter, &ptr, 0);
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
                                     os->remote_endpoint_id,
                                     e_key,
                                     MX_INFINITE,
                                     &re_addr);
        mad_mx_unlock();
        mad_mx_check_return("mx_connect", return_code);
        os->remote_endpoint_addr	= re_addr;
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


#ifdef MAD_MESSAGE_POLLING
p_mad_connection_t
mad_mx_poll_message(p_mad_channel_t channel)
{
        FAILURE("unsupported");
}
#endif // MAD_MESSAGE_POLLING

void
mad_mx_new_message(p_mad_connection_t out){

//        p_mad_mx_connection_specific_t	os		= NULL;
//        p_mad_mx_channel_specific_t	chs		= NULL;
//        mx_endpoint_addr_t		dest_address;
//        uint64_t			match_info	= 0;
//
//        LOG_IN();
//        os		= out->specific;
//        chs		= out->channel->specific;
//        dest_address	= os->remote_endpoint_addr;
//
//        match_info = (uint64_t)out->channel->id <<32 | out->channel->process_lrank;
//
//#ifdef MARCEL
//        {
//                p_mad_mx_driver_specific_t	ds	= NULL;
//                mx_return_t		rc	= MX_SUCCESS;
//                marcel_pollid_t		pollid	= 0;
//                mad_mx_poll_req_t	req	= {
//                        .endpoint	= NULL,
//                        .request	= 0,
//                        .test_return	= MX_SUCCESS,
//                        .result		= 0
//                };
//
//                ds		= out->channel->adapter->driver->specific;
//                pollid		= ds->mx_pollid;
//                req.endpoint	= chs->endpoint;
//                mad_mx_lock();
//                rc		= mx_isend(req.endpoint,
//                                           NULL, 0,
//                                           dest_address,
//                                           match_info,
//                                           NULL, &(req.request));
//                mad_mx_unlock();
//                mad_mx_check_status("mx_isend", rc);
//
//                marcel_poll(pollid, &req);
//                mad_mx_check_status("mx_test", req.test_return);
//        }
//#else /* MARCEL */
//        {
//                mx_endpoint_t	ep	= NULL;
//                mx_request_t	request = 0;
//                mx_status_t	status;
//                uint32_t	result	= 0;
//                mx_return_t	rc	= MX_SUCCESS;
//
//                ep	= chs->endpoint;
//                rc	= mx_isend(ep,
//                                   NULL, 0,
//                                   dest_address,
//                                   match_info,
//                                   NULL, &request);
//
//                mad_mx_check_status("mx_isend", rc);
//
//                do {
//                        rc = mx_test(ep, &request, &status, &result);
//                } while (rc == MX_SUCCESS && !result);
//
//                mad_mx_check_status("mx_test", rc);
//        }
//#endif /* MARCEL */
//        LOG_OUT();

        p_mad_mx_connection_specific_t	os	= NULL;
        p_mad_mx_channel_specific_t	chs		= NULL;
        mx_endpoint_addr_t		dest_address;
        uint64_t			match_info	= 0;

        LOG_IN();

        os		= out->specific;
        chs		= out->channel->specific;
        dest_address	= os->remote_endpoint_addr;

        match_info = (uint64_t)out->channel->id <<32 | out->channel->process_lrank;
#ifdef MARCEL
        {
                p_mad_mx_driver_specific_t	ds	= NULL;
                mx_return_t		rc	= MX_SUCCESS;
                marcel_pollid_t		pollid	= 0;
                mad_mx_poll_req_t	req	= {
                        .endpoint	= NULL,
                        .request	= 0,
                        .test_return	= MX_SUCCESS,
                        .result		= 0
                };

                ds		= out->channel->adapter->driver->specific;
                pollid		= ds->mx_pollid;
                req.endpoint	= chs->endpoint;
                mad_mx_lock();
                rc		= mx_isend(req.endpoint,
                                           NULL, 0,
                                           dest_address,
                                           match_info,
                                           NULL, &(req.request));
                mad_mx_unlock();
                mad_mx_check_return("mx_isend", rc);
                marcel_poll(pollid, &req);

                mad_mx_check_return("mx_test", req.test_return);
        }
#else /* MARCEL */
        {
                mx_endpoint_t	ep	= NULL;
                mx_request_t	request = 0;
                mx_status_t	status;
                uint32_t	result	= 0;
                mx_return_t	rc	= MX_SUCCESS;

                ep	= chs->endpoint;
                rc	= mx_isend(ep,
                                   NULL, 0,
                                   dest_address,
                                   match_info,
                                   NULL, &request);
        os =  out->specific;
        os->begin_packing = 1;

                mad_mx_check_return("mx_isend", rc);

                do {
                        rc = mx_test(ep, &request, &status, &result);
                } while (rc == MX_SUCCESS && !result);

                mad_mx_check_return("mx_test", rc);
        }
#endif /* MARCEL */
        LOG_OUT();
}





p_mad_connection_t
mad_mx_receive_message(p_mad_channel_t ch) {
        //p_mad_mx_channel_specific_t	chs		= NULL;
        //p_tbx_darray_t			in_darray	= NULL;
        //p_mad_connection_t		in		= NULL;
        //uint64_t 			match_info	= 0;
        //
        //LOG_IN();
        //chs		= ch->specific;
        //in_darray	= ch->in_connection_darray;
        //match_info	= (uint64_t)ch->id <<32;
        //
#ifdef MARCEL
        //{
        //        p_mad_mx_driver_specific_t	ds	= NULL;
        //        mx_return_t		rc	= MX_SUCCESS;
        //        marcel_pollid_t		pollid	= 0;
        //        mad_mx_poll_req_t	req	= {
        //                .endpoint	= NULL,
        //                .request	= 0,
        //                .test_return	= MX_SUCCESS,
        //                .result		= 0
        //        };
        //
        //        ds		= ch->adapter->driver->specific;
        //        pollid		= ds->mx_pollid;
        //        req.endpoint	= chs->endpoint;
        //
        //        mad_mx_lock();
        //        rc		= mx_irecv(req.endpoint,
        //                                   NULL, 0,
        //                                   match_info,
        //                                   MX_MATCH_MASK_BC, NULL, &(req.request));
        //        mad_mx_unlock();
        //        mad_mx_check_status("mx_irecv", rc);
        //
        //        marcel_poll(pollid, &req);
        //        mad_mx_check_status("mx_test", req.test_return);
        //
        //        in = tbx_darray_get(in_darray, req.status.match_info & 0xFFFFFFFF);
        //}
#else /* MARCEL */
        //{
        //        mx_endpoint_t	ep	= NULL;
        //        mx_request_t	request	= 0;
        //        mx_status_t	status;
        //        uint32_t	result	= 0;
        //        mx_return_t	rc	= MX_SUCCESS;
        //
        //        ep = chs->endpoint;
        //        rc = mx_irecv(ep,
        //                      NULL, 0,
        //                      //&seg, 1,
        //                      match_info,
        //                      MX_MATCH_MASK_BC, NULL, &request);
        //        mad_mx_check_status("mx_irecv", rc);
        //        do {
        //                rc = mx_test(ep, &request, &status, &result);
        //        } while (rc == MX_SUCCESS && !result);
        //        mad_mx_check_status("mx_test", rc);
        //
        //        in = tbx_darray_get(in_darray, status.match_info & 0xFFFFFFFF);
        //}
#endif /* MARCEL */
        //
        //LOG_OUT();
        //
        //return in;



        p_mad_mx_channel_specific_t	chs		= NULL;
        uint64_t 			match_info	= 0;
        p_mad_connection_t		in		= NULL;
        p_mad_mx_connection_specific_t	is	= NULL;
        mx_request_t request	= 0;
        mx_status_t	status;
        uint32_t	result	= 0;
        mx_return_t	rc	= MX_SUCCESS;
        p_tbx_darray_t			in_darray	= NULL;

        mx_segment_t seg_temp;

        LOG_IN();
        chs		= ch->specific;
        in_darray	= ch->in_connection_darray;
        match_info	= (uint64_t)ch->id <<32;

        seg_temp.segment_ptr = chs->buffer;
        seg_temp.segment_length = FIRST_MSG_SIZE;

        rc = mx_irecv(chs->endpoint,
                      &seg_temp, 1,
                      match_info,
                      MX_MATCH_MASK_BC, NULL, &request);
        mad_mx_check_return("mx_irecv", rc);
        do {
                rc = mx_test(chs->endpoint, &request, &status, &result);
        } while (rc == MX_SUCCESS && !result);
        mad_mx_check_return("mx_test", rc);

        in = tbx_darray_get(in_darray, status.match_info & 0xFFFFFFFF);

        is = in->specific;
        is->begin_packing = 1;
        chs->length	= status.msg_length;
        //DISP_VAL("receive_msg, chs->length", chs->length);

        LOG_OUT();

        return in;
}

void
mad_mx_send_buffer(p_mad_link_t     lnk,
                   p_mad_buffer_t   buffer) {
        p_mad_connection_t		out	= NULL;
        p_mad_mx_connection_specific_t	os	= NULL;
        p_mad_mx_channel_specific_t	chs	= NULL;
        mx_return_t			rc	= MX_SUCCESS;
        mx_endpoint_addr_t		dest_address ;
        uint64_t			match_info = 0;
        mx_segment_t			seg;

        LOG_IN();
        out	= lnk->connection;
        os	= out->specific;
        chs	= out->channel->specific;

        dest_address	= os->remote_endpoint_addr;
        match_info	= (uint64_t)out->channel->id <<32 | out->channel->process_lrank;

#ifdef MARCEL
//        {
//                p_mad_mx_driver_specific_t	ds	= NULL;
//                marcel_pollid_t		pollid	= 0;
//                mad_mx_poll_req_t	req	= {
//                        .endpoint	= NULL,
//                        .request	= 0,
//                        .test_return	= MX_SUCCESS,
//                        .result		= 0
//                };
//
//                ds		= out->channel->adapter->driver->specific;
//                pollid		= ds->mx_pollid;
//                req.endpoint	= chs->endpoint;
//
//                mad_mx_lock();
// #error the segment is not filled
//                rc = mx_isend(req.endpoint,
//                              &seg, 1,
//                              dest_address,
//                              match_info,
//                              NULL, &req.request);
//                mad_mx_unlock();
//                mad_mx_check_status("mx_isend", rc);
//
//                marcel_poll(pollid, &req);
//                mad_mx_check_status("mx_test", req.test_return);
//        }
#else /* MARCEL */
        {
                mx_endpoint_t	ep = NULL;
                mx_request_t	request = 0;
                mx_status_t	status;
                uint32_t	result = 0;

                ep = chs->endpoint;

                if (os->begin_packing) {
                        uint32_t length = 0;

                        os->begin_packing = 0;

                        length = tbx_min(buffer->bytes_written - buffer->bytes_read,
                                         FIRST_MSG_SIZE);

                        seg.segment_ptr		= buffer->buffer+buffer->bytes_read;
                        seg.segment_length	= length;


                        rc = mx_isend(ep,
                                      &seg, 1,
                                      dest_address,
                                      match_info,
                                      NULL, &request);
                        mad_mx_check_return("mx_isend", rc);

                        do {
                                rc = mx_test(ep, &request, &status, &result);
                        } while (rc == MX_SUCCESS && !result);
                        mad_mx_check_return("mx_wait", rc);

                        buffer->bytes_read += length;

                        if (!mad_more_data(buffer))
                                goto no_more_data;
                }

                seg.segment_ptr = buffer->buffer+buffer->bytes_read;
                seg.segment_length = buffer->bytes_written - buffer->bytes_read;

                rc = mx_isend(ep,
                              &seg, 1,
                              dest_address,
                              match_info,
                              NULL, &request);
                mad_mx_check_return("mx_isend", rc);

                do {
                        rc = mx_test(ep, &request, &status, &result);
                } while (rc == MX_SUCCESS && !result);
                mad_mx_check_return("mx_wait", rc);

        no_more_data:
                ;
        }
#endif /* MARCEL */
        LOG_OUT();
}

void
mad_mx_receive_buffer(p_mad_link_t    lnk,
                      p_mad_buffer_t *_buffer) {
        p_mad_buffer_t			buffer  = *_buffer;
        p_mad_connection_t		in	= NULL;
        p_mad_mx_connection_specific_t	is	= NULL;
        p_mad_mx_channel_specific_t	chs	= NULL;
        mx_return_t			rc	= MX_SUCCESS;
        uint64_t match_info = 0;
        mx_segment_t seg;

        LOG_IN();
        in	= lnk->connection;
        is	= in->specific;
        chs	= in->channel->specific;

        if (is->begin_packing) {
                is->begin_packing = 0;

                if (chs->length > buffer->length - buffer->bytes_written)
                        FAILURE("message size larger than expected");

                memcpy(buffer->buffer + buffer->bytes_written, chs->buffer, chs->length);
                buffer->bytes_written += chs->length;

                if (mad_buffer_full(buffer))
                        goto no_more_data;
        }

        seg.segment_ptr = buffer->buffer + buffer->bytes_written;
        seg.segment_length = buffer->length - buffer->bytes_written;

        match_info = (uint64_t)in->channel->id <<32 | in->remote_rank;

#ifdef MARCEL
//        {
//                p_mad_mx_driver_specific_t	ds	= NULL;
//                marcel_pollid_t		pollid	= 0;
//                mad_mx_poll_req_t	req	= {
//                        .endpoint	= NULL,
//                        .request	= 0,
//                        .test_return	= MX_SUCCESS,
//                        .result		= 0
//                };
//
//                ds		= in->channel->adapter->driver->specific;
//                pollid		= ds->mx_pollid;
//                req.endpoint	= chs->endpoint;
//
//                mad_mx_lock();
//                rc = mx_irecv(req.endpoint,
//                              &seg, 1,
//                              match_info,
//                              MX_MATCH_MASK_NONE, NULL, &req.request);
//                mad_mx_unlock();
//                mad_mx_check_status("mx_irecv", rc);
//
//                marcel_poll(pollid, &req);
//                mad_mx_check_status("mx_test", req.test_return);
//        }
#else /* MARCEL */
        {
                mx_endpoint_t	ep	= NULL;
                mx_request_t	request	= 0;
                mx_status_t	status;
                uint32_t	result	= 0;

                ep = chs->endpoint;

                rc = mx_irecv(ep,
                              &seg, 1,
                              match_info,
                              MX_MATCH_MASK_NONE, NULL, &request);
                mad_mx_check_return("mx_irecv", rc);

                do {
                        rc = mx_test(ep, &request, &status, &result);
                } while (rc == MX_SUCCESS && !result);
                mad_mx_check_return("mx_wait", rc);
        }

 no_more_data:
        ;
#endif /* MARCEL */
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
        p_mad_connection_t		out	= NULL;
        p_mad_mx_connection_specific_t	os	= NULL;
        p_mad_mx_channel_specific_t	chs	= NULL;
        uint64_t                        match_info = 0;
        mx_endpoint_addr_t		dest_address ;
        p_tbx_list_t buffer_list = NULL;
        mx_return_t  rc         = MX_SUCCESS;
        mx_status_t  status;
        uint32_t     result	= 0;

        LOG_IN();
        out	= lnk->connection;
        os	= out->specific;
        chs	= out->channel->specific;
        match_info = (uint64_t)out->channel->id <<32 | out->channel->process_lrank;
        dest_address = os->remote_endpoint_addr;

        buffer_list = &buffer_group->buffer_list;

        if (!tbx_empty_list(buffer_list)) {
                mx_segment_t seg_list[buffer_list->length];
                mx_request_t test_list[2 * buffer_list->length];
                tbx_list_reference_t ref;
                unsigned int current_length = 0, i = 0, j = 0, k = 0;
                mx_segment_t seg_temp;

                seg_temp.segment_length = 0;

                tbx_list_reference_init(&ref, &(buffer_group->buffer_list));

                do{
                        p_mad_buffer_t b = NULL;
                        unsigned int missing_length = 0;

                        b = tbx_get_list_reference_object(&ref);

                        if(os->begin_packing){
                                mx_request_t	request	= 0;
                                mx_status_t	status;
                                uint32_t	result	= 0;
                                uint32_t	length	= 0;

                                os->begin_packing = 0;

                                // on envoie une premiere partie en guise de new_msg
                                length = tbx_min(b->bytes_written - b->bytes_read,
                                                 FIRST_MSG_SIZE);

                                seg_temp.segment_length = length;
                                seg_temp.segment_ptr = b->buffer;
                                rc = mx_isend(chs->endpoint,
                                              &seg_temp, 1,
                                              dest_address,
                                              match_info,
                                              NULL, &request);
                                mad_mx_check_return("mx_isend", rc);
                                do {
                                        rc = mx_test(chs->endpoint, &request, &status, &result);
                                } while (rc == MX_SUCCESS && !result);
                                mad_mx_check_return("mx_wait", rc);

                                //DISP_VAL("mad_mx_send_buffer_group_2, length", length);

                                b->bytes_read += length;

                                seg_temp.segment_length = 0;

                                if(!mad_more_data(b))
                                        goto no_more_data;

                        }

                        missing_length = b->bytes_written - b->bytes_read;

                        if(current_length + missing_length > THRESHOLD) {
                                unsigned int x_threshold = 0;

                                // on complete le multi_segment courant
                                seg_list[i].segment_ptr = b->buffer + b->bytes_read;
                                seg_list[i].segment_length = (THRESHOLD - current_length);

                                missing_length -= (THRESHOLD - current_length);
                                // on envoie le reste du tampon considéré par morceau de taille THRESHOLD
                                x_threshold = missing_length / THRESHOLD;

                                if(x_threshold > 0){
                                        seg_temp.segment_ptr =  seg_list[i].segment_ptr + seg_list[i].segment_length;
                                        seg_temp.segment_length = THRESHOLD * x_threshold;

                                        test_list[j++] = mad_mx_send(chs->endpoint, &seg_temp, 1, dest_address, match_info);
                                        missing_length -= x_threshold * THRESHOLD;
                                }
                                //on garde le surplus pour le multi_segment suivant
                                seg_temp.segment_ptr = seg_list[i].segment_ptr + seg_list[i].segment_length + THRESHOLD * x_threshold;
                                seg_temp.segment_length = missing_length;
                        } else {
                                seg_list[i].segment_ptr = b->buffer + b->bytes_read;
                                seg_list[i].segment_length = missing_length;
                        }
                        current_length += seg_list[i].segment_length;
                        i++;

                        if(current_length == THRESHOLD){
                                test_list[j++] = mad_mx_send(chs->endpoint, seg_list, i, dest_address, match_info);
                                current_length = 0;
                                if(seg_temp.segment_length != 0){
                                        seg_list[0] = seg_temp;
                                        i = 1;
                                        current_length = seg_temp.segment_length;
                                        seg_temp.segment_length = 0;
                                } else {
                                        i = 0;
                                        current_length = 0;
                                }
                        }

                no_more_data:
                        ;

                }while(tbx_forward_list_reference(&ref));

                if(i != 0) {
                        test_list[j++] = mad_mx_send(chs->endpoint, seg_list, i, dest_address, match_info);
                }

                //DISP_VAL("em : j", j);


                for(k = 0; k < j; k++){

                        //      DISP_VAL("em: k dans while", k);
                        do {
                                rc = mx_test(chs->endpoint, &test_list[k], &status, &result);
                        } while (rc == MX_SUCCESS && !result);
                        mad_mx_check_return("mx_wait", rc);
                }
        }
        LOG_OUT();
}

void
mad_mx_receive_sub_buffer_group_2(p_mad_link_t         lnk,
                                  p_mad_buffer_group_t buffer_group) {
        p_mad_connection_t		in	    = NULL;
        p_mad_mx_connection_specific_t	is	    = NULL;
        p_mad_mx_channel_specific_t	chs	    = NULL;
        uint64_t                        match_info  = 0;
        p_tbx_list_t                    buffer_list = NULL;
        mx_return_t  rc         = MX_SUCCESS;
        mx_status_t  status;
        uint32_t     result	= 0;

        LOG_IN();
        in	= lnk->connection;
        is	= in->specific;
        chs	= in->channel->specific;
        match_info = (uint64_t)in->channel->id <<32 | in->remote_rank;
        buffer_list = &buffer_group->buffer_list;

        if (!tbx_empty_list(buffer_list)) {
                mx_segment_t seg_list[buffer_list->length];
                mx_request_t test_list[2 * buffer_list->length];
                tbx_list_reference_t ref;
                unsigned int current_length = 0, i =0, j = 0, k = 0;
                mx_segment_t seg_temp;

                seg_temp.segment_length = 0;
                tbx_list_reference_init(&ref, &(buffer_group->buffer_list));

                do{
                        p_mad_buffer_t b = NULL;
                        unsigned int missing_length = 0;

                        b = tbx_get_list_reference_object(&ref);

                        if(is->begin_packing){
                                is->begin_packing = 0;

                                //on a deja recu une premiere partie dans receive_message
                                if (chs->length > (b->length - b->bytes_written))
                                        FAILURE("message size larger than expected");
                                memcpy(b->buffer + b->bytes_written, chs->buffer, chs->length);

                                b->bytes_written += chs->length;

                                if (mad_buffer_full(b))
                                        goto no_more_data;
                        }

                        missing_length = b->length - b->bytes_written;

                        if(current_length + missing_length > THRESHOLD) {
                                unsigned int x_threshold = 0;
                                // on complete le multi_segment courant
                                seg_list[i].segment_ptr = b->buffer + b->bytes_written;
                                seg_list[i].segment_length = (THRESHOLD - current_length);

                                missing_length -= (THRESHOLD - current_length);
                                // on envoie le reste du tampon considéré par morceau de taille THRESHOLD
                                x_threshold = missing_length / THRESHOLD;

                                if(x_threshold > 0){
                                        seg_temp.segment_ptr =  seg_list[i].segment_ptr + seg_list[i].segment_length;
                                        seg_temp.segment_length = THRESHOLD * x_threshold;

                                        test_list[j++] = mad_mx_recv(chs->endpoint, &seg_temp, 1, match_info);

                                        missing_length -= x_threshold * THRESHOLD;
                                }
                                //on garde le surplus pour le multi_segment suivant
                                seg_temp.segment_ptr = seg_list[i].segment_ptr + seg_list[i].segment_length + THRESHOLD * x_threshold;
                                seg_temp.segment_length = missing_length;
                        } else {
                                seg_list[i].segment_ptr = b->buffer + b->bytes_written;
                                seg_list[i].segment_length = missing_length;
                        }
                        current_length += seg_list[i].segment_length;
                        i++;

                        if(current_length == THRESHOLD){
                                test_list[j++] = mad_mx_recv(chs->endpoint, seg_list, i, match_info);
                                current_length = 0;
                                if(seg_temp.segment_length != 0){
                                        seg_list[0] = seg_temp;
                                        i = 1;
                                        current_length = seg_temp.segment_length;
                                        seg_temp.segment_length = 0;
                                } else {
                                        i = 0;
                                        current_length = 0;
                                }
                        }

                no_more_data:
                        ;
                }while(tbx_forward_list_reference(&ref));

                if(i != 0) {
                        test_list[j++] = mad_mx_recv(chs->endpoint, seg_list, i, match_info);
                }


                //                DISP_VAL("recv : j", j);


                for(k = 0; k < j; k++){
                        //                        DISP_VAL("recv : k dans while", k);
                        do {
                                rc = mx_test(chs->endpoint, &test_list[k], &status, &result);
                        } while (rc == MX_SUCCESS && !result);
                        mad_mx_check_return("mx_wait", rc);
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
