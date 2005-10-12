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
 * Mad_communication.c
 * ===================
 */
#include "madeleine.h"

p_mad_connection_t
mad_begin_packing(p_mad_channel_t      channel,
		  ntbx_process_lrank_t remote_rank){
    p_mad_driver_t     driver                 = NULL;
    p_mad_adapter_t    adapter                = NULL;
    p_mad_connection_t connection             = NULL;
    p_mad_driver_interface_t  interface       = NULL;
    LOG_IN();
    adapter     = channel->adapter;
    driver      = adapter->driver;
    interface   = driver->interface;
    connection  = tbx_darray_get(channel->out_connection_darray, remote_rank);

    // lock the connection
    if (connection->lock == tbx_true)
        FAILURE("mad_begin_packing: connection dead lock");
    connection->lock = tbx_true;

    if (interface->new_message)
        interface->new_message(connection);

    // initialize the sequence
    connection->sequence = 1;

    LOG_OUT();
    return connection;
}

void
mad_pack(p_mad_connection_t   connection,
	 void                *buffer,
	 size_t               buffer_length,
	 mad_send_mode_t      send_mode,
	 mad_receive_mode_t   receive_mode){
    p_mad_iovec_t             mad_iovec   = NULL;
    p_mad_channel_t           channel     = NULL;
    p_mad_adapter_t           adapter     = NULL;
    p_mad_driver_t            driver      = NULL;
    p_mad_driver_interface_t  interface      = NULL;
    ntbx_process_lrank_t      remote_rank = -1;
    unsigned int              seq         = -1;
    tbx_bool_t need_rdv = tbx_false;
    LOG_IN();
    //DISP("-->pack");
    remote_rank = connection->remote_rank;
    channel     = connection->channel;
    adapter     = channel->adapter;
    driver      = adapter->driver;
    interface   = driver->interface;
    seq         = connection->sequence;
    need_rdv    = interface->buffer_need_rdv(buffer_length);

    connection->sequence++;

    // create a mad_iovec
    mad_iovec = mad_iovec_create(remote_rank, channel,
                                 seq, need_rdv,
                                 send_mode, receive_mode);
    mad_iovec_add_data2(mad_iovec, buffer, buffer_length, 2);

    tbx_slist_append(driver->s_msg_slist, mad_iovec);
    tbx_slist_append(connection->packs_list, mad_iovec);

    driver->nb_pack_to_send++;
    LOG_OUT();
    //DISP("<-- pack");

}

void
mad_end_packing(p_mad_connection_t connection){
    p_mad_channel_t channel = NULL;
    p_mad_adapter_t adapter = NULL;
    p_mad_driver_t driver = NULL;
    ntbx_process_lrank_t      remote_rank = -1;
    p_tbx_slist_t packs_list = NULL;
    p_mad_driver_interface_t interface = NULL;
    LOG_IN();
    channel     = connection->channel;
    adapter     = channel->adapter;
    driver      = adapter->driver;
    interface   = driver->interface;
    remote_rank = connection->remote_rank;
    packs_list  = connection->packs_list;

    // flush packs
    while(packs_list->length){
        mad_s_make_progress(adapter);
        mad_r_make_progress(adapter);
    }
    if (interface->finalize_message)
        interface->finalize_message(connection);

    // reset the sequence
    connection->sequence = 0;

    // unlock the connection
    connection->lock = tbx_false;
    LOG_OUT();
}


int nb_chronos_mad_r_mkp_2 = 0;
int nb_chronos_mad_s_mkp_2 = 0;
double chrono_r_mkp_2 = 0.0;
double chrono_s_mkp_2 = 0.0;

void
mad_wait_packs(p_mad_connection_t connection){
    p_mad_channel_t channel = NULL;
    p_mad_adapter_t adapter = NULL;
    p_mad_driver_t driver = NULL;

    tbx_tick_t        t1;
    tbx_tick_t        t2;
    tbx_tick_t        t3;
    LOG_IN();
    channel     = connection->channel;
    adapter     = channel->adapter;
    driver      = adapter->driver;

    // flush packs
    while(connection->need_reception){
        TBX_GET_TICK(t1);
        mad_s_make_progress(adapter);
        TBX_GET_TICK(t2);
        mad_r_make_progress(adapter);
        TBX_GET_TICK(t3);

        chrono_s_mkp_2        += TBX_TIMING_DELAY(t1, t2);
        chrono_r_mkp_2        += TBX_TIMING_DELAY(t2, t3);
        nb_chronos_mad_r_mkp_2++;
        nb_chronos_mad_s_mkp_2++;
    }

    while(connection->packs_list->length){
        TBX_GET_TICK(t1);
        mad_s_make_progress(adapter);
        TBX_GET_TICK(t2);

        if(connection->need_reception){
            mad_r_make_progress(adapter);
        }

        chrono_s_mkp_2        += TBX_TIMING_DELAY(t1, t2);
        nb_chronos_mad_s_mkp_2++;
    }
    LOG_OUT();
}

p_mad_connection_t
mad_begin_unpacking(p_mad_channel_t channel){
    p_mad_connection_t       connection = NULL;
    p_mad_driver_interface_t interface = NULL;
    LOG_IN();
    // lock the channel
    if (channel->reception_lock == tbx_true)
        FAILURE("mad_begin_unpacking: reception dead lock");
    channel->reception_lock = tbx_true;

    interface = channel->adapter->driver->interface;
    connection = interface->receive_message(channel);
    if (!connection)
        FAILURE("message reception failed");

    // initialize the sequence
    channel->sequence = 1;
    LOG_OUT();
    return connection;
}


void
mad_unpack(p_mad_connection_t    connection,
            void                *buffer,
            size_t               buffer_length,
            mad_send_mode_t      send_mode,
            mad_receive_mode_t   receive_mode){
    p_mad_channel_t           channel     = NULL;
    p_mad_adapter_t           adapter     = NULL;
    p_mad_driver_t            driver      = NULL;
    p_mad_driver_interface_t  interface      = NULL;
    ntbx_process_lrank_t      remote_rank = -1;
    p_mad_iovec_t             mad_iovec   = NULL;
    unsigned int              seq         = -1;
    tbx_bool_t need_rdv = tbx_false;
    LOG_IN();
    remote_rank = connection->remote_rank;
    channel     = connection->channel;
    adapter     = channel->adapter;
    driver      = adapter->driver;
    interface   = driver->interface;
    seq         = channel->sequence;
    need_rdv = interface->buffer_need_rdv(buffer_length);

    channel->sequence++;

    // create a mad_iovec
    mad_iovec = mad_iovec_create(remote_rank, channel,
                                 seq, need_rdv,
                                 send_mode, receive_mode);

    mad_iovec_add_data2(mad_iovec, buffer, buffer_length, 0);

    tbx_slist_append(channel->unpacks_list, mad_iovec);

    if(receive_mode == mad_receive_EXPRESS){
        while(channel->unpacks_list->length){
            mad_r_make_progress(adapter);
            mad_s_make_progress(adapter);
        }
    }
    LOG_OUT();
}

void
mad_end_unpacking(p_mad_connection_t connection){
    p_mad_channel_t channel = NULL;
    p_mad_adapter_t adapter = NULL;
    p_mad_driver_t driver = NULL;
    p_mad_driver_interface_t interface = NULL;
    LOG_IN();

    channel = connection->channel;
    adapter = channel->adapter;
    driver = adapter->driver;
    interface = driver->interface;

    // flush unpacks
    while(channel->unpacks_list->length){
        mad_r_make_progress(adapter);
        mad_s_make_progress(adapter);
    }

    if (interface->message_received)
        interface->message_received(connection);

    // reset the sequence
    channel->sequence = 0;

    // unlock the channel
    channel->reception_lock = tbx_false;

    LOG_OUT();
}

void
mad_wait_unpacks(p_mad_connection_t connection){
    p_mad_channel_t channel = NULL;
    p_mad_adapter_t adapter = NULL;
    p_mad_driver_t driver = NULL;
    LOG_IN();
    channel = connection->channel;
    adapter = channel->adapter;
    driver = adapter->driver;

    while(channel->need_send){
        mad_r_make_progress(adapter);
        mad_s_make_progress(adapter);
    }
    while(channel->unpacks_list->length){
        mad_r_make_progress(adapter);
    }
    LOG_OUT();
}
