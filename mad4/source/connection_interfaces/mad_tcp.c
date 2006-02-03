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
 * Mad_tcp.c
 * =========
 */
#include "madeleine.h"
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>



/*
 * local structures
 * ----------------
 */
typedef struct s_mad_tcp_driver_specific{
    int dummy;
} mad_tcp_driver_specific_t, *p_mad_tcp_driver_specific_t;

typedef struct s_mad_tcp_adapter_specific{
    int  connection_socket;
    int  connection_port;

    uint32_t nb_pre_posted_areas;
} mad_tcp_adapter_specific_t, *p_mad_tcp_adapter_specific_t;

typedef struct s_mad_tcp_channel_specific{
    int dummy;
} mad_tcp_channel_specific_t, *p_mad_tcp_channel_specific_t;

typedef struct s_mad_tcp_connection_specific{
    struct iovec new_msg;
} mad_tcp_connection_specific_t, *p_mad_tcp_connection_specific_t;

typedef struct s_mad_tcp_track_set_specific{
    int dummy;
}mad_tcp_track_set_specific_t, *p_mad_tcp_track_set_specific_t;

typedef struct s_mad_tcp_track_specific{
    struct pollfd *to_poll;
    unsigned int   nb_polled_sock;
    int            delay;
    int            last_polled_idx;

    int            unexpected_len; // pour l'emission
}mad_tcp_track_specific_t, *p_mad_tcp_track_specific_t;

typedef struct s_mad_tcp_port_specific{
    int desc;

    struct iovec iovec[NB_ENTRIES + 1];

    size_t        nb_seg;
    int           current_seg_id;
    int           nb_treated_bytes;
    int           waiting_length;

    p_mad_iovec_t mad_iovec;

    int            unexpected_len; // pour la réception
}mad_tcp_port_specific_t, *p_mad_tcp_port_specific_t;

#define MAD_TCP_PRE_POSTED_SIZE  2048  // A CHANGER : taille data + taille GH (4) + taille DH (8)

static p_tbx_memory_t mad_tcp_unexpected_key;

/*
 * Registration function
 * ---------------------
 */

char *
mad_tcp_register(p_mad_driver_interface_t interface)
{
    LOG_IN();
    TRACE("Registering TCP driver");

    interface->driver_init            = mad_tcp_driver_init;
    interface->adapter_init           = mad_tcp_adapter_init;
    interface->channel_init           = NULL;
    interface->connection_init        = mad_tcp_connection_init;
    interface->link_init              = NULL;
    interface->track_set_init         = NULL;
    interface->track_init             = mad_tcp_track_init;

    interface->before_open_channel    = NULL;
    interface->accept                 = mad_tcp_accept;
    interface->connect                = mad_tcp_connect;
    interface->after_open_channel     = NULL;

    interface->before_close_channel   = NULL;
    interface->disconnect             = mad_tcp_disconnect;
    interface->after_close_channel    = NULL;
    interface->link_exit              = NULL;
    interface->connection_exit        = mad_tcp_connection_exit;
    interface->channel_exit           = NULL;
    interface->adapter_exit           = mad_tcp_adapter_exit;
    interface->driver_exit            = NULL;
    interface->track_set_exit         = NULL;
    interface->track_exit             = mad_tcp_track_exit;

    interface->new_message            = mad_tcp_new_message;
    interface->finalize_message       = NULL;
    interface->receive_message        = mad_tcp_receive_message;
    interface->message_received       = NULL;

    interface->init_pre_posted        = mad_tcp_init_pre_posted;
    interface->replace_pre_posted     = mad_tcp_replace_pre_posted;
    interface->remove_all_pre_posted  = mad_tcp_remove_all_pre_posted;

    interface->rdma_available             = NULL;
    interface->gather_scatter_available   = NULL;
    interface->get_max_nb_ports           = NULL;
    interface->msg_length_max             = NULL;
    interface->copy_length_max            = NULL;
    interface->gather_scatter_length_max  = NULL;
    interface->need_rdv                   = mad_tcp_need_rdv;
    interface->buffer_need_rdv            = mad_tcp_buffer_need_rdv;

    interface->isend                      = mad_tcp_isend;
    interface->irecv                      = mad_tcp_irecv;
    interface->send                       = NULL;
    interface->recv                       = NULL;
    interface->s_test                     = mad_tcp_s_test;
    interface->r_test                     = mad_tcp_r_test;
    interface->wait                       = NULL;

    LOG_OUT();
    return "tcp";
}

void
mad_tcp_driver_init(p_mad_driver_t driver, int *argc, char ***argv) {
    LOG_IN();
    TRACE("Initializing TCP driver");
    driver->connection_type  = mad_bidirectional_connection;
    driver->buffer_alignment = 32;

    /** contrôle de flux sur le nombre des unexpected stockés **/
    // nombre maximal de messages unexpected
    driver->nb_unexpecteds                = 10;
    /*****/

    LOG_OUT();
}

void
mad_tcp_adapter_init(p_mad_adapter_t adapter) {
    p_mad_driver_t driver = NULL;
    p_mad_tcp_adapter_specific_t adapter_specific = NULL;
    p_tbx_string_t               parameter_string = NULL;
    ntbx_tcp_address_t           address;

    int total                    = 0;
    void *unexpected_area   = NULL;
    p_mad_iovec_t mad_iovec = NULL;
    int   i = 0;
    LOG_IN();
    if (strcmp(adapter->dir_adapter->name, "default"))
        FAILURE("unsupported adapter");


    driver = adapter->driver;
    adapter_specific  = TBX_MALLOC(sizeof(mad_tcp_adapter_specific_t));

    /** Initialisation de la socket serveur **/
    adapter_specific->connection_socket = ntbx_tcp_socket_create(&address, 0);
    SYSCALL(listen(adapter_specific->connection_socket,
                   tbx_min(5, SOMAXCONN)));

    adapter_specific->connection_port =
        (ntbx_tcp_port_t)ntohs(address.sin_port);




    /** Préparation des mad_iovecs destinés aux messages unexpected **/
    total = driver->nb_unexpecteds;
    // Réservation des zones de données à pré-poster
    tbx_malloc_init(&mad_tcp_unexpected_key, MAD_TCP_PRE_POSTED_SIZE, total, "tcp_pre_posted");

    // Initialistaion des mad_iovecs à pré-poster
    adapter->pre_posted  = mad_pipeline_create(total);

    for(i = 0; i < total; i++){
        unexpected_area = tbx_malloc(mad_tcp_unexpected_key);

        mad_iovec = mad_iovec_create(-1, NULL, 0, tbx_false, 0, 0);
        mad_iovec_add_data(mad_iovec,
                           unexpected_area,
                           MAD_TCP_PRE_POSTED_SIZE);
        mad_pipeline_add(adapter->pre_posted, mad_iovec);
    }
    adapter_specific->nb_pre_posted_areas = total;

    adapter->specific = adapter_specific;

    /** Paramètre à échanger par léonie **/
    parameter_string   = tbx_string_init_to_int(adapter_specific->connection_port);
    adapter->parameter = tbx_string_to_cstring(parameter_string);
    tbx_string_free(parameter_string);
    parameter_string   = NULL;
    LOG_OUT();
}

void
mad_tcp_connection_init(p_mad_connection_t in,
			p_mad_connection_t out) {
    p_mad_tcp_connection_specific_t specific = NULL;

    LOG_IN();
    specific = TBX_MALLOC(sizeof(mad_tcp_connection_specific_t));
    specific->new_msg.iov_base = TBX_MALLOC(MAD_TCP_PRE_POSTED_SIZE);
    memset(specific->new_msg.iov_base, 0, MAD_TCP_PRE_POSTED_SIZE);
    specific->new_msg.iov_len = MAD_TCP_PRE_POSTED_SIZE;


    in->specific = out->specific = specific;
    in->nb_link  = 1;
    out->nb_link = 1;
    LOG_OUT();
}

void
mad_tcp_track_init(p_mad_adapter_t adapter,
                   uint32_t track_id) {
    int nb_dest = 0;

    p_mad_track_set_t           s_track_set = NULL;
    p_mad_track_t               s_track     = NULL;
    p_mad_tcp_track_specific_t  s_ts        = NULL;
    p_mad_port_t                s_port      = NULL;
    p_mad_tcp_port_specific_t   s_port_s    = NULL;

    p_mad_track_set_t           r_track_set = NULL;
    p_mad_track_t               r_track     = NULL;
    p_mad_tcp_track_specific_t  r_ts        = NULL;
    p_mad_port_t                r_port      = NULL;
    p_mad_tcp_port_specific_t   r_port_s    = NULL;

    int i = 0;

    LOG_IN();
    /** Emission **/
    s_track_set = adapter->s_track_set;
    s_track     = s_track_set->tracks_tab[track_id];
    nb_dest     = s_track->nb_dest;

    //DISP_VAL("-->open_track, nb_ports", nb_dest);

    for(i = 0; i < nb_dest; i++){
        s_port           = s_track->local_ports[i];
        s_port->specific = TBX_MALLOC(sizeof(mad_tcp_port_specific_t));
        s_port_s = s_port->specific;
        s_port_s->desc = -1;
        s_port_s->waiting_length = 0;
        s_port_s->nb_treated_bytes = 0;
    }

    s_ts           = TBX_MALLOC(sizeof(mad_tcp_track_specific_t));
    s_ts->to_poll  = TBX_MALLOC(nb_dest * sizeof(struct pollfd));
    s_ts->nb_polled_sock = 0;
    s_ts->delay          = 0;

    s_ts->last_polled_idx = 0;
    s_ts->unexpected_len = 7689;

    s_track->specific = s_ts;



    /** Réception **/
    r_track_set   = adapter->r_track_set;
    r_track       = r_track_set->tracks_tab[track_id];
    nb_dest       = r_track->nb_dest;

    for(i = 0; i < nb_dest; i++){
        r_port = r_track->local_ports[i];
        r_port->specific = TBX_MALLOC(sizeof(mad_tcp_port_specific_t));
        r_port_s = r_port->specific;
        r_port_s->desc = -1;
        r_port_s->waiting_length = 0;
        r_port_s->nb_treated_bytes = 0;
        r_port_s->unexpected_len = 4329;
    }

    r_ts                 = TBX_MALLOC(sizeof(mad_tcp_track_specific_t));
    r_ts->to_poll        = TBX_MALLOC(nb_dest * sizeof(struct pollfd));
    r_ts->nb_polled_sock = 0;
    r_ts->delay          = 0;

    r_ts->last_polled_idx = 0;
    r_ts->unexpected_len = 48768;

    r_track->specific = r_ts;

    //DISP("<--open_track");
    LOG_OUT();
}


void
mad_tcp_accept(p_mad_connection_t   in,
	       p_mad_adapter_info_t adapter_info TBX_UNUSED) {
    p_mad_adapter_t               adapter = NULL;
    p_mad_tcp_adapter_specific_t  adapter_specific    = NULL;

    uint32_t                   nb_track = 0;
    p_mad_track_t             *s_tracks_tab = NULL;
    p_mad_track_t             *r_tracks_tab = NULL;

    p_mad_track_set_t          s_track_set = NULL;
    p_mad_track_t              s_track = NULL;
    p_mad_tcp_track_specific_t s_ts = NULL;
    p_mad_tcp_port_specific_t  s_port_s = NULL;

    p_mad_track_set_t          r_track_set = NULL;
    p_mad_track_t              r_track = NULL;
    p_mad_tcp_track_specific_t r_ts = NULL;
    p_mad_tcp_port_specific_t  r_port_s = NULL;

    ntbx_tcp_socket_t          desc                =   -1;
    int i = 0;

    int remote = 0;
    int no = 0;
    LOG_IN();
    //DISP("-->accept");

    adapter          = in->channel->adapter;
    adapter_specific = adapter->specific;

    remote = in->remote_rank;

    s_track_set          = adapter->s_track_set;
    r_track_set = adapter->r_track_set;

    nb_track      = s_track_set->nb_track;
    s_tracks_tab  = s_track_set->tracks_tab;
    r_tracks_tab  = r_track_set->tracks_tab;

    for(i = 0; i < nb_track; i++){
        s_track      = s_tracks_tab[i];
        s_ts         = s_track->specific;
        s_port_s     = s_track->local_ports[remote]->specific;

        //DISP_VAL("s_ts - unexpectde_len", s_ts->unexpected_len);



        r_track      = r_tracks_tab[i];
        r_ts         = r_track->specific;
        r_port_s     = r_track->local_ports[remote]->specific;

        // test si on a déjà initialisé la connexion
        if(s_port_s->desc != -1)
            return;

        SYSCALL(desc = accept(adapter_specific->connection_socket,
                              NULL, NULL));
        ntbx_tcp_socket_setup(desc);

        SYSCALL(setsockopt(desc, SOL_TCP, TCP_NODELAY, &no,
                           sizeof(no)));

        SYSCALL(fcntl(desc, F_SETFL, O_NONBLOCK));

        s_port_s->desc          = desc;
        s_ts->to_poll[remote].fd            = desc;
        s_ts->to_poll[remote].events        = POLLOUT;
        s_ts->to_poll[remote].revents       = 600;
        s_ts->nb_polled_sock = (s_ts->nb_polled_sock > remote + 1 ? s_ts->nb_polled_sock : remote + 1);

        r_port_s->desc = desc;
        r_ts->to_poll[remote].fd            = desc;
        r_ts->to_poll[remote].events        = POLLIN;
        r_ts->to_poll[remote].revents       = 500;
        r_ts->nb_polled_sock = (r_ts->nb_polled_sock > remote + 1 ? r_ts->nb_polled_sock : remote + 1);

        //DISP_VAL("ACCEPT : de ma socket", desc);
        //DISP_VAL("track_id", i);
        //DISP_VAL("remote", remote);
        //DISP("En émission");
        //DISP_VAL("s_ts->nb_polled_sock", s_ts->nb_polled_sock);
        //DISP_VAL("to_poll[remote].fd    ", s_ts->to_poll[remote].fd);
        //DISP_VAL("to_poll[remote].events", s_ts->to_poll[remote].events);
        //DISP_VAL("to_poll[remote].revents", s_ts->to_poll[remote].revents);
        //DISP("En réception");
        //DISP_VAL("r_ts->nb_polled_sock", r_ts->nb_polled_sock);
        //DISP_VAL("to_poll[remote].fd    ", r_ts->to_poll[remote].fd);
        //DISP_VAL("to_poll[remote].events", r_ts->to_poll[remote].events);
        //DISP_VAL("to_poll[remote].revents", r_ts->to_poll[remote].revents);
        //DISP("");
    }

    //DISP("<--accept");
    LOG_OUT();
}

void
mad_tcp_connect(p_mad_connection_t   out,
		p_mad_adapter_info_t adapter_info) {
    p_mad_dir_node_t           remote_node           = NULL;
    p_mad_dir_adapter_t        remote_adapter        = NULL;
    ntbx_tcp_port_t            remote_port           =    0;
    ntbx_tcp_socket_t          desc                  =   -1;
    ntbx_tcp_address_t         remote_address;

    p_mad_adapter_t            adapter = NULL;
    p_mad_track_set_t          s_track_set  = NULL;
    p_mad_track_set_t          r_track_set  = NULL;
    uint32_t                   nb_track     = 0;
    p_mad_track_t             *s_tracks_tab   = NULL;
    p_mad_track_t             *r_tracks_tab   = NULL;

    p_mad_track_t              s_track  = NULL;
    p_mad_tcp_track_specific_t s_ts     = NULL;
    p_mad_tcp_port_specific_t  s_port_s = NULL;

    p_mad_track_t              r_track  = NULL;
    p_mad_tcp_track_specific_t r_ts     = NULL;
    p_mad_tcp_port_specific_t  r_port_s = NULL;

    int i = 0;
    int remote = 0;
    int no = 0;

    LOG_IN();

    //DISP("-->connect");


    remote_node    = adapter_info->dir_node;
    remote_adapter = adapter_info->dir_adapter;
    remote_port    = strtol(remote_adapter->parameter,
                            (char **)NULL, 10);

    adapter     = out->channel->adapter;
    s_track_set = adapter->s_track_set;
    r_track_set = adapter->r_track_set;

    s_tracks_tab = s_track_set->tracks_tab;
    r_tracks_tab = r_track_set->tracks_tab;

    nb_track = s_track_set->nb_track;

    remote         = out->remote_rank;


    for(i = 0; i < nb_track; i++){
        s_track        = s_tracks_tab[i];
        s_ts           = s_track->specific;
        s_port_s       = s_track->local_ports[remote]->specific;

        r_track        = r_tracks_tab[i];
        r_ts           = r_track->specific;
        r_port_s       = r_track->local_ports[remote]->specific;

        // test si on a déjà initialisé la connexion
        if(s_port_s->desc != -1)
            return;

        desc = ntbx_tcp_socket_create(NULL, 0);

#ifndef LEO_IP
        ntbx_tcp_address_fill   (&remote_address, remote_port, remote_node->name);
#else // LEO_IP
        ntbx_tcp_address_fill_ip(&remote_address, remote_port, &remote_node->ip);
#endif // LEO_IP

        SYSCALL(connect(desc, (struct sockaddr *)&remote_address,
                        sizeof(ntbx_tcp_address_t)));
        ntbx_tcp_socket_setup(desc);

        SYSCALL(setsockopt(desc, SOL_TCP, TCP_NODELAY, &no,
                           sizeof(no)));

        SYSCALL(fcntl(desc, F_SETFL, O_NONBLOCK));

        s_port_s->desc = desc;
        r_port_s->desc = desc;

        s_ts->to_poll[remote].fd            = desc;
        s_ts->to_poll[remote].events        = POLLOUT;
        s_ts->to_poll[remote].revents       = 1000;
        s_ts->nb_polled_sock = (s_ts->nb_polled_sock > remote + 1 ? s_ts->nb_polled_sock : remote + 1);

        r_ts->to_poll[remote].fd            = desc;
        r_ts->to_poll[remote].events        = POLLIN;
        r_ts->to_poll[remote].revents       = 2000;
        r_ts->nb_polled_sock = (r_ts->nb_polled_sock > remote + 1 ? r_ts->nb_polled_sock : remote + 1);

        //DISP_VAL("CONNECT : de ma socket", desc);
        //DISP_VAL("track_id", i);
        //DISP_VAL("remote", remote);
        //DISP("En émission");
        //DISP_VAL("s_ts->nb_polled_sock", s_ts->nb_polled_sock);
        //DISP_VAL("to_poll[remote].fd    ", s_ts->to_poll[remote].fd);
        //DISP_VAL("to_poll[remote].events", s_ts->to_poll[remote].events);
        //DISP_VAL("to_poll[remote].revents", s_ts->to_poll[remote].revents);
        //DISP("En réception");
        //DISP_VAL("r_ts->nb_polled_sock", r_ts->nb_polled_sock);
        //DISP_VAL("to_poll[remote].fd    ", r_ts->to_poll[remote].fd);
        //DISP_VAL("to_poll[remote].events", r_ts->to_poll[remote].events);
        //DISP_VAL("to_poll[remote].revents", r_ts->to_poll[remote].revents);
        //DISP("");
    }

    //DISP("<--connect");
    LOG_OUT();
}


/********************************************************/
/********************************************************/
/********************************************************/
/********************************************************/
void
mad_tcp_disconnect(p_mad_connection_t connection) {
    //p_mad_tcp_connection_specific_t cs = connection->specific;

    LOG_IN();
    //SYSCALL(close(cs->socket));
    //connection_specific->socket = -1;
    LOG_OUT();
}

void
mad_tcp_connection_exit(p_mad_connection_t in,
			p_mad_connection_t out){
    p_mad_tcp_connection_specific_t specific = NULL;
    LOG_IN();
    specific = in->specific;

    TBX_FREE(specific->new_msg.iov_base);
    TBX_FREE(specific);

    LOG_OUT();
}


void
mad_tcp_adapter_exit(p_mad_adapter_t a) {
    p_mad_tcp_adapter_specific_t as = NULL;
    p_mad_pipeline_t pre_posted    = NULL;
    p_mad_iovec_t    mad_iovec     = NULL;
    int              nb_elm        = 0;
    int i = 0;
    LOG_IN();

    as         = a->specific;
    pre_posted = a->pre_posted;
    nb_elm     = as->nb_pre_posted_areas;

    // Libération des structures à pré-poster
    for(i = 0; i < nb_elm; i++){
        mad_iovec = mad_pipeline_remove(pre_posted);

        tbx_free(mad_tcp_unexpected_key, mad_iovec->data[0].iov_base);
        mad_iovec_free(mad_iovec);
    }
    tbx_malloc_clean(mad_tcp_unexpected_key);

    as	= a->specific;
    TBX_FREE(as);
    a->specific	  = NULL;
    a->parameter  = NULL;
    LOG_OUT();
}


void
mad_tcp_track_exit(p_mad_track_t track){
    p_mad_tcp_track_specific_t track_s = NULL;
    p_mad_port_t               port    = NULL;
    int nb_dest = 0;
    int i =0;

    LOG_IN();
    nb_dest = track->nb_dest;

    for(i = 0; i < nb_dest; i++){
        port = track->local_ports[i];

        TBX_FREE(port->specific);
    }

    track_s = track->specific;

    TBX_FREE(track_s->to_poll);
    TBX_FREE(track_s);
    LOG_OUT();
}
/********************************************************/
/********************************************************/
/********************************************************/
/********************************************************/
void
mad_tcp_new_message(p_mad_connection_t out){
    p_mad_channel_t     channel      = NULL;
    p_mad_adapter_t     adapter      = NULL;

    p_mad_track_set_t         s_track_set = NULL;
    p_mad_track_t             track     = NULL;
    p_mad_tcp_track_specific_t ts        = NULL;

    int destination = -1;
    p_mad_port_t              port    = NULL;
    p_mad_tcp_port_specific_t port_s   = NULL;

    int written = 0;
    int waiting_length = 0;
    int nb_treated_bytes = 0;
    struct iovec iovec;

    p_mad_tcp_connection_specific_t os = NULL;

    LOG_IN();
    //DISP("--->new_message");
    channel     = out->channel;
    adapter     = channel->adapter;
    s_track_set = adapter->s_track_set;
    track       = s_track_set->cpy_track;
    ts          = track->specific;

    destination = out->remote_rank;
    port        = track->local_ports[destination];
    port_s      = port->specific;

    os = out->specific;
    iovec = os->new_msg;

    //DISP_VAL("NEW_MSG: deja ecrits", nb_treated_bytes);
    //DISP_VAL("         reste à écrire", iovec.iov_len);

    waiting_length = MAD_TCP_PRE_POSTED_SIZE;

    while(nb_treated_bytes != waiting_length){
        // J'attends que la socket soit disponible en écriture
        while(1){
            SYSCALL(poll(ts->to_poll, ts->nb_polled_sock, -1));

            if(ts->to_poll[destination].revents & POLLOUT){
                //DISP("J'ai le droit d'écrire!!!");
                break;
            }
        }

        // J'écris
        SYSCALL(written = writev(port_s->desc, &iovec, 1));

        // Mis à jour de la zone à transmettre
        if(written){
            nb_treated_bytes += written;
            iovec.iov_base += written;
            iovec.iov_len  -= written;

            //DISP_VAL("new_msg - nb ecrits", sent);
            //DISP_VAL("          deja ecrits", nb_treated_bytes - sent);
            //DISP_VAL("          reste à écrire", iovec.iov_len);
        }
    }

    //DISP_VAL("new_msg - nb write", cnt);

    iovec.iov_base -= nb_treated_bytes;
    iovec.iov_len  += nb_treated_bytes;

    //DISP("<---new_message");
    LOG_OUT();
}



p_mad_connection_t
mad_tcp_receive_message(p_mad_channel_t ch) {
    p_mad_tcp_channel_specific_t	chs		= NULL;
    p_mad_adapter_t             a  = NULL;
    p_mad_tcp_adapter_specific_t as = NULL;
    p_mad_connection_t		in		= NULL;
    p_tbx_darray_t		in_darray = NULL;

    p_mad_track_t             track = NULL;
    p_mad_tcp_track_specific_t ts = NULL;

    p_mad_port_t              port = NULL;
    p_mad_tcp_port_specific_t port_s = NULL;

    p_mad_iovec_t mad_iovec = NULL;
    int i = 0;
    int j = 0;
    int nb_read = 0;
    int nb_polled_sock = 0;
    int nb_treated_bytes = 0;
    int waiting_length = 0;

    void * data = NULL;
    int len = 0;

    LOG_IN();
    //DISP("--->receive_message");
    chs		= ch->specific;
    a           = ch->adapter;
    as          = a ->specific;
    in_darray	= ch->in_connection_darray;

    track          = a->r_track_set->cpy_track;
    ts             = track->specific;
    nb_polled_sock = ts->nb_polled_sock;

    i = nb_polled_sock;
    while (i == nb_polled_sock){
        // détection des sockets prêtes à recevoir
        SYSCALL(poll(ts->to_poll, nb_polled_sock, -1));

        // tourniquet identifiant la prochaine socket à utiliser
        for(i = 0, j = (ts->last_polled_idx + 1) % nb_polled_sock;
            i < nb_polled_sock;
            i++, j = (j+1) % nb_polled_sock){

            // si revents de la structure pollfd == POLLIN alors
            // il y a des données disponibles en lecture sur la socket
            if(ts->to_poll[j].revents & POLLIN){
                //DISP_VAL("Je peux lire la source", j);
                break;
            }
        }
    }

    ts->last_polled_idx = j;

    port           = track->local_ports[j];
    port_s         = port->specific;
    waiting_length = MAD_TCP_PRE_POSTED_SIZE;

    mad_iovec = mad_pipeline_remove(a->pre_posted);
    mad_iovec->data[0].iov_len = waiting_length;

    //sauvegarde de l'iovec
    data = mad_iovec->data[0].iov_base;
    len  = mad_iovec->data[0].iov_len;


    //DISP_VAL("waiting_length", len);
    //DISP_VAL("waiting_length", waiting_length);
    //DISP_VAL("nb_treated bytes", nb_treated_bytes);
    //DISP_PTR(" avant iov_base", mad_iovec->data[0].iov_base);


    //DISP_VAL("recv_msg - nb lus", sent);
    //DISP_VAL("           deja lus", nb_treated_bytes);
    //DISP_VAL("          reste à lire", mad_iovec->data[0].iov_len);



    while(1){
        SYSCALL(nb_read = readv(port_s->desc, mad_iovec->data, 1));

        if(nb_read){
            nb_treated_bytes += nb_read;
            mad_iovec->data[0].iov_base += nb_read;
            mad_iovec->data[0].iov_len  -= nb_read;

            //static int i = 0;
            //if(i < 5){
            //    DISP_VAL("recv_msg - nb lus", sent);
            //    DISP_VAL("           deja lus", nb_treated_bytes - sent);
            //    DISP_VAL("          reste à lire", mad_iovec->data[0].iov_len);
            //    i++;
            //}
        }

        if(nb_treated_bytes == waiting_length){
            break;
        } else {
            while(1){
                SYSCALL(poll(ts->to_poll, nb_polled_sock, -1));

                // si revents de la structure pollfd == POLLIN
                // alors données disponibles en lecture
                if(ts->to_poll[j].revents & POLLIN){
                    //DISP("Je peux lire");
                    break;
                }
            }
        }
    }

    in = tbx_darray_get(in_darray, j);
    if(!in){
        FAILURE("mad_tcp_receive_message : connection not found");
    }

    mad_iovec->data[0].iov_base = data;
    mad_iovec->data[0].iov_len = len;

    //DISP_PTR("apres iov_base", mad_iovec->data[0].iov_base);

    mad_pipeline_add(a->pre_posted, mad_iovec);

    //DISP("<---receive_message");
    LOG_OUT();
    return in;
}

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
tbx_bool_t
mad_tcp_need_rdv(p_mad_iovec_t mad_iovec){
    LOG_IN();
    if(mad_iovec->length > (MAD_TCP_PRE_POSTED_SIZE - 12)) // TODO
        return tbx_true;
    LOG_OUT();
    return tbx_false;
}

tbx_bool_t
mad_tcp_buffer_need_rdv(size_t buffer_length){
    LOG_IN();

    //if(buffer_length + MAD_IOVEC_DATA_HEADER_SIZE > MAD_TCP_PRE_POSTED_SIZE) ???
    if(buffer_length > (MAD_TCP_PRE_POSTED_SIZE - 12)) //TODO
        return tbx_true;
    LOG_OUT();
    return tbx_false;
}


/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

tbx_bool_t
mad_tcp_s_test(p_mad_track_set_t track_set) {
    p_mad_tcp_track_specific_t ts = NULL;
    p_mad_iovec_t              mad_iovec = NULL;

    p_mad_port_t          port = NULL;
    p_mad_tcp_port_specific_t port_s = NULL;
    struct pollfd *polled;

    int written = 0;

    p_mad_track_t track = NULL;

    LOG_IN();
    //DISP("-->s_test");

    mad_iovec = track_set->cur;
    track     = mad_iovec->track;
    ts        = track->specific;


    SYSCALL(poll(ts->to_poll, ts->nb_polled_sock, 0));

    polled = &(ts->to_poll[mad_iovec->remote_rank]);

    // si struct pollfd.revents == POLLOUT
    // alors je peux écrire sur la socket
    if(polled->revents & POLLOUT){
        port   = track->local_ports[mad_iovec->remote_rank];
        port_s = port->specific;


        DISP_VAL("###############WRITE : len ", *((int *)port_s->iovec[0].iov_base));
        DISP_VAL("###############WRITE : nb_seg ", port_s->nb_seg);

        //DISP("WRITE");
        SYSCALL(written = writev(port_s->desc, port_s->iovec, port_s->nb_seg));

        if(written){
            //DISP_VAL("s_test - written    ", written);
            //DISP_VAL("s_test - déjà ecrits", port_s->nb_treated_bytes);
            //DISP_VAL("s_test - attendus   ", port_s->waiting_length);
            //DISP("");

            port_s->nb_treated_bytes += written;

            if(port_s->nb_treated_bytes == port_s->waiting_length){
                port_s->nb_treated_bytes = 0;
                port_s->nb_seg = 0;
                port_s->current_seg_id = 0;
                //DISP("ENVOYE!!!!");
                return tbx_true;

            } else {
                int to_write = 0;
                int current_seg_id = port_s->current_seg_id;

                // passe au prochain segment si ca deborde
                to_write = port_s->iovec[current_seg_id].iov_len;
                while(written >= to_write){
                    port_s->iovec[current_seg_id].iov_len  = 0;

                    written = written - to_write;
                    current_seg_id++;
                    to_write = port_s->iovec[current_seg_id].iov_len;
                }

                port_s->iovec[current_seg_id].iov_base += written;
                port_s->iovec[current_seg_id].iov_len  -= written;
            }
        }
    }
    //DISP("<--s_test");
    LOG_OUT();
    return tbx_false;
}


static p_mad_iovec_t
mad_tcp_receive_data(p_mad_port_t  port){
    p_mad_tcp_port_specific_t port_s = NULL;
    int nb_read = 0;
    int current_seg_id = 0;
    LOG_IN();

    port_s = port->specific;
    current_seg_id = port_s->current_seg_id;

    //DISP("READ");
    SYSCALL(nb_read = readv(port_s->desc,
                            port_s->iovec,
                            port_s->nb_seg));

    if(nb_read){
        //DISP("on a lu");
        //DISP_VAL("r_test - read    ", nb_read);
        //DISP_VAL("r_test - déjà lus", port_s->nb_treated_bytes);
        //DISP_VAL("r_test - attendus   ", port_s->waiting_length);
        //DISP("");

        port_s->nb_treated_bytes += nb_read;

        if(port_s->nb_treated_bytes == port_s->waiting_length){
            //DISP("RECU!!!!");
            port_s->nb_treated_bytes = 0;
            port_s->nb_seg = 0;
            port_s->waiting_length = 0;
            port_s->current_seg_id = 0;

            return port_s->mad_iovec;
        } else {
            int to_read = 0;

            // passe au prochain segment si ca deborde
            to_read = port_s->iovec[current_seg_id].iov_len;
            while(nb_read >= to_read){
                port_s->iovec[current_seg_id].iov_len = 0;

                nb_read = nb_read - to_read;
                current_seg_id++;
                to_read = port_s->iovec[current_seg_id].iov_len;
            }

            port_s->iovec[current_seg_id].iov_base += nb_read;
            port_s->iovec[current_seg_id].iov_len  -= nb_read;

            return NULL;
        }
    }
    LOG_OUT();
    return NULL;
}




p_mad_iovec_t
mad_tcp_r_test(p_mad_track_t track) {
    p_mad_tcp_track_specific_t ts = NULL;

    p_mad_port_t          port = NULL;
    p_mad_tcp_port_specific_t port_s = NULL;
    struct pollfd polled;

    int i = 0;
    int nb_read = 0;
    int j = 0;
    int nb_dest = -1;
    int nb_polled_sock = 0;
    int nb_revents = 0;
    int current_seg_id = 0;

    LOG_IN();
    //DISP("-->r_test");

    ts        = track->specific;
    nb_dest   = track->nb_dest;
    nb_polled_sock = ts->nb_polled_sock;


    SYSCALL(nb_revents = poll(ts->to_poll, nb_polled_sock, ts->delay));

    //if(nb_revents)
    //    DISP_VAL("tcp_r_test - nb_revents", nb_revents);
    //DISP_VAL("           - nb_polled_sock", nb_polled_sock);

    for(i = 0, j = (ts->last_polled_idx + 1) % nb_polled_sock;
        i < nb_polled_sock && nb_revents;
        i++, j = (j+1) % nb_polled_sock){

        polled = ts->to_poll[j];

        // si revents de la structure pollfd == POLLIN
        // alors je peux lire sur la socket
        if(polled.revents & POLLIN){
            //DISP_VAL("           - id de la socket pollée", j);

            nb_revents--;
            ts->last_polled_idx = j;

            port   = track->local_ports[j];
            port_s = port->specific;
            current_seg_id = port_s->current_seg_id;

            // si on n'a pas initialisé la taille à recevoir
            if(track->pre_posted && port_s->waiting_length == 0){

                //DISP_VAL("port_s->iovec[0].iov_len",
                //port_s->iovec[0].iov_len);
                //DISP_VAL("port_s->iovec[1].iov_len",
                //port_s->iovec[1].iov_len);


                // Je lis ce qui est nécessaire pour récupérer la taille
                SYSCALL(nb_read = readv(port_s->desc, port_s->iovec, 1));
                
                if(nb_read){
                    port_s->nb_treated_bytes += nb_read;

                    if(port_s->nb_treated_bytes == port_s->iovec[0].iov_len){
                        int len = * ((int*)port_s->iovec[0].iov_base);
                        DISP_VAL("RECV - len recue", len);

                        port_s->waiting_length = len;
                        DISP_VAL("port_s->iovec[0].iov_len", port_s->iovec[0].iov_len);
                        port_s->iovec[0].iov_len = 0;
                        port_s->iovec[1].iov_len = len;
                        port_s->current_seg_id++;
                        port_s->nb_treated_bytes = 0;

                        // on lit les données
                        SYSCALL(nb_revents = poll(ts->to_poll, nb_polled_sock, ts->delay));
                        if(nb_revents && polled.revents & POLLIN)
                            return mad_tcp_receive_data(port);
                        else
                            return NULL;

                    } else {
                        port_s->iovec[0].iov_base += nb_read;
                        port_s->iovec[0].iov_len  -= nb_read;

                        return NULL;
                    }
                }

            } else { // on lit les données
                //DISP("on ne lit que les données");
                //DISP_VAL("port_s->waiting_length", port_s->waiting_length);
                return mad_tcp_receive_data(port);
            }
        }
    }
    ts->last_polled_idx = (1 + ts->last_polled_idx) % nb_polled_sock;

    //DISP("<--r_test");
    LOG_OUT();
    return NULL;
}


void
mad_tcp_isend(p_mad_track_t track,
              p_mad_iovec_t mad_iovec){

    p_mad_port_t              port    = NULL;
    p_mad_tcp_port_specific_t port_specific    = NULL;
    struct iovec *iovec = NULL;
    int i = 0;
    p_mad_tcp_track_specific_t ts = NULL;
    LOG_IN();


    //DISP("--->tcp_isend");
    ts = track->specific;
    port          = track->local_ports[mad_iovec->remote_rank];
    port_specific = port->specific;
    iovec         = port_specific->iovec;

    ts->unexpected_len = mad_iovec->length;

    if(track->pre_posted){
        iovec[0].iov_base = &ts->unexpected_len;
        iovec[0].iov_len  = sizeof(int);

        DISP_VAL("ISEND - longueur envoyée", * ((int *)iovec[0].iov_base));

        for(i = 0; i < mad_iovec->total_nb_seg; i++){
            iovec[i+1].iov_base = mad_iovec->data[i].iov_base;
            iovec[i+1].iov_len  = mad_iovec->data[i].iov_len;
        }

        port_specific->waiting_length = mad_iovec->length + sizeof(int);

        port_specific->nb_seg           = mad_iovec->total_nb_seg + 1;
    } else {

        for(i = 0; i < mad_iovec->total_nb_seg; i++){
            iovec[i].iov_base = mad_iovec->data[i].iov_base;
            iovec[i].iov_len  = mad_iovec->data[i].iov_len;
        }

        port_specific->waiting_length = mad_iovec->length;
        port_specific->nb_seg         = mad_iovec->total_nb_seg;
    }

    port_specific->current_seg_id   = 0;
    port_specific->nb_treated_bytes = 0;

    //DISP("<---tcp_isend");
    LOG_OUT();
}



void
mad_tcp_irecv(p_mad_track_t track,
              p_mad_iovec_t mad_iovec){
    p_mad_port_t              port             = NULL;
    p_mad_tcp_port_specific_t port_specific    = NULL;
    int i = 0;

    LOG_IN();
    //DISP("--->tcp_irecv");
    port          = track->local_ports[mad_iovec->remote_rank];
    port_specific = port->specific;

    if(track->pre_posted){
        port_specific->unexpected_len = 15768;

        port_specific->iovec[0].iov_base = &port_specific->unexpected_len;
        port_specific->iovec[0].iov_len  = sizeof(int);

        for(i = 0; i < mad_iovec->total_nb_seg; i++){
            port_specific->iovec[i+1].iov_base = mad_iovec->data[i].iov_base;
            port_specific->iovec[i+1].iov_len  = mad_iovec->data[i].iov_len;

            //DISP_VAL("mad_iovec->data[i].iov_len", mad_iovec->data[i].iov_len);
        }

        port_specific->nb_seg  = mad_iovec->total_nb_seg + 1;
        port_specific->waiting_length   = 0;

    } else {
        for(i = 0; i < mad_iovec->total_nb_seg; i++){
            port_specific->iovec[i].iov_base = mad_iovec->data[i].iov_base;
            port_specific->iovec[i].iov_len  = mad_iovec->data[i].iov_len;
        }

        port_specific->nb_seg = mad_iovec->total_nb_seg;
        port_specific->waiting_length = mad_iovec->length;

        //DISP_VAL("gros -len", port_specific->waiting_length);
    }

    port_specific->mad_iovec        = mad_iovec;
    port_specific->current_seg_id   = 0;
    port_specific->nb_treated_bytes = 0;

    track->pending_reception[mad_iovec->remote_rank] = mad_iovec;
    track->nb_pending_reception++;
    track->track_set->reception_tracks_in_use[track->id] = tbx_true;


    DISP_VAL("irecv - port_s->iovec[0].iov_len", port_specific->iovec[0].iov_len);
    DISP_VAL("irecv - port_s->iovec[1].iov_len", port_specific->iovec[1].iov_len);


    //DISP_PTR("irecv : iovec[0].iov_base", port_specific->iovec[0].iov_base);
    //DISP_VAL("      : iovec[0].contenu ", *((int *)port_specific->iovec[0].iov_base));

    //DISP("<---tcp_irecv");
    LOG_OUT();
}



void
mad_tcp_init_pre_posted(p_mad_adapter_t adapter,
                       p_mad_track_t track){
    p_mad_track_set_t r_track_set = NULL;
    p_mad_iovec_t mad_iovec = NULL;
    ntbx_process_lrank_t my_rank = -1;
    int nb_dest = 0;
    int i = 0;
    p_mad_madeleine_t madeleine = NULL;
    p_mad_session_t     session = NULL;

    LOG_IN();
    //DISP("-->mad_tcp_init_pre_posted");
    madeleine = mad_get_madeleine();
    session = madeleine->session;
    my_rank = session->process_rank;

    r_track_set = adapter->r_track_set;
    nb_dest = track->nb_dest;

    //DISP_VAL("init_pre_posted, nb_dest", nb_dest);

    for(i = 0; i < nb_dest; i++){
        //if(i  != my_rank){
            mad_iovec = mad_pipeline_remove(adapter->pre_posted);
            mad_iovec->track = track;
            mad_iovec->remote_rank = i;

            track->pending_reception[i] = mad_iovec;

            r_track_set->nb_pending_reception++;
            track->nb_pending_reception++;

            mad_tcp_irecv(track, mad_iovec);
            //}
    }

    r_track_set->reception_tracks_in_use[track->id] = tbx_true;
    r_track_set->status = MAD_MKP_PROGRESS;

    //DISP("<--mad_tcp_add_pre_posted");
    LOG_OUT();
}

void
mad_tcp_replace_pre_posted(p_mad_adapter_t adapter,
                           p_mad_track_t track,
                           int port_id){
    p_mad_iovec_t mad_iovec = NULL;
    p_mad_track_set_t r_track_set = NULL;
    LOG_IN();

    //DISP("-------->replace pre posted");


    r_track_set = adapter->r_track_set;

    mad_iovec              = mad_pipeline_remove(adapter->pre_posted);
    mad_iovec->track       = track;
    mad_iovec->remote_rank = port_id;

    mad_iovec->data[0].iov_len = MAD_TCP_PRE_POSTED_SIZE;

    //DISP_VAL("mad_iovec->data[0].iov_len", mad_iovec->data[0].iov_len);

    track->pending_reception[port_id] = mad_iovec;

    r_track_set->nb_pending_reception++;
    track->nb_pending_reception++;

    mad_tcp_irecv(track, mad_iovec);

    r_track_set->reception_tracks_in_use[track->id] = tbx_true;
    r_track_set->status = MAD_MKP_PROGRESS;

    //DISP("<---replace pre posted");
    LOG_OUT();
}


void
mad_tcp_remove_all_pre_posted(p_mad_adapter_t adapter){
    p_mad_track_set_t r_track_set = NULL;
    p_mad_track_t     cpy_track       = NULL;
    p_mad_iovec_t     mad_iovec   = NULL;
    int i = 0;
    LOG_IN();
    //DISP("-->remove pre posted");

    r_track_set = adapter->r_track_set;
    cpy_track   = r_track_set->cpy_track;

    for( i = 0; i < cpy_track->nb_dest; i++){
        mad_iovec   = cpy_track->pending_reception[i];
        cpy_track->pending_reception[i] = NULL;

        if(mad_iovec){
            mad_pipeline_add(adapter->pre_posted, mad_iovec);
        }
    }

    //DISP("<--remove pre posted");
    LOG_OUT();
}
