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

tbx_tick_t chrono_test;

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
    uint64_t nic_id;
    uint32_t board_id;

    uint32_t nb_pre_posted_areas;
} mad_mx_adapter_specific_t, *p_mad_mx_adapter_specific_t;

typedef struct s_mad_mx_channel_specific{
    int dummy;
} mad_mx_channel_specific_t, *p_mad_mx_channel_specific_t;

typedef struct s_mad_mx_connection_specific{
    int dummy;
} mad_mx_connection_specific_t, *p_mad_mx_connection_specific_t;


typedef struct s_mad_mx_track_set_specific{
    int dummy;
}mad_mx_track_set_specific_t, *p_mad_mx_track_set_specific_t;


typedef struct s_mad_mx_track_specific{
    int dummy;
}mad_mx_track_specific_t, *p_mad_mx_track_specific_t;

typedef struct s_mad_mx_port_specific{
    mx_endpoint_t      *endpoint;
    mx_endpoint_addr_t *endpoint_addr;
    uint32_t            endpoint_id;

    mx_request_t       *request;

    p_mad_iovec_t mad_iovec;

}mad_mx_port_specific_t, *p_mad_mx_port_specific_t;

//#define MAD_MX_GATHER_SCATTER_THRESHOLD  32768
#define MAD_MX_PRE_POSTED_SIZE           32000

//static p_tbx_memory_t mad_mx_unexpected_key;

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
mad_mx_register(p_mad_driver_interface_t interface) {
    LOG_IN();
    TRACE("Registering MX driver");
    interface->driver_init          = mad_mx_driver_init;
    interface->adapter_init         = mad_mx_adapter_init;
    interface->channel_init         = mad_mx_channel_init;
    interface->connection_init      = mad_mx_connection_init;
    interface->link_init            = NULL;
    interface->track_set_init       = NULL;
    interface->track_init           = mad_mx_track_init;

    interface->before_open_channel  = NULL;
    interface->accept               = mad_mx_accept;
    interface->connect              = mad_mx_connect;
    interface->after_open_channel   = NULL;

    interface->before_close_channel = NULL;
    interface->disconnect           = NULL;
    interface->after_close_channel  = NULL;
    interface->link_exit            = NULL;
    interface->connection_exit      = mad_mx_connection_exit;
    interface->channel_exit         = mad_mx_channel_exit;
    interface->adapter_exit         = mad_mx_adapter_exit;
    interface->driver_exit          = mad_mx_driver_exit;
    interface->track_set_exit       = NULL;
    interface->track_exit           = mad_mx_track_exit; //mad_mx_close_track;

    interface->new_message          = mad_mx_new_message;
    interface->finalize_message     = NULL;
    interface->receive_message      = mad_mx_receive_message;
    interface->message_received     = NULL;

    interface->init_pre_posted       = mad_mx_init_pre_posted;
    interface->replace_pre_posted    = mad_mx_replace_pre_posted;
    interface->remove_all_pre_posted = mad_mx_remove_all_pre_posted;


    //interface->rdma_available             = NULL;
    //interface->gather_scatter_available   = NULL;
    //interface->get_max_nb_ports           = NULL;
    //interface->msg_length_max             = NULL;
    //interface->copy_length_max            = mad_mx_cpy_limit_size;
    //interface->gather_scatter_length_max  = mad_mx_gather_scatter_length_max;
    //interface->need_rdv                   = mad_mx_need_rdv;
    //interface->buffer_need_rdv            = mad_mx_buffer_need_rdv;
    interface->isend               = mad_mx_isend;
    interface->irecv               = mad_mx_irecv;
    interface->s_test              = mad_mx_s_test;
    interface->r_test              = mad_mx_r_test;

    LOG_OUT();
    return "mx";
}

void
mad_mx_driver_init(p_mad_driver_t d, int *argc, char ***argv) {
    mx_return_t return_code	= MX_SUCCESS;
    LOG_IN();

    d->connection_type  = mad_bidirectional_connection;
    d->buffer_alignment = 32;

    /** contrôle de flux sur le nombre des unexpected stockés **/
    d->nb_unexpecteds = 10; //nombre maximal de messages unexpected
    d->cpy_threshold = MAD_MX_PRE_POSTED_SIZE;
    d->max_nb_ports = 4;
    d->need_new_msg = tbx_true;
    d->need_recv_msg = tbx_true;
    d->gather_scatter_available = tbx_true;
    d->rdma_available = tbx_false;


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
    p_mad_mx_adapter_specific_t	as = NULL;
    p_mad_driver_t driver          = NULL;
    int nb_unexpected = 0;
    p_mad_madeleine_t madeleine = NULL;
    int nb_dest = 0;
    //p_mad_iovec_t mad_iovec = NULL;
    //void *unexpected_area   = NULL;
    //int   i = 0;
    uint64_t                        nic_id;
    char                            hostname[MX_MAX_HOSTNAME_LEN];

    LOG_IN();
    as     = TBX_MALLOC(sizeof(mad_mx_adapter_specific_t));
    driver = a->driver;

    madeleine = mad_get_madeleine();
    nb_dest = tbx_slist_get_length(madeleine->dir->process_slist);
    nb_unexpected  = driver->nb_unexpecteds * nb_dest;

    //// Réservation des zones de données à pré-poster
    //tbx_malloc_init(&mad_mx_unexpected_key, MAD_MX_PRE_POSTED_SIZE, nb_unexpected, "mx_pre_posted");
    //
    //// Initialistaion des mad_iovecs à pré-poster
    //a->pre_posted  = mad_pipeline_create(nb_unexpected);
    //
    //for(i = 0; i < nb_unexpected; i++){
    //    unexpected_area = tbx_malloc(mad_mx_unexpected_key);
    //
    //    mad_iovec = mad_iovec_create(-1, NULL, 0, tbx_false, 0, 0);
    //    mad_iovec_add_data(mad_iovec,
    //                       unexpected_area,
    //                       MAD_MX_PRE_POSTED_SIZE);
    //    mad_pipeline_add(a->pre_posted, mad_iovec);
    //}

    as->nb_pre_posted_areas = nb_unexpected;

    // Identifiant de la carte
    if (strcmp(a->dir_adapter->name, "default")) {
        FAILURE("unsupported adapter");
    } else {
        as->board_id	= MX_ANY_NIC;
    }

    a->specific	= as;

    /* ID of the first Myrinet card */
    mx_board_number_to_nic_id(0, &nic_id);
    /* Hostname attached to this ID */
    mx_nic_id_to_hostname(nic_id, hostname);

    a->parameter	= tbx_strdup(hostname);

    LOG_OUT();
}

void
mad_mx_channel_init(p_mad_channel_t ch) {
    p_mad_adapter_t             adapter     = NULL;
    p_mad_mx_adapter_specific_t as          = NULL;
    p_tbx_string_t              param_str   = NULL;
    p_mad_track_set_t           s_track_set = NULL;
    uint32_t                    nb_track    = 0;
    p_mad_track_t              *tracks_tab  = NULL;
    p_mad_track_t               track       = NULL;
    p_mad_mx_port_specific_t   port_s          = NULL;
    uint32_t i = 0;
    LOG_IN();
    adapter     = ch->adapter;
    as          = adapter->specific;
    s_track_set = adapter->s_track_set;
    tracks_tab  = s_track_set->tracks_tab;
    nb_track    = s_track_set->nb_track;
    track       = tracks_tab[0];
    port_s      = track->local_ports[0]->specific;

    param_str = tbx_string_init_to_int(port_s->endpoint_id);
    for(i = 1; i < nb_track; i++){
        track = tracks_tab[i];
        port_s = track->local_ports[0]->specific;

        tbx_string_append_char(param_str, ' ');
        tbx_string_append_int (param_str, port_s->endpoint_id);
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
    in ->nb_link = 1;
    out->nb_link = 1;
    LOG_OUT();
}


void
mad_mx_track_init(p_mad_adapter_t adapter,
                  uint32_t track_id){
    p_mad_mx_adapter_specific_t as      = NULL;
    uint64_t                    nic_id  = 0;
    uint32_t                    nb_edp  = 0;

    p_mad_track_set_t           s_track_set = NULL;
    p_mad_track_t               s_track     = NULL;
    p_mad_port_t s_port = NULL;
    p_mad_mx_port_specific_t   s_ps        = NULL;


    p_mad_track_set_t           r_track_set = NULL;
    p_mad_track_t               r_track     = NULL;
    p_mad_port_t r_port = NULL;
    p_mad_mx_port_specific_t   r_ps        = NULL;

    mx_endpoint_t  endpoint;
    uint32_t       endpoint_id  = 1;
    const uint32_t endpoint_key = 0xFFFFFFFF;

    mx_return_t    rc    = MX_SUCCESS;
    LOG_IN();
    as = adapter->specific;
    nic_id = as->board_id;

#ifdef MX_VERSION_AFTER_1_0_0
    rc = mx_get_info(endpoint,
                     MX_MAX_NATIVE_ENDPOINTS,
                     NULL,
                     0,
                     &nb_edp,
                     sizeof(uint32_t));
#else // ! MX_VERSION_AFTER_1_0_0
    rc = mx_get_info(endpoint,
                     MX_MAX_NATIVE_ENDPOINTS,
                     &nb_edp,
                     sizeof(uint32_t));
#endif // MX_VERSION_AFTER_1_0_0
    mad_mx_check_return("mx_open_track", rc);

    s_track_set = adapter->s_track_set;
    s_track     = s_track_set->tracks_tab[track_id];
    s_port      = s_track->local_ports[0];

    s_ps = TBX_MALLOC(sizeof(mad_mx_port_specific_t));
    s_ps->endpoint      = TBX_MALLOC(sizeof(mx_endpoint_t));
    s_ps->endpoint_addr = TBX_MALLOC(sizeof(mx_endpoint_addr_t));
    s_ps->request       = TBX_MALLOC(sizeof(mx_request_t));
    s_port->specific = s_ps;



    r_track_set   = adapter->r_track_set;
    r_track       = r_track_set->tracks_tab[track_id];
    r_port = r_track->local_ports[0];

    r_ps          = TBX_MALLOC(sizeof(mad_mx_port_specific_t));
    r_ps->request = TBX_MALLOC(sizeof(mx_request_t));
    r_port->specific = r_ps;

    for(endpoint_id = 1, rc = MX_BUSY;
        rc == MX_BUSY && endpoint_id <= nb_edp;
        endpoint_id++){
        rc = mx_open_endpoint(nic_id,
                              endpoint_id,
                              endpoint_key,
                              NULL,
                              0,
                              s_ps->endpoint);
    }
    mad_mx_check_return("mx_open_track", rc);
    r_ps->endpoint = s_ps->endpoint;

    s_ps->endpoint_id = endpoint_id - 1;
    r_ps->endpoint_id = endpoint_id - 1;

    rc = mx_get_endpoint_addr(*s_ps->endpoint,
                              s_ps->endpoint_addr);
    mad_mx_check_return("mx_open_track", rc);
    r_ps->endpoint_addr = s_ps->endpoint_addr;

    //{
    //    static int rdv = 0;
    //
    //    if(!rdv){
    //        s_ps = s_track_set->cpy_track->local_ports[0]->specific;
    //        DISP_PTR("En emission, endpoint", s_ps->endpoint);
    //        r_ps = r_track_set->cpy_track->local_ports[0]->specific;
    //        DISP_PTR("En réception, endpoint", r_ps->endpoint);
    //        rdv ++;
    //
    //    } else if (rdv == 1) {
    //        s_ps = s_track_set->rdv_track->local_ports[0]->specific;
    //        DISP_PTR("En emission, endpoint", s_ps->endpoint);
    //        r_ps = r_track_set->rdv_track->local_ports[0]->specific;
    //        DISP_PTR("En réception, endpoint", r_ps->endpoint);
    //        rdv ++;
    //    }
    //
    //}
    //DISP("<--- TRACK_INIT");

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

    if(s_track->remote_ports[remote_rank]){
        goto end;
    } else {
        char	     *remote_hostname     = NULL;
        size_t	      remote_hostname_len = 0;
        uint64_t      remote_nic_id	     = 0;

        char *param = NULL;
        char *endpoint_id = NULL;
        char *ptr = NULL;

        p_mad_port_t s_port = NULL;
        p_mad_mx_port_specific_t s_ps = NULL;

        p_mad_port_t             remote_port = NULL;
        p_mad_mx_port_specific_t remote_ps = NULL;

        mx_return_t rc = MX_SUCCESS;
        uint32_t    i = 1;


        /* Hostname */
        remote_hostname_len	= strlen(ai->dir_adapter->parameter) + 2;
        remote_hostname	        = TBX_MALLOC(remote_hostname_len + 1);
        strcpy(remote_hostname, ai->dir_adapter->parameter);

        DISP("MX checking hostname %s", remote_hostname);
        /* Remote NIC id */
        rc = mx_hostname_to_nic_id(remote_hostname, &remote_nic_id);
        mad_mx_check_return("mx_hostname_to_nic_id", rc);

        /* Initialisation of the remote tracks */
        param = ai->channel_parameter;

        endpoint_id = strtok(param, " ");
        if (!endpoint_id || endpoint_id == ptr)
            FAILURE("invalid channel parameter");

        remote_ps = TBX_MALLOC(sizeof(mad_mx_port_specific_t));
        remote_ps->endpoint_id = strtol(endpoint_id, &ptr, 0);
        remote_ps->endpoint_addr = TBX_MALLOC(sizeof(mx_endpoint_addr_t));


        s_track  = s_track_set->tracks_tab[0];
        s_port   = s_track->local_ports[0];
        s_ps     = s_port->specific;

        *remote_ps->endpoint_addr = mad_mx_connect_endpoint(s_ps->endpoint,
                                                            remote_nic_id,
                                                            remote_ps->endpoint_id);

        remote_port = TBX_MALLOC(sizeof(mad_port_t));
        remote_port->specific = remote_ps;

        s_track->remote_ports[remote_rank] = remote_port;

        while (1){
            /* extract string from string sequence */
            endpoint_id = strtok(NULL, " ");
            if (endpoint_id == ptr)
                FAILURE("invalid channel parameter");
            /* check if there is nothing else to extract */
            if (!endpoint_id)
                break;

            remote_ps = TBX_MALLOC(sizeof(mad_mx_port_specific_t));
            remote_ps->endpoint_id = strtol(endpoint_id, &ptr, 0);
            remote_ps ->endpoint_addr = TBX_MALLOC(sizeof(mx_endpoint_addr_t));

            s_track  = s_track_set->tracks_tab[i];
            s_port     = s_track->local_ports[0];
            s_ps     = s_port->specific;

            *remote_ps->endpoint_addr = mad_mx_connect_endpoint(s_ps->endpoint,
                                                                remote_nic_id,
                                                                remote_ps->endpoint_id);

            remote_port = TBX_MALLOC(sizeof(mad_port_t));
            remote_port->specific = remote_ps;

            s_track->remote_ports[remote_rank] = remote_port;

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
    //p_mad_mx_adapter_specific_t as = NULL;
    //p_mad_pipeline_t pre_posted    = NULL;
    //p_mad_iovec_t    mad_iovec     = NULL;
    //int              nb_elm        = 0;
    //int i = 0;
    //LOG_IN();
    //
    //as         = a->specific;
    //pre_posted = a->pre_posted;
    //nb_elm     = as->nb_pre_posted_areas;
    //
    //// Libération des structures à pré-poster
    //for(i = 0; i < nb_elm; i++){
    //    mad_iovec = mad_pipeline_remove(pre_posted);
    //
    //    tbx_free(mad_mx_unexpected_key, mad_iovec->data[0].iov_base);
    //    mad_iovec_free(mad_iovec);
    //}
    //tbx_malloc_clean(mad_mx_unexpected_key);
    //
    //as	= a->specific;
    //TBX_FREE(as);
    //a->specific	  = NULL;
    //a->parameter  = NULL;
    //LOG_OUT();
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
    p_mad_channel_t     channel      = NULL;
    p_mad_adapter_t     adapter      = NULL;

    p_mad_track_set_t         s_track_set = NULL;
    p_mad_track_t             s_track     = NULL;
    p_mad_mx_port_specific_t  s_ps        = NULL;
    p_mad_port_t             r_port     = NULL;
    p_mad_mx_port_specific_t  r_ps        = NULL;

    mx_endpoint_t      *endpoint     = NULL;
    mx_endpoint_addr_t *dest_address = NULL;
    uint64_t            match_info   = 0;
    mx_return_t         rc           = MX_SUCCESS;
    mx_request_t        request;
    mx_status_t         status;
    uint32_t            result;
    LOG_IN();

    //DISP("-------------->new_message");

    channel = out->channel;
    adapter = channel->adapter;
    s_track_set = adapter->s_track_set;
    s_track  = s_track_set->cpy_track;

    s_ps     = s_track->local_ports[out->remote_rank]->specific;
    endpoint = s_ps->endpoint;

    r_port      = s_track->remote_ports[out->remote_rank];
    r_ps         = r_port->specific;
    dest_address = r_ps->endpoint_addr;

    match_info = ((uint64_t)channel->id + 1) << 32
        | channel->process_lrank;

    rc = mx_isend(*endpoint,
                  NULL, 0,
                  *dest_address,
                  match_info,
                  NULL, &request);
    do {
        rc = mx_test(*endpoint, &request, &status, &result);
    } while (rc == MX_SUCCESS && !result);

    //DISP("<--------------new_message");
    LOG_OUT();
}

p_mad_connection_t
mad_mx_receive_message(p_mad_channel_t ch) {
    p_mad_mx_channel_specific_t	chs		= NULL;
    p_mad_adapter_t             a  = NULL;
    p_mad_mx_adapter_specific_t as = NULL;
    p_mad_connection_t		in		= NULL;
    p_tbx_darray_t		in_darray = NULL;

    p_mad_track_set_t         r_track_set = NULL;
    p_mad_track_t             track = NULL;
    p_mad_mx_port_specific_t r_ps = NULL;


    mx_endpoint_t *ep         = NULL;
    uint64_t 	   match_info = 0;
    mx_return_t    rc         = MX_SUCCESS;
    mx_request_t   request;
    mx_status_t    status;
    uint32_t       result;
    LOG_IN();

    //DISP("-------------->receive_message");
    chs		= ch->specific;
    a           = ch->adapter;
    as          = a->specific;
    in_darray	= ch->in_connection_darray;
    match_info	= ((uint64_t)ch->id + 1) <<32;
    r_track_set = a->r_track_set;
    track = r_track_set->cpy_track;

    r_ps    = track->local_ports[0]->specific;
    ep    = r_ps->endpoint;

    //DISP_PTR("endpoint", ep);
    //DISP_PTR("request", request);

    rc	= mx_irecv(*ep,
                   NULL, 0,
                   match_info,
                   MX_MATCH_MASK_BC, NULL, &request);
    do {
        rc = mx_test(*ep, &request, &status, &result);
    } while (rc == MX_SUCCESS && !result);

    match_info = status.match_info;
    in	= tbx_darray_get(in_darray, match_info & 0xFFFFFFFF);
    if(!in){
        FAILURE("mad_mx_receive_message : connection not found");
    }

    //DISP("<--------------receive_message");

    LOG_OUT();
    return in;
}

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
//tbx_bool_t
//mad_mx_need_rdv(p_mad_iovec_t mad_iovec){
//    LOG_IN();
//    if(mad_iovec->length > MAD_MX_PRE_POSTED_SIZE)
//        return tbx_true;
//    LOG_OUT();
//    return tbx_false;
//}
//
//tbx_bool_t
//mad_mx_buffer_need_rdv(size_t buffer_length){
//    LOG_IN();
//    if(buffer_length > MAD_MX_PRE_POSTED_SIZE)
//        return tbx_true;
//    LOG_OUT();
//    return tbx_false;
//}
//
//uint32_t
//mad_mx_gather_scatter_length_max(void){
//    return MAD_MX_GATHER_SCATTER_THRESHOLD;
//}
//
//uint32_t
//mad_mx_cpy_limit_size(void){
//    return MAD_MX_PRE_POSTED_SIZE;
//}
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

tbx_bool_t
mad_mx_s_test(p_mad_track_set_t track_set){
    p_mad_mx_port_specific_t ps = NULL;
    p_mad_iovec_t              mad_iovec = NULL;
    p_mad_track_t track = NULL;


    mx_request_t             *request;
    mx_endpoint_t            *endpoint;

    mx_return_t  rc     = MX_SUCCESS;
    uint32_t     result	= 0;
    mx_status_t  status;
    LOG_IN();
    //DISP("-------------->s_test");

    mad_iovec = track_set->cur;
    track     = mad_iovec->track;

    ps = track->local_ports[mad_iovec->remote_rank]->specific;

    request  = ps->request;
    endpoint = ps->endpoint;

    //rc = mx_wait(*endpoint, request, MX_INFINITE, &status, &result);
    rc = mx_test(*endpoint, request, &status, &result);
    if(rc == MX_SUCCESS && result){
        TBX_GET_TICK(chrono_test);
        LOG_OUT();
        //DISP("<--------------s_test");
        return tbx_true;
    }
    LOG_OUT();

    //DISP("<--------------s_test");

    return tbx_false;
}

p_mad_iovec_t
mad_mx_r_test(p_mad_track_t track){
    p_mad_mx_port_specific_t ps = NULL;
    mx_request_t             *request;
    mx_endpoint_t            *endpoint;

    mx_return_t  rc     = MX_SUCCESS;
    uint32_t     result	= 0;
    mx_status_t  status;
    LOG_IN();

    //DISP("---->r_test");

    ps = track->local_ports[0]->specific;
    request  = ps->request;
    endpoint = ps->endpoint;

    //rc = mx_wait(*endpoint, request, MX_INFINITE, &status, &result);

    //DISP_PTR("endpoint", endpoint);
    //DISP_PTR("request", request);

    rc = mx_test(*endpoint, request, &status, &result);
    if(rc == MX_SUCCESS && result){
        TBX_GET_TICK(chrono_test);
        LOG_OUT();
        //DISP_VAL("<---r_test", ps->mad_iovec);
        return ps->mad_iovec;
    }


    //DISP("<---r_test");


    LOG_OUT();
    return NULL;
}


//void
//mad_mx_wait(p_mad_track_t track){
//    p_mad_mx_track_specific_t ts = NULL;
//    mx_request_t             *request;
//    mx_endpoint_t            *endpoint;
//
//    mx_return_t  rc = MX_SUCCESS;
//    mx_status_t  status;
//    uint32_t     result	= 0;
//    LOG_IN();
//    ts       = track->specific;
//    request  = ts->request;
//    endpoint = ts->endpoint;
//
//    do {
//        rc = mx_test(*(endpoint), request, &status, &result);
//    } while (rc == MX_SUCCESS && !result);
//    LOG_OUT();
//}


void
mad_mx_isend(p_mad_track_t track, p_mad_iovec_t mad_iovec){
    p_mad_mx_port_specific_t ps           = NULL;
    p_mad_port_t             remote_port = NULL;
    p_mad_mx_port_specific_t remote_ps    = NULL;

    mx_endpoint_t      *endpoint     = NULL;
    mx_endpoint_addr_t *dest_address = NULL;
    uint64_t            match_info   = 0;
    mx_segment_t       *seg_list     = NULL;
    mx_request_t       *request      = NULL;
    mx_return_t         rc           = MX_SUCCESS;
    uint32_t nb_seg = 0;

    LOG_IN();
    //DISP("---->isend");
    ps = track->local_ports[mad_iovec->remote_rank]->specific;
    endpoint = ps->endpoint;
    request  = ps->request;

    remote_port  = track->remote_ports[mad_iovec->remote_rank];
    remote_ps    = remote_port->specific;
    dest_address = remote_ps->endpoint_addr;
    seg_list     = (mx_segment_t *)mad_iovec->data;
    match_info   = 0;
    nb_seg = mad_iovec->total_nb_seg;

    //DISP_PTR("endpoint", endpoint);
    //DISP_PTR("request", request);

    track->track_set->cur = mad_iovec;


    rc = mx_isend(*endpoint,
                  seg_list, nb_seg,
                  *dest_address,
                  match_info,
                  NULL, request);

    //DISP("---->isend");
    LOG_OUT();
}

void
mad_mx_irecv(p_mad_track_t track, p_mad_iovec_t mad_iovec){
    p_mad_mx_port_specific_t ps         = NULL;
    mx_endpoint_t            *endpoint   = NULL;
    uint64_t                  match_info = 0;
    mx_segment_t             *seg_list   = NULL;
    mx_request_t             *request    = NULL;
    mx_return_t               rc         = MX_SUCCESS;
    uint32_t nb_seg = 0;
    LOG_IN();

    //DISP("--->mad_mx_irecv");
    ps = track->local_ports[0]->specific;
    endpoint = ps->endpoint;
    request  = ps->request;
    seg_list = (mx_segment_t *)mad_iovec->data;
    match_info = 0;
    nb_seg = mad_iovec->total_nb_seg;


    //DISP_PTR("endpoint", endpoint);
    //DISP_PTR("request", request);

    rc	= mx_irecv(*endpoint,
                   seg_list, nb_seg,
                   match_info,
                   MX_MATCH_MASK_NONE, NULL,
                   request);

    ps->mad_iovec        = mad_iovec;

    track->pending_reception[mad_iovec->remote_rank] = mad_iovec;
    track->nb_pending_reception++;
    track->track_set->reception_tracks_in_use[track->id] = tbx_true;

    //DISP("<---mad_mx_irecv");
    LOG_OUT();
}

void
mad_mx_init_pre_posted(p_mad_adapter_t adapter,
                       p_mad_track_t track){
    p_mad_track_set_t r_track_set = NULL;
    p_mad_iovec_t mad_iovec = NULL;
    ntbx_process_lrank_t my_rank = -1;
    int nb_dest = 0;
    int i = 0;
    p_mad_madeleine_t madeleine = NULL;
    p_mad_session_t     session = NULL;

    LOG_IN();
    //DISP("-->mad_mx_init_pre_posted");
    madeleine = mad_get_madeleine();
    session = madeleine->session;
    my_rank = session->process_rank;

    r_track_set = adapter->r_track_set;
    nb_dest = track->nb_dest;

    //DISP_VAL("init_pre_posted, nb_dest", nb_dest);

    for(i = 0; i < nb_dest; i++){
        if(i  != my_rank){
            mad_iovec = mad_pipeline_remove(adapter->pre_posted);
            mad_iovec->track = track;
            mad_iovec->remote_rank = i;

            track->pending_reception[i] = mad_iovec;

            r_track_set->nb_pending_reception++;
            track->nb_pending_reception++;

            mad_mx_irecv(track, mad_iovec);
        }
    }

    r_track_set->reception_tracks_in_use[track->id] = tbx_true;
    r_track_set->status = MAD_MKP_PROGRESS;
    //DISP("<--mad_mx_init_pre_posted");
    LOG_OUT();
}


void
mad_mx_replace_pre_posted(p_mad_adapter_t adapter,
                          p_mad_track_t track,
                          int port_id){
    p_mad_iovec_t mad_iovec = NULL;
    p_mad_track_set_t r_track_set = NULL;
    //DISP("---------6> replace pre posted");

    r_track_set = adapter->r_track_set;

    mad_iovec   = mad_pipeline_remove(adapter->pre_posted);
    mad_iovec->track       = track;
    mad_iovec->remote_rank = port_id;

    track->pending_reception[port_id] = mad_iovec;

    r_track_set->nb_pending_reception++;
    track->nb_pending_reception++;

    mad_mx_irecv(track, mad_iovec);

    r_track_set->reception_tracks_in_use[track->id] = tbx_true;
    r_track_set->status = MAD_MKP_PROGRESS;

    //DISP("<--------- replace pre posted");
    LOG_OUT();
}

void
mad_mx_remove_all_pre_posted(p_mad_adapter_t adapter){
    p_mad_track_set_t r_track_set = NULL;
    p_mad_track_t     cpy_track       = NULL;
    p_mad_iovec_t     mad_iovec   = NULL;
    int i = 0;
    LOG_IN();
    //DISP("-->remove pre posted");

    r_track_set = adapter->r_track_set;
    cpy_track   = r_track_set->cpy_track;

    for(i = 0; i < cpy_track->nb_dest; i++){
        mad_iovec   = cpy_track->pending_reception[i];
        cpy_track->pending_reception[i] = NULL;

        if(mad_iovec){
            mad_pipeline_add(adapter->pre_posted, mad_iovec);
        }
    }

    //DISP("<--remove pre posted");
    LOG_OUT();
}

void
mad_mx_track_exit(p_mad_track_t track){
    //return_code	= mx_close_endpoint(as->endpoint1);
}
