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
 * Mad_driver_interface.h
 * ======================
 */

#ifndef MAD_DRIVER_INTERFACE_H
#define MAD_DRIVER_INTERFACE_H

typedef struct s_mad_driver_interface
{
    // Initialisation statique
    void (*driver_init)        (p_mad_driver_t, int *, char ***);
    void (*adapter_init)       (p_mad_adapter_t);
    void (*channel_init)       (p_mad_channel_t);
    void (*connection_init)    (p_mad_connection_t,
                                p_mad_connection_t);
    void (*link_init)          (p_mad_link_t);
    void (*track_set_init)     (p_mad_track_set_t);
    void (*track_init)         (p_mad_adapter_t, uint32_t);


    // Initialisation dynamique
    void (*before_open_channel)(p_mad_channel_t);
    void (*accept)             (p_mad_connection_t,
                                p_mad_adapter_info_t);
    void (*connect)            (p_mad_connection_t,
                                p_mad_adapter_info_t);
    void (*after_open_channel) (p_mad_channel_t);


    // Fermeture
    void (*before_close_channel)(p_mad_channel_t);
    void (*disconnect)(p_mad_connection_t);
    void (*after_close_channel)(p_mad_channel_t);
    void (*link_exit)(p_mad_link_t);
    void (*connection_exit)(p_mad_connection_t,
                            p_mad_connection_t);
    void (*channel_exit)(p_mad_channel_t);
    void (*adapter_exit)(p_mad_adapter_t);
    void (*driver_exit)(p_mad_driver_t);
    void (*track_set_exit)(p_mad_track_set_t);
    void (*track_exit)(p_mad_track_t);


    /* Message transfer */
    void (*new_message)(p_mad_connection_t);
    void (*finalize_message)(p_mad_connection_t);
    p_mad_connection_t (*receive_message)(p_mad_channel_t);
    void (*message_received)(p_mad_connection_t);


    // Gestion des pré-postés
    void (*init_pre_posted)(p_mad_adapter_t, p_mad_track_t);
    void (*replace_pre_posted)(p_mad_adapter_t,
                               p_mad_track_t, int port_id);
    void (*remove_all_pre_posted)(p_mad_adapter_t);


    // **** Heuristiques **** //
    tbx_bool_t (*rdma_available)();
    tbx_bool_t (*gather_scatter_available)();
    int (*get_max_nb_ports)();
    uint32_t (*msg_length_max)();
    uint32_t (*copy_length_max)();
    uint32_t (*gather_scatter_length_max)();
    tbx_bool_t (*need_rdv)(p_mad_iovec_t);
    tbx_bool_t (*buffer_need_rdv)(size_t);



    // **** Primitives Bas Niveau **** //
    void (*isend)(p_mad_track_t, p_mad_iovec_t);
    void (*irecv)(p_mad_track_t, p_mad_iovec_t);
    tbx_bool_t    (*s_test)(p_mad_track_set_t);
    p_mad_iovec_t (*r_test)(p_mad_track_t);
    
    // ??
    p_mad_channel_t (*get_sub_channel)(p_mad_channel_t,
                                       unsigned int);
} mad_driver_interface_t;

#endif /* MAD_DRIVER_INTERFACE_H */
