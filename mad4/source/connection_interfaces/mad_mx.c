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

// Uncomment the following line if you are using a version >= 1.0.0
#define MX_VERSION_AFTER_1_0_0

#ifdef MX_VERSION_AFTER_1_0_0
#define MX_MATCH_MASK_BC   ((uint64_t)0xffffffff << 32)
#endif

/*
 * local structures
 * ----------------
 */
typedef struct s_mad_mx_driver_specific{
    int dummy;
} mad_mx_driver_specific_t, *p_mad_mx_driver_specific_t;

typedef struct s_mad_mx_adapter_specific{
    uint64_t                nic_id;
    uint32_t                board_id;
} mad_mx_adapter_specific_t, *p_mad_mx_adapter_specific_t;

typedef struct s_mad_mx_channel_specific{
    int dummy;
} mad_mx_channel_specific_t, *p_mad_mx_channel_specific_t;

typedef struct s_mad_mx_connection_specific{
    int dummy;
} mad_mx_connection_specific_t, *p_mad_mx_connection_specific_t;


typedef struct s_mad_mx_track_specific{
    mx_endpoint_t      *endpoint;
    mx_endpoint_addr_t *endpoint_addr;
    uint32_t           endpoint_id;

    mx_request_t  *request;
}mad_mx_track_specific_t, *p_mad_mx_track_specific_t;

#define MAD_MX_GATHER_SCATTER_THRESHOLD  32768
#define MAD_MX_PRE_POSTED_SIZE           32000

static p_tbx_memory_t mad_mx_unexpected_key;

static
void
mad_mx_print_status(mx_status_t s) TBX_UNUSED;

static
void
mad_mx_print_status(mx_status_t s){
    const char *msg = NULL;
    LOG_IN();

    msg = mx_strstatus(s.code);
    DISP("mx status: %s", msg);
    LOG_OUT();
}

static
void
mad_mx_check_return(char *msg, mx_return_t return_code) {
    LOG_IN();
    if (return_code != MX_SUCCESS){
        const char *msg_mx = NULL;

        msg_mx = mx_strerror(return_code);

        DISP("%s failed with code %s = %d/0x%x", msg, msg_mx, return_code, return_code);
        FAILURE("mx error");
    }
    LOG_OUT();
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

#ifdef MX_VERSION_AFTER_1_0_0
    return_code = mx_get_info(NULL, MX_NIC_COUNT, &nb_nic, sizeof(nb_nic), &nb_nic, sizeof(nb_nic));
#else // ! MX_VERSION_AFTER_1_0_0
    return_code = mx_get_info(NULL, MX_NIC_COUNT, &nb_nic, sizeof(nb_nic));
#endif // MX_VERSION_AFTER_1_0_0
    nb_nic = nb_nic & 0xff;

    mad_mx_check_return("mad_mx_startup_info", return_code);
    DISP("NIC count: %d", nb_nic);


    nic_id_array = TBX_MALLOC((nb_nic+1) * sizeof(*nic_id_array));
    mad_mx_check_return("mad_mx_startup_info", return_code);


#ifdef MX_VERSION_AFTER_1_0_0
    return_code = mx_get_info(NULL, MX_NIC_IDS, nic_id_array, (nb_nic+1) * sizeof(*nic_id_array), nic_id_array, (nb_nic+1) * sizeof(*nic_id_array));
#else // ! MX_VERSION_AFTER_1_0_0
    return_code = mx_get_info(NULL, MX_NIC_IDS, nic_id_array, (nb_nic+1) * sizeof(*nic_id_array));
#endif // MX_VERSION_AFTER_1_0_0
    mad_mx_check_return("mad_mx_startup_info", return_code);

    for (i = 0; i < nb_nic; i++) {
        DISP("NIC %d id: %llx", i, nic_id_array[i]);
    }
    TBX_FREE(nic_id_array);
    nic_id_array = NULL;

#ifdef MX_VERSION_AFTER_1_0_0
    return_code = mx_get_info(NULL, MX_MAX_NATIVE_ENDPOINTS, &nb_native_ep, sizeof(nb_native_ep), &nb_native_ep, sizeof(nb_native_ep));
#else // ! MX_VERSION_AFTER_1_0_0
    return_code = mx_get_info(NULL, MX_MAX_NATIVE_ENDPOINTS, &nb_native_ep, sizeof(nb_native_ep));
#endif // MX_VERSION_AFTER_1_0_0
    mad_mx_check_return("mad_mx_startup_info", return_code);
    DISP("Native endpoint count: %d", nb_native_ep);

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
    interface->driver_init          = mad_mx_driver_init;
    interface->adapter_init         = mad_mx_adapter_init;
    interface->channel_init         = mad_mx_channel_init;
    interface->before_open_channel  = NULL;
    interface->connection_init      = mad_mx_connection_init;
    interface->link_init            = NULL; //mad_mx_link_init;
    interface->accept               = mad_mx_accept;
    interface->connect              = mad_mx_connect;
    interface->after_open_channel   = NULL;
    interface->before_close_channel = NULL;
    interface->disconnect           = mad_mx_disconnect;
    interface->after_close_channel  = NULL;
    interface->link_exit            = NULL;
    interface->connection_exit      = mad_mx_connection_exit;
    interface->channel_exit         = mad_mx_channel_exit;
    interface->adapter_exit         = mad_mx_adapter_exit;
    interface->driver_exit          = mad_mx_driver_exit;

    interface->new_message          = mad_mx_new_message;
    interface->finalize_message     = NULL;
    interface->receive_message      = mad_mx_receive_message;
    interface->message_received     = NULL;


    interface->rdma_available             = NULL;
    interface->gather_scatter_available   = NULL;
    interface->get_max_nb_ports           = NULL;
    interface->msg_length_max             = NULL;
    interface->copy_length_max            = mad_mx_cpy_limit_size;
    interface->gather_scatter_length_max  = mad_mx_gather_scatter_length_max;
    interface->need_rdv                   = mad_mx_need_rdv;
    interface->buffer_need_rdv            = mad_mx_buffer_need_rdv;


    interface->isend               = mad_mx_isend;
    interface->irecv               = mad_mx_irecv;
    interface->send                = NULL;
    interface->recv                = NULL;
    interface->test                = mad_mx_test;
    interface->wait                = mad_mx_wait;
    interface->open_track          = mad_mx_open_track;

    interface->add_pre_posted = mad_mx_add_pre_posted;
    interface->clean_pre_posted = mad_mx_clean_pre_posted;
    interface->remove_all_pre_posted = mad_mx_remove_all_pre_posted;

    interface->add_pre_sent   = NULL;

    LOG_OUT();
    return "mx";
}


void
mad_mx_driver_init(p_mad_driver_t d, int *argc, char ***argv) {
    mx_return_t return_code	= MX_SUCCESS;
    LOG_IN();

    d->connection_type  = mad_bidirectional_connection;
    d->buffer_alignment = 32;

    TRACE("Initializing MX driver");
    mx_set_error_handler(MX_ERRORS_RETURN);

    return_code	= mx_init();
    mad_mx_check_return("mx_init", return_code);

    /* debug only */
#ifndef MX_NO_STARTUP_INFO
    mad_mx_startup_info();
#endif // MX_NO_STARTUP_INFO

    LOG_OUT();
}

void
mad_mx_adapter_init(p_mad_adapter_t a) {
    p_mad_mx_adapter_specific_t	as	  = NULL;
    LOG_IN();
    as		= TBX_MALLOC(sizeof(mad_mx_adapter_specific_t));

    if (strcmp(a->dir_adapter->name, "default")) {
        FAILURE("unsupported adapter");
    } else {
        as->board_id	= MX_ANY_NIC;
    }

    a->specific	= as;
    a->parameter	= tbx_strdup("-");

    tbx_malloc_init(&mad_mx_unexpected_key, MAD_MX_PRE_POSTED_SIZE, 512, "mx_pre_posted");
    LOG_OUT();
}

void
mad_mx_channel_init(p_mad_channel_t ch) {
    p_mad_adapter_t adapter = NULL;
    p_mad_mx_adapter_specific_t as = NULL;
    p_tbx_string_t param_str	= NULL;

    p_mad_track_set_t s_track_set = NULL;
    p_mad_track_t track = NULL;
    p_mad_mx_track_specific_t ts = NULL;

    uint32_t i = 0;
    LOG_IN();
    adapter = ch->adapter;
    as = adapter->specific;

    s_track_set = adapter->s_track_set;
    track = s_track_set->tracks_tab[0];
    ts = track->specific;

    param_str = tbx_string_init_to_int(ts->endpoint_id);
    for(i = 1; i < s_track_set->nb_track; i++){
        track = s_track_set->tracks_tab[i];
        ts = track->specific;

        tbx_string_append_char(param_str, ' ');
        tbx_string_append_int (param_str, ts->endpoint_id);
    }
    ch->parameter = tbx_string_to_cstring(param_str);
    tbx_string_free(param_str);
    param_str = NULL;

    LOG_OUT();
}

void
mad_mx_connection_init(p_mad_connection_t in,
                       p_mad_connection_t out) {
    LOG_IN();
    in->nb_link		        = 1;
    out->nb_link		= 1;
    LOG_OUT();
}

static mx_endpoint_addr_t
mad_mx_connect_endpoint(mx_endpoint_t *e,
                        uint64_t remote_nic_id,
                        uint32_t remote_endpoint_id){
    mx_endpoint_addr_t re_addr;
    const uint32_t     e_key		= 0xFFFFFFFF;
    mx_return_t        return_code	= MX_SUCCESS;
    LOG_IN();

    return_code	= mx_connect(*e,
                             remote_nic_id,
                             remote_endpoint_id,
                             e_key,
                             MX_INFINITE,
                             &re_addr);

    mad_mx_check_return("mx_connect", return_code);
    LOG_OUT();
    return re_addr;
}


static
void
mad_mx_accept_connect(p_mad_connection_t   cnx,
                      p_mad_adapter_info_t ai) {
    p_mad_adapter_t      a           = NULL;
    p_mad_track_set_t    s_track_set = NULL;
    p_mad_track_t        s_track     = NULL;
    ntbx_process_lrank_t remote_rank = 0;
    LOG_IN();

    a           = cnx->channel->adapter;
    s_track_set = a->s_track_set;
    s_track     = s_track_set->tracks_tab[0];
    remote_rank = cnx->remote_rank;

    if(s_track->remote_tracks[remote_rank]){
        goto end;
    } else {
        char	     *remote_hostname     = NULL;
        size_t	      remote_hostname_len = 0;
        uint64_t      remote_nic_id	     = 0;

        char *param = NULL;
        char *endpoint_id = NULL;
        char *ptr = NULL;

        p_mad_track_t     remote_track = NULL;
        p_mad_mx_track_specific_t remote_ts = NULL;

        p_mad_mx_track_specific_t s_ts = NULL;

        mx_return_t rc = MX_SUCCESS;
        uint32_t    i = 1;


        /* Hostname */
        remote_hostname_len	= strlen(ai->dir_node->name) + 2;
        remote_hostname	        = TBX_MALLOC(remote_hostname_len + 1);
        strcpy(remote_hostname, ai->dir_node->name);

        {
            char *tmp = NULL;

            tmp = strchr(remote_hostname, '.');
            if (tmp) {
                *tmp = '\0';
            }
        }
        strcat(remote_hostname, ":0");

        /* Remote NIC id */
        rc = mx_hostname_to_nic_id(remote_hostname, &remote_nic_id);
        mad_mx_check_return("mx_hostname_to_nic_id", rc);


        /* Initialisation of the remote tracks */
        param = ai->channel_parameter;

        endpoint_id = strtok(param, " ");
        if (!endpoint_id || endpoint_id == ptr)
            FAILURE("invalid channel parameter");

        remote_ts = TBX_MALLOC(sizeof(mad_mx_track_specific_t));
        remote_ts->endpoint_id = strtol(endpoint_id, &ptr, 0);

        s_track  = s_track_set->tracks_tab[0];
        s_ts     = s_track->specific;
        remote_ts ->endpoint_addr = TBX_MALLOC(sizeof(mx_endpoint_addr_t));
        *remote_ts->endpoint_addr = mad_mx_connect_endpoint(s_ts->endpoint,
                                                            remote_nic_id,
                                                            remote_ts->endpoint_id);

        remote_track = TBX_MALLOC(sizeof(mad_track_t));
        remote_track->id = 0;
        remote_track->specific = remote_ts;

        s_track->remote_tracks[remote_rank] = remote_track;

        while (1){
            /* extract string from string sequence */
            endpoint_id = strtok(NULL, " ");
            if (endpoint_id == ptr)
                FAILURE("invalid channel parameter");
            /* check if there is nothing else to extract */
            if (!endpoint_id)
                break;

            remote_ts = TBX_MALLOC(sizeof(mad_mx_track_specific_t));
            remote_ts->endpoint_id = strtol(endpoint_id, &ptr, 0);

            s_track  = s_track_set->tracks_tab[i];
            s_ts     = s_track->specific;
            remote_ts ->endpoint_addr = TBX_MALLOC(sizeof(mx_endpoint_addr_t));
            *remote_ts->endpoint_addr = mad_mx_connect_endpoint(s_ts->endpoint,
                                                                remote_nic_id,
                                                                remote_ts->endpoint_id);

            remote_track = TBX_MALLOC(sizeof(mad_track_t));
            remote_track->id = i;
            remote_track->specific = remote_ts;

            s_track->remote_tracks[remote_rank] = remote_track;

            i++;
        }
        TBX_FREE(remote_hostname);
        remote_hostname = NULL;
        remote_hostname_len = 0;
    }
 end:
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

/********************************************************/
/********************************************************/
/********************************************************/
/********************************************************/
void
mad_mx_disconnect(p_mad_connection_t c) {
    LOG_IN();
    LOG_OUT();
}

void
mad_mx_connection_exit(p_mad_connection_t in,
                       p_mad_connection_t out) {
    LOG_IN();
    LOG_OUT();
}

void
mad_mx_channel_exit(p_mad_channel_t ch) {
    LOG_IN();
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

    tbx_malloc_clean(mad_mx_unexpected_key);
    LOG_OUT();
}

void
mad_mx_driver_exit(p_mad_driver_t d) {
    mx_return_t return_code	= MX_SUCCESS;
    LOG_IN();

    TRACE("Finalizing MX driver");
    return_code	= mx_finalize();
    mad_mx_check_return("mx_finalize", return_code);

    LOG_OUT();
}

/********************************************************/
/********************************************************/
/********************************************************/
/********************************************************/
void
mad_mx_new_message(p_mad_connection_t out){
    p_mad_adapter_t adapter = NULL;
    mx_endpoint_t *endpoint;
    mx_endpoint_addr_t *dest_address;
    uint64_t match_info = 0;
    mx_request_t request;
    mx_return_t  rc    = MX_SUCCESS;
    p_mad_track_t s_track = NULL;
    p_mad_mx_track_specific_t s_ts = NULL;
    p_mad_track_t r_track = NULL;
    p_mad_mx_track_specific_t r_ts = NULL;
    mx_status_t                 status;
    uint32_t                    result;
    LOG_IN();

    adapter = out->channel->adapter;
    s_track = adapter->s_track_set->tracks_tab[0];
    s_ts = s_track->specific;
    endpoint = s_ts->endpoint;

    r_track = s_track->remote_tracks[out->remote_rank];
    r_ts = r_track->specific;
    dest_address = r_ts->endpoint_addr;


    match_info = ((uint64_t)out->channel->id +1) << 32
        | out->channel->process_lrank;


    rc = mx_isend(*endpoint,
                  NULL, 0,
                  *dest_address,
                  match_info,
                  NULL, &request);
    do {
        rc = mx_test(*endpoint, &request, &status, &result);
    } while (rc == MX_SUCCESS && !result);
    mad_mx_check_return("mx_test", rc);
    LOG_OUT();
}

p_mad_connection_t
mad_mx_receive_message(p_mad_channel_t ch) {
    p_mad_mx_channel_specific_t	chs		= NULL;
    p_mad_connection_t		in		= NULL;
    p_tbx_darray_t		in_darray = NULL;
    uint64_t 			match_info	= 0;
    mx_segment_t 		*s;
    p_mad_adapter_t             a  = NULL;
    p_mad_mx_adapter_specific_t as = NULL;

    mx_endpoint_t               *ep;
    mx_request_t                request;
    mx_status_t                 status;
    uint32_t                    result;
    mx_return_t                 rc = MX_SUCCESS;
    p_mad_track_t track = NULL;
    p_mad_mx_track_specific_t ts = NULL;

    p_mad_iovec_t mad_iovec = NULL;
    void *recv_msg = NULL;
    LOG_IN();
    chs		= ch->specific;
    in_darray	= ch->in_connection_darray;
    match_info	= ((uint64_t)ch->id + 1) <<32;
    a = ch->adapter;
    as = a->specific;

    track = tbx_htable_get(a->r_track_set->tracks_htable,
                           "cpy");
    ts = track->specific;
    ep = ts->endpoint;


    mad_iovec = mad_iovec_create(0, 0);

    recv_msg = tbx_malloc(mad_mx_unexpected_key);
    mad_iovec_add_data(mad_iovec,
                       recv_msg,
                       MAD_MX_PRE_POSTED_SIZE,
                       0);

    s = (mx_segment_t *) (mad_iovec->data);

    rc	= mx_irecv(*ep,
                   NULL, 0,
                   match_info,
                   MX_MATCH_MASK_BC, NULL, &request);
    do {
        rc = mx_test(*ep, &request, &status, &result);
    } while (rc == MX_SUCCESS && !result);
    mad_mx_check_return("mx_test", rc);

    match_info = status.match_info;
    in	= tbx_darray_get(in_darray, match_info & 0xFFFFFFFF);
    if(!in){
        FAILURE("mad_mx_receive_message : connection not found");
    }

    tbx_free(mad_mx_unexpected_key, recv_msg);

    LOG_OUT();
    return in;
}

/*************************************************************************************************/
/*************************************************************************************************/
/*************************************************************************************************/
tbx_bool_t
mad_mx_need_rdv(p_mad_iovec_t mad_iovec){
    LOG_IN();
    if(mad_iovec->length > MAD_MX_PRE_POSTED_SIZE)
        return tbx_true;
    LOG_OUT();

    return tbx_false;
}

tbx_bool_t
mad_mx_buffer_need_rdv(size_t buffer_length){
    LOG_IN();
    if(buffer_length > MAD_MX_PRE_POSTED_SIZE)
        return tbx_true;
    LOG_OUT();

    return tbx_false;
}

uint32_t
mad_mx_gather_scatter_length_max(void){
    return MAD_MX_GATHER_SCATTER_THRESHOLD;
}


uint32_t
mad_mx_cpy_limit_size(void){
    return MAD_MX_PRE_POSTED_SIZE;
}

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

tbx_bool_t
mad_mx_test(p_mad_track_t track){
    p_mad_mx_track_specific_t ts = NULL;
    mx_request_t *request;
    mx_endpoint_t *endpoint;

    mx_return_t  rc = MX_SUCCESS;
    mx_status_t  status;
    uint32_t     result	= 0;
    LOG_IN();

    ts = track->specific;
    request = ts->request;
    endpoint = ts->endpoint;

    //DISP("mad_mx_test : TEST");
    rc = mx_test(*endpoint, request, &status, &result);
    mad_mx_check_return("mx_test failed", rc);

    if(rc == MX_SUCCESS && result){
        return tbx_true;
    }
    LOG_OUT();
    return tbx_false;
}

void
mad_mx_wait(p_mad_track_t track){
    p_mad_mx_track_specific_t ts = NULL;
    mx_request_t *request;
    mx_endpoint_t *endpoint;

    mx_return_t  rc = MX_SUCCESS;
    mx_status_t  status;
    uint32_t     result	= 0;
    LOG_IN();

    ts = track->specific;
    request = ts->request;
    endpoint = ts->endpoint;

    do {
        rc = mx_test(*(endpoint), request, &status, &result);
    } while (rc == MX_SUCCESS && !result);
    mad_mx_check_return("mx_test failed", rc);
    LOG_OUT();
}

void
mad_mx_open_track(p_mad_adapter_t adapter,
                  uint32_t track_id){
    p_mad_mx_track_specific_t s_ts = NULL;
    p_mad_mx_track_specific_t r_ts = NULL;
    p_mad_mx_adapter_specific_t as = NULL;
    uint64_t nic_id = 0;
    uint32_t max_native_endpoints = 0;

    p_mad_track_t s_track = NULL;
    p_mad_track_t r_track = NULL;

    mx_endpoint_t endpoint;
    uint32_t        endpoint_id  = 1;
    const uint32_t  endpoint_key = 0xFFFFFFFF;
    mx_return_t     rc    = MX_SUCCESS;
    LOG_IN();

    as = adapter->specific;
    nic_id = as->board_id;


#ifdef MX_VERSION_AFTER_1_0_0
    rc = mx_get_info(endpoint,
                     MX_MAX_NATIVE_ENDPOINTS,
                     NULL,
                     0,
                     &max_native_endpoints,
                     sizeof(uint32_t));
#else // ! MX_VERSION_AFTER_1_0_0
    rc = mx_get_info(endpoint,
                     MX_MAX_NATIVE_ENDPOINTS,
                     &max_native_endpoints,
                     sizeof(uint32_t));
#endif // MX_VERSION_AFTER_1_0_0
    mad_mx_check_return("mx_open_track", rc);

    s_track = adapter->s_track_set->tracks_tab[track_id];
    s_ts = TBX_MALLOC(sizeof(mad_mx_track_specific_t));
    s_ts->endpoint      = TBX_MALLOC(sizeof(mx_endpoint_t));
    s_ts->endpoint_addr = TBX_MALLOC(sizeof(mx_endpoint_addr_t));
    s_ts->request       = TBX_MALLOC(sizeof(mx_request_t));


    r_track = adapter->r_track_set->tracks_tab[track_id];
    r_ts          = TBX_MALLOC(sizeof(mad_mx_track_specific_t));
    r_ts->request = TBX_MALLOC(sizeof(mx_request_t));

    for(endpoint_id = 1, rc = MX_BUSY;
        rc == MX_BUSY && endpoint_id <= max_native_endpoints;
        endpoint_id++){
        rc = mx_open_endpoint(nic_id,
                              endpoint_id,
                              endpoint_key,
                              NULL,
                              0,
                              s_ts->endpoint);
    }
    mad_mx_check_return("mx_open_track", rc);
    r_ts->endpoint = s_ts->endpoint;

    s_ts->endpoint_id = endpoint_id - 1;
    r_ts->endpoint_id = endpoint_id - 1;

    rc = mx_get_endpoint_addr(*s_ts->endpoint, s_ts->endpoint_addr);
    mad_mx_check_return("mx_open_track", rc);
    r_ts->endpoint_addr = s_ts->endpoint_addr;

    s_track->specific = s_ts;
    r_track->specific = r_ts;
    LOG_OUT();
}

void
mad_mx_isend(p_mad_track_t track,
             ntbx_process_lrank_t remote_rank,
             struct iovec *iovec, uint32_t nb_seg){
    p_mad_mx_track_specific_t ts = NULL;
    p_mad_track_t remote_track = NULL;
    p_mad_mx_track_specific_t remote_ts = NULL;
    mx_endpoint_t *endpoint;
    mx_endpoint_addr_t *dest_address;
    uint64_t match_info = 0;
    mx_segment_t *seg_list = NULL;
    mx_request_t *request;
    mx_return_t     rc    = MX_SUCCESS;
    LOG_IN();

    ts       = track->specific;
    endpoint = ts->endpoint;
    request  = ts->request;
    remote_track = track->remote_tracks[remote_rank];
    remote_ts    = remote_track->specific;
    dest_address = remote_ts->endpoint_addr;
    seg_list     = (mx_segment_t *)iovec;
    match_info   = 0;
    /* match_info = ((uint64_t)connection->channel->id +1) << 32 | connection->channel->process_lrank; */

    rc = mx_isend(*endpoint,
                  seg_list, nb_seg,
                  *dest_address,
                  match_info,
                  NULL, ts->request);
    LOG_OUT();
}




void
mad_mx_irecv(p_mad_track_t track,
             struct iovec *iovec, uint32_t nb_seg){
    p_mad_mx_track_specific_t ts = NULL;
    mx_endpoint_t *endpoint;
    uint64_t match_info = 0;
    mx_segment_t *seg_list = NULL;
    mx_request_t *request;
    mx_return_t     rc    = MX_SUCCESS;
    LOG_IN();

    ts = track->specific;
    endpoint = ts->endpoint;
    request  = ts->request;
    seg_list = (mx_segment_t *)iovec;
    match_info = 0;
    /* match_info = ((uint64_t)connection->channel->id +1) << 32 | connection->channel->process_lrank; */

    rc	= mx_irecv(*endpoint,
                   seg_list, nb_seg,
                   match_info,
                   MX_MATCH_MASK_NONE, NULL,
                   request);
    LOG_OUT();
}



void
mad_mx_add_pre_posted(p_mad_adapter_t adapter,
                      p_mad_track_t track){
    p_mad_iovec_t mad_iovec = NULL;
    void *unexpected_area   = NULL;
    LOG_IN();

    mad_iovec = mad_iovec_create(0, 0);
    mad_iovec->track = track;

    unexpected_area = tbx_malloc(mad_mx_unexpected_key);
    mad_iovec_add_data(mad_iovec,
                       unexpected_area,
                       MAD_MX_PRE_POSTED_SIZE,
                       0);
    tbx_slist_append(adapter->r_track_set->pipeline, mad_iovec);
    adapter->r_track_set->pipeline_nb_elm++;

    mad_mx_irecv(track, mad_iovec->data, 1);
    LOG_OUT();
}

void
mad_mx_clean_pre_posted(p_mad_iovec_t mad_iovec){
    LOG_IN();
    tbx_free(mad_mx_unexpected_key,
             mad_iovec->data[0].iov_base);

    mad_iovec_free(mad_iovec, tbx_false);
    LOG_OUT();
}


void
mad_mx_remove_all_pre_posted(p_mad_adapter_t adapter,
                             p_mad_track_t track){
    p_tbx_slist_t r_pipeline = NULL;
    p_mad_iovec_t mad_iovec = NULL;
    tbx_bool_t ref_forward = tbx_true;
    LOG_IN();
    //DISP("-->remove_pre_posted");

    if(!adapter->r_track_set)
        goto end;

    r_pipeline = adapter->r_track_set->pipeline;

    if(!r_pipeline->length){
        goto end;
    }

    tbx_slist_ref_to_head(r_pipeline);
    while(1){
        ref_forward = tbx_slist_ref_forward(r_pipeline);

        mad_iovec = tbx_slist_extract(r_pipeline);
        if(mad_iovec == NULL)
            break;

        tbx_free(mad_mx_unexpected_key,
                 mad_iovec->data[0].iov_base);
        mad_iovec_free(mad_iovec, tbx_false);

        adapter->r_track_set->pipeline_nb_elm--;

        if(!ref_forward)
            break;
    }

 end:
    //DISP("<--remove_pre_posted");
    LOG_OUT();
}


void
mad_mx_close_track(p_mad_track_t track){
    //return_code	= mx_close_endpoint(as->endpoint1);
    //mad_mx_check_return("mx_close_endpoint 1", return_code);

}


/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
