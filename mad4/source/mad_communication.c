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
#ifdef MARCEL
    marcel_mutex_lock(&(connection->lock_mutex));
#else /* MARCEL */
    if (connection->lock == tbx_true)
        FAILURE("mad_begin_packing: connection dead lock");
    connection->lock = tbx_true;
#endif /* MARCEL*/

    if (interface->new_message)
        interface->new_message(connection);

    // initialize the sequence
    connection->sequence = 0;

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
    ntbx_process_lrank_t      remote_rank = -1;
    unsigned int              seq         = -1;
    LOG_IN();

    remote_rank = connection->remote_rank;
    channel     = connection->channel;
    adapter     = channel->adapter;
    driver      = adapter->driver;
    seq         = connection->sequence;
    connection->sequence++;

    // create a mad_iovec
    mad_iovec = mad_iovec_create(channel->id, seq);
    mad_iovec->remote_rank = remote_rank;
    mad_iovec->area_nb_seg[0] = 1;
    mad_iovec->channel = channel;
    mad_iovec->send_mode = send_mode;
    mad_iovec->receive_mode = receive_mode;
    mad_iovec_add_data(mad_iovec, buffer, buffer_length, 0);

    tbx_slist_append(driver->s_msg_slist, mad_iovec);
    tbx_slist_append(connection->packs_list, mad_iovec);

    LOG_OUT();
}

void
mad_extended_pack(p_mad_connection_t   connection,
                  void                *buffer,
                  size_t               buffer_length,
                  mad_send_mode_t      send_mode,
                  mad_receive_mode_t   receive_mode,
                  mad_send_mode_t      next_send_mode,
                  mad_receive_mode_t   next_receive_mode){
    // au niveau du canal,
    // stocker le mad_iovec en cours pour chaque connexion
    // à chaque pack, on met à jour le mad_iovec courant

    p_mad_iovec_t             mad_iovec   = NULL;
    p_mad_channel_t           channel     = NULL;
    p_mad_adapter_t           adapter     = NULL;
    p_mad_driver_t            driver      = NULL;
    ntbx_process_lrank_t      remote_rank = -1;
    unsigned int              seq         = -1;
    LOG_IN();

    remote_rank = connection->remote_rank;
    channel     = connection->channel;
    adapter     = channel->adapter;
    driver      = adapter->driver;
    seq         = connection->sequence;
    connection->sequence++;

    // create a mad_iovec
    mad_iovec = mad_iovec_create(channel->id, seq);
    mad_iovec->remote_rank = remote_rank;
    mad_iovec->area_nb_seg[0] = 1;
    mad_iovec->channel = channel;
    mad_iovec->send_mode = send_mode;
    mad_iovec->receive_mode = receive_mode;
    mad_iovec->next_send_mode = next_send_mode;
    mad_iovec->next_receive_mode = next_receive_mode;
    mad_iovec_add_data(mad_iovec, buffer, buffer_length, 0);

    tbx_slist_append(driver->s_msg_slist, mad_iovec);
    tbx_slist_append(connection->packs_list, mad_iovec);

    LOG_OUT();
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
        mad_s_make_progress(driver, adapter);
        mad_r_make_progress(driver, adapter, channel);
    }

    if (interface->finalize_message)
        interface->finalize_message(connection);

    // reset the sequence
    connection->sequence = 0;

    // unlock the connection
#ifdef MARCEL
    marcel_mutex_unlock(&(connection->lock_mutex));
#else // MARCEL
    connection->lock = tbx_false;
#endif // MARCEL

    LOG_OUT();
}

void
mad_wait_packs(p_mad_connection_t connection){
    p_mad_channel_t channel = NULL;
    p_mad_adapter_t adapter = NULL;
    p_mad_driver_t driver = NULL;

    LOG_IN();
    channel     = connection->channel;
    adapter     = channel->adapter;
    driver      = adapter->driver;

    // flush packs
    while(connection->packs_list->length){
        mad_s_make_progress(driver, adapter);
        mad_r_make_progress(driver, adapter, channel);
    }
    LOG_OUT();
}

p_mad_connection_t
mad_begin_unpacking(p_mad_channel_t channel){
    p_mad_connection_t       connection = NULL;
    p_mad_driver_interface_t interface = NULL;
    LOG_IN();

    // lock the channel
#ifdef MARCEL
  marcel_mutex_lock(&(channel->reception_lock_mutex));
#else /* MARCEL */
  if (channel->reception_lock == tbx_true)
        FAILURE("mad_begin_unpacking: reception dead lock");
    channel->reception_lock = tbx_true;
#endif /* MARCEL */

    interface = channel->adapter->driver->interface;
    connection = interface->receive_message(channel);
    if (!connection)
        FAILURE("message reception failed");

    // initialize the sequence
    channel->sequence = 0;

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
    ntbx_process_lrank_t      remote_rank = -1;
    p_mad_iovec_t             mad_iovec   = NULL;
    unsigned int              seq         = -1;
    LOG_IN();

    remote_rank = connection->remote_rank;
    channel     = connection->channel;
    adapter     = channel->adapter;
    driver      = adapter->driver;
    seq         = channel->sequence;
    channel->sequence++;

    // create a mad_iovec
    mad_iovec = mad_iovec_create(channel->id, seq);
    mad_iovec->remote_rank = remote_rank;
    mad_iovec->send_mode = send_mode;
    mad_iovec->receive_mode = receive_mode;
    mad_iovec->area_nb_seg[0] = 1;
    mad_iovec->channel = channel;
    mad_iovec_add_data(mad_iovec, buffer, buffer_length, 0);

    tbx_slist_append(driver->r_msg_slist, mad_iovec);
    tbx_slist_append(channel->unpacks_list, mad_iovec);

    if(receive_mode == mad_receive_EXPRESS){
        while(channel->unpacks_list->length){
            mad_r_make_progress(driver, adapter, channel);
            mad_s_make_progress(driver, adapter);
        }
    }
    LOG_OUT();
}


void
mad_extended_unpack(p_mad_connection_t    connection,
            void                *buffer,
            size_t               buffer_length,
            mad_send_mode_t      send_mode,
            mad_receive_mode_t   receive_mode,
            mad_send_mode_t      next_send_mode,
            mad_receive_mode_t   next_receive_mode){
    // au niveau de madeleine,
    // stocker un mad_iovec en construction pour chaque canal
    // à chaque unpack, on le met à jour

    p_mad_channel_t           channel     = NULL;
    p_mad_adapter_t           adapter     = NULL;
    p_mad_driver_t            driver      = NULL;
    ntbx_process_lrank_t      remote_rank = -1;
    p_mad_iovec_t             mad_iovec   = NULL;
    unsigned int              seq         = -1;
    LOG_IN();

    remote_rank = connection->remote_rank;
    channel     = connection->channel;
    adapter     = channel->adapter;
    driver      = adapter->driver;
    seq         = channel->sequence;
    channel->sequence++;

    // create a mad_iovec
    mad_iovec = mad_iovec_create(channel->id, seq);
    mad_iovec->remote_rank = remote_rank;
    mad_iovec->area_nb_seg[0] = 1;
    mad_iovec->channel = channel;
    mad_iovec->send_mode = send_mode;
    mad_iovec->receive_mode = receive_mode;
    mad_iovec->next_send_mode = next_send_mode;
    mad_iovec->next_receive_mode = next_receive_mode;
    mad_iovec_add_data(mad_iovec, buffer, buffer_length, 0);

    tbx_slist_append(driver->r_msg_slist, mad_iovec);
    tbx_slist_append(channel->unpacks_list, mad_iovec);

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
        mad_r_make_progress(driver, adapter, channel);
        mad_s_make_progress(driver, adapter);
    }

    if (interface->message_received)
        interface->message_received(connection);

    // reset the sequence
    channel->sequence = 0;

    // unlock the channel
#ifdef MARCEL
    marcel_mutex_unlock(&(connection->channel->reception_lock_mutex));
#else // MARCEL
    channel->reception_lock = tbx_false;
#endif //MARCEL

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

    while(channel->unpacks_list->length){
        mad_r_make_progress(driver, adapter, channel);
        mad_s_make_progress(driver, adapter);
    }
    LOG_OUT();
}
