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
    /* Driver initialization */
    void
    (*driver_init)(p_mad_driver_t, int *, char ***);

    /* Adapter initialization */
    void
    (*adapter_init)(p_mad_adapter_t);


    /* Channel/Connection/Link initialization */
    void
    (*channel_init)(p_mad_channel_t);

    void
    (*before_open_channel)(p_mad_channel_t);

    void
    (*connection_init)(p_mad_connection_t,
                       p_mad_connection_t);

    void
    (*link_init)(p_mad_link_t);

    /* Point-to-point connection */
    void
    (*accept)(p_mad_connection_t,
              p_mad_adapter_info_t);

    void
    (*connect)(p_mad_connection_t,
               p_mad_adapter_info_t);

    /* Channel clean-up functions */
    void
    (*after_open_channel)(p_mad_channel_t);

    void
    (*before_close_channel)(p_mad_channel_t);

    /* Connection clean-up function */
    void
    (*disconnect)(p_mad_connection_t);

    void
    (*after_close_channel)(p_mad_channel_t);

    /* Deallocation functions */
    void
    (*link_exit)(p_mad_link_t);

    void
    (*connection_exit)(p_mad_connection_t,
                       p_mad_connection_t);

    void
    (*channel_exit)(p_mad_channel_t);

    void
    (*adapter_exit)(p_mad_adapter_t);

    void
    (*driver_exit)(p_mad_driver_t);


    /* Message transfer */
    void
    (*new_message)(p_mad_connection_t);

    void
    (*finalize_message)(p_mad_connection_t);

    p_mad_connection_t
    (*receive_message)(p_mad_channel_t);

    void
    (*message_received)(p_mad_connection_t);

    p_mad_channel_t
    (*get_sub_channel)(p_mad_channel_t,
                       unsigned int);


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
    void (*isend)(p_mad_track_t,
                  ntbx_process_lrank_t,
                  struct iovec *, uint32_t);

    void (*irecv)(p_mad_track_t,
                  struct iovec *, uint32_t);


    void (*send)(p_mad_track_t,
                 p_mad_connection_t,
                 struct iovec *, uint32_t);
    void (*recv)(p_mad_track_t,
                 struct iovec *, uint32_t);

    tbx_bool_t (*test)(p_mad_track_t);
    void       (*wait)(p_mad_track_t);

    void (*open_track)(p_mad_adapter_t, uint32_t);
    void (*close_track)(p_mad_track_t);

    void (*add_pre_posted)(p_mad_adapter_t, p_mad_track_set_t);
    void (*remove_all_pre_posted)(p_mad_adapter_t);

    void (*add_pre_sent)  (p_mad_adapter_t, p_mad_track_t);

    void (*close_track_set)(p_mad_track_set_t);

} mad_driver_interface_t;

#endif /* MAD_DRIVER_INTERFACE_H */


















    ///* Dynamic paradigm selection */
    //p_mad_link_t
    //(*choice)(p_mad_connection_t,
    //          size_t,
    //          mad_send_mode_t,
    //          mad_receive_mode_t);
    //
    ///* Static buffers management */
    //p_mad_buffer_t
    //(*get_static_buffer)(p_mad_link_t);
    //
    //void
    //(*return_static_buffer)(p_mad_link_t, p_mad_buffer_t);
    //
    ///* Message transfer */
    //void
    //(*new_message)(p_mad_connection_t);
    //
    //void
    //(*finalize_message)(p_mad_connection_t);
    //
    //p_mad_connection_t
    //(*receive_message)(p_mad_channel_t);
    //
    //#ifdef MAD_MESSAGE_POLLING
    //    p_mad_connection_t
    //    (*poll_message)(p_mad_channel_t);
    //#endif /* MAD_MESSAGE_POLLING */
    //
    //void
    //(*message_received)(p_mad_connection_t);
    //
    /* Buffer transfer */
    //void
    //(*send_buffer)(p_mad_link_t,
    //               p_mad_buffer_t);
    //
    //void
    //(*receive_buffer)(p_mad_link_t,
    //                  p_mad_buffer_t *);
    //
    /* Buffer group transfer */
    //        void
    //        (*send_buffer_group)(p_mad_link_t,
    //                             p_mad_buffer_group_t);
    //
    //        void
    //        (*receive_sub_buffer_group)(p_mad_link_t,
    //                                    tbx_bool_t,
    //                                    p_mad_buffer_group_t);
    //
    //void
    //(*send_iovec)(p_mad_link_t,
    //              p_mad_pipeline_t,
    //              p_mad_on_going_t);
    //
    //void
    //(*receive_iovec)(p_mad_link_t,
    //                 p_mad_on_going_t);
    //
    //
    //
    //p_mad_channel_t
    //(*get_sub_channel)(p_mad_channel_t,
    //                   unsigned int);
    //
    //void
    //(*isend_buffer_group)(p_mad_link_t,
    //                      p_mad_buffer_group_t);
    //
    //void
    //(*ireceive_sub_buffer_group)(p_mad_link_t,
    //                             tbx_bool_t,
    //                             p_mad_buffer_group_t);
    //
    //void
    //(*s_make_progress)(p_mad_adapter_t);
    //
    //void
    //(*r_make_progress)(p_mad_adapter_t, p_mad_channel_t);
    //
    //tbx_bool_t
    //(*need_rdv)(p_mad_iovec_t);
    //
    //unsigned int
    //(*limit_size)(void);
    //
    //void
    //(*create_areas)(p_mad_iovec_t);
    //
    //void
    //(*sending_track_init)(p_mad_adapter_t);
    //
    //void
    //(*reception_track_init)(p_mad_adapter_t);
    //
    //void
    //(*sending_track_exit)(p_mad_adapter_t);
    //
    //void
    //(*reception_track_exit)(p_mad_adapter_t);
    //
    //void
    //(*on_going_init)(p_mad_on_going_t);
    //
    //void
    //(*on_going_free)(p_mad_on_going_t);
    //
    //void
    //(*receive_large_iovec)(p_mad_iovec_t);
    //
    //void
    //(*send_large_iovec)(p_mad_iovec_t);
    //
    //
    //tbx_bool_t
    //(*send_control)(unsigned int dest,
    //                unsigned int channel_id,
    //                struct iovec * iov);
