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
    p_mad_driver_interface_t  interface              = NULL;
    LOG_IN();

    //DISP("-------------------------->begin_packing");

    adapter                = channel->adapter;
    driver                 = adapter->driver;
    interface              = driver->interface;
    connection             = tbx_darray_get(channel->out_connection_darray, remote_rank);

    // on pose un verrou sur la connexion
    if (connection->lock == tbx_true)
        FAILURE("mad_begin_packing: connection dead lock");
    connection->lock = tbx_true;

    if (interface->new_message)
        interface->new_message(connection);

    // on initialise la sequence
    connection->sequence = 0;

    LOG_OUT();
    //DISP("<--begin_packing");
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
  //DISP("------->pack");

    remote_rank = connection->remote_rank;
    channel     = connection->channel;
    adapter     = channel->adapter;
    driver      = adapter->driver;
    seq         = connection->sequence;
    connection->sequence++;

    // creation du mad_iovec
    mad_iovec = mad_iovec_create(channel->id, seq);
    mad_iovec->remote_rank = remote_rank;
    mad_iovec->area_nb_seg[0] = 1;
    mad_iovec->channel = channel;
    mad_iovec->send_mode = send_mode;
    mad_iovec->receive_mode = receive_mode;

    mad_iovec_add_data(mad_iovec, buffer, buffer_length, 0);

    //{
    //    DISP("***************");
    //    DISP("* pack : ajout du mad_iovec dans la liste :");
    //    DISP_VAL("* ->channl_id      ", mad_iovec->channel_id);
    //    DISP_VAL("* ->channl_sequence", mad_iovec->sequence);
    //    DISP_VAL("* ->channl_length  ", mad_iovec->length);
    //    DISP_VAL("* ->channl_nb_seg  ", mad_iovec->total_nb_seg);
    //    DISP("***************");
    //}

    tbx_slist_append(driver->s_msg_slist, mad_iovec);
    //DISP("ajout dans les a traiter");

    tbx_slist_append(connection->packs_list, mad_iovec);
    //DISP_VAL("mad_communication : AJOUT : connection->packs_list - len", tbx_slist_get_length(connection->packs_list));


    //if(receive_mode == mad_receive_EXPRESS){
    //    mad_s_make_progress(driver, adapter);
    //    mad_r_make_progress(driver, adapter, channel);
    //}

    //DISP("<--pack");
    LOG_OUT();
}


void
mad_pack2(p_mad_connection_t   connection,
          void                *buffer,
          size_t               buffer_length,
          mad_send_mode_t      send_mode,
          mad_receive_mode_t   receive_mode,
          mad_send_mode_t      next_send_mode,
          mad_receive_mode_t   next_receive_mode){

    p_mad_iovec_t             mad_iovec   = NULL;
    p_mad_channel_t           channel     = NULL;
    p_mad_adapter_t           adapter     = NULL;
    p_mad_driver_t            driver      = NULL;
    ntbx_process_lrank_t      remote_rank = -1;
    unsigned int              seq         = -1;
    LOG_IN();
    //DISP("-->pack2");

    remote_rank = connection->remote_rank;
    channel     = connection->channel;
    adapter     = channel->adapter;
    driver      = adapter->driver;
    seq         = connection->sequence;
    connection->sequence++;

    // creation du mad_iovec
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

    mad_s_make_progress(driver, adapter);

    //DISP("<--pack2");
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

  //DISP("-->end_packing");
    channel     = connection->channel;
    adapter     = channel->adapter;
    driver      = adapter->driver;
    interface   = driver->interface;
    remote_rank = connection->remote_rank;
    packs_list  = connection->packs_list;

    // on flushe tous les packs
  //DISP_VAL("************ end_packing : packs_list-len", tbx_slist_get_length(packs_list));
    while(packs_list->length){
        mad_s_make_progress(driver, adapter);
        mad_r_make_progress(driver, adapter, channel);
        //DISP_VAL("packs_list-len", tbx_slist_get_length(packs_list));
    }

    if (interface->finalize_message)
        interface->finalize_message(connection);

    // on écrase la sequence
    connection->sequence = 0;

    // on rend le verrou sur la connexion
    connection->lock = tbx_false;
    LOG_OUT();
  //DISP("<-------------------------end_packing");
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

    //DISP("------------------------------->begin_unpacking");
    // on pose un verrou sur le canal
    if (channel->reception_lock == tbx_true)
        FAILURE("mad_begin_unpacking: reception dead lock");
    channel->reception_lock = tbx_true;

    interface = channel->adapter->driver->interface;

    connection = interface->receive_message(channel);
    if (!connection)
        FAILURE("message reception failed");

    // on initialise la sequence
    channel->sequence = 0;

    LOG_OUT();
    //DISP("<--begin_unpacking");

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
  //DISP("----->unpack");
    remote_rank = connection->remote_rank;
    channel     = connection->channel;
    adapter     = channel->adapter;
    driver      = adapter->driver;
    seq         = channel->sequence;
    channel->sequence++;

    // creation du mad_iovec
    mad_iovec = mad_iovec_create(channel->id, seq);
    mad_iovec->remote_rank = remote_rank;
    mad_iovec->send_mode = send_mode;
    mad_iovec->receive_mode = receive_mode;
    mad_iovec->area_nb_seg[0] = 1;
    mad_iovec->channel = channel;
    mad_iovec_add_data(mad_iovec, buffer, buffer_length, 0);

    //{
    //    //DISP("unpack : ajout du mad_iovec dans la liste :");
    //    DISP_VAL("->channl_id      ", mad_iovec->channel_id);
    //    DISP_VAL("->channl_sequence", mad_iovec->sequence);
    //    DISP_VAL("->channl_length  ", mad_iovec->length);
    //    DISP_VAL("->channl_nb_seg  ", mad_iovec->total_nb_seg);
    //}

    tbx_slist_append(driver->r_msg_slist, mad_iovec);
    tbx_slist_append(channel->unpacks_list, mad_iovec);

    if(receive_mode == mad_receive_EXPRESS){
        //DISP_VAL("UNPACK : len unpacks list", channel->unpacks_list->length);
        //while(mad_iovec){
        while(channel->unpacks_list->length){
            mad_r_make_progress(driver, adapter, channel);
            mad_s_make_progress(driver, adapter);
        }
      //DISP("************* On a passe le express ************");
    }

    //} else {
    //    mad_r_make_progress(driver, adapter, channel);
    //}
    LOG_OUT();
    //DISP("<--unpack");
}


void
mad_unpack2(p_mad_connection_t    connection,
            void                *buffer,
            size_t               buffer_length,
            mad_send_mode_t      send_mode,
            mad_receive_mode_t   receive_mode,
            mad_send_mode_t      next_send_mode,
            mad_receive_mode_t   next_receive_mode){

    p_mad_channel_t           channel     = NULL;
    p_mad_adapter_t           adapter     = NULL;
    p_mad_driver_t            driver      = NULL;
    ntbx_process_lrank_t      remote_rank = -1;
    p_mad_iovec_t             mad_iovec   = NULL;
    unsigned int              seq         = -1;

    LOG_IN();
    //DISP("-->unpack2");
    remote_rank = connection->remote_rank;
    channel     = connection->channel;
    adapter     = channel->adapter;
    driver      = adapter->driver;
    seq         = channel->sequence;
    channel->sequence++;

    // creation du mad_iovec
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

    mad_r_make_progress(driver, adapter, channel);

    //DISP("<--unpack2");
    LOG_OUT();
}


void
mad_end_unpacking(p_mad_connection_t connection){
    p_mad_channel_t channel = NULL;
    p_mad_adapter_t adapter = NULL;
    p_mad_driver_t driver = NULL;
    p_mad_driver_interface_t interface = NULL;

    //tbx_bool_t b = tbx_true;
    LOG_IN();
  //DISP("-->end_unpacking");

    channel = connection->channel;
    adapter = channel->adapter;
    driver = adapter->driver;
    interface = driver->interface;

    // on flushe tous les unpacks

  //DISP_VAL("*************end unpaking : len unpacks list", channel->unpacks_list->length);

    while(channel->unpacks_list->length){
        //if(b == tbx_true
        //   && channel->unpacks_list->length == 1){
        //    p_mad_iovec_t temp = tbx_slist_index_get(channel->unpacks_list, 0);
        //
        //    DISP("dans la liste des unpacks, il reste :");
        //    DISP_VAL("channel_id", temp->channel_id);
        //    DISP_VAL("sequence  ", temp->sequence);
        //    DISP_VAL("length    ", temp->length);
        //
        //    if(adapter->waiting_list->length){
        //        temp = tbx_slist_index_get(adapter->waiting_list, 0);
        //
        //        DISP("dans la liste des waiting : ");
        //        DISP_VAL("channel_id", temp->channel_id);
        //        DISP_VAL("sequence  ", temp->sequence);
        //        DISP_VAL("length    ", temp->length);
        //    } else {
        //        DISP("waiting_list est VIDE");
        //    }
        //
        //    b = tbx_false;
        //}

        mad_r_make_progress(driver, adapter, channel);
        mad_s_make_progress(driver, adapter);
        //DISP_VAL("len unpacks list", channel->unpacks_list->length);
    }

    if (interface->message_received)
        interface->message_received(connection);

    // on écrase la sequence
    channel->sequence = 0;

    channel->reception_lock = tbx_false;
    LOG_OUT();
   //DISP("<--------------------------end_unpacking");
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


//void
//mad_extended_pack(p_mad_connection_t   connection,
//                  void                *buffer,
//                  size_t               buffer_length,
//                  mad_send_mode_t      send_mode,
//                  mad_receive_mode_t   receive_mode){
//    LOG_IN();
//    // Par canal, il nous faut un tableau de taille nb_connexion
//    // qui permet de stocker le mad_iovec en construction
//    // si on fait extended, on va voir si il existe dejà un mad_iovec
//    //    si oui, on le met à jour
//    //    si non, on le crée, on le complète et on le met dans le tableau
//    LOG_OUT();
//}

//void
//mad_extended_unpack(p_mad_connection_t   connection,
//                    void                *buffer,
//                    size_t               buffer_length,
//                    mad_send_mode_t      send_mode,
//                    mad_receive_mode_t   receive_mode){
//    LOG_IN();
//    // dans madeleine,
//    // il nous faut un tableau de taille nb_canaux
//    // qui permet de stocker le mad_iovec en construction
//    // si on fait extended, on va voir si il existe dejà un mad_iovec
//    //    si oui, on le met à jour
//    //    si non, on le crée, on le complète et on le met dans le tableau
//    LOG_OUT();
//}

