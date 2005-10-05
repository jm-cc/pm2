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

tbx_tick_t fin_pack;



static void
disp_s_msg_list(p_mad_adapter_t adapter){
    p_mad_iovec_t mad_iovec = NULL;
    p_tbx_slist_t list = NULL;
    int i = 0;
    LOG_IN();

    list = adapter->driver->s_msg_slist;

    DISP("");
    DISP("");

    DISP    ("#####################################");
    DISP    ("# ADAPTER->s_msg_slist");

    if(!list->length){
        DISP("# s_msg_list vide");
    } else {
        tbx_slist_ref_to_head(list);
        do{
            mad_iovec = tbx_slist_ref_get(list);
            if(!mad_iovec)
                break;

            DISP("#");
            DISP_VAL("# element", i);
            DISP_VAL("# s_msg_list : channel_id  :", mad_iovec->channel->id);
            DISP_VAL("# s_msg_list : remote_rank :", mad_iovec->remote_rank);
            DISP_VAL("# s_msg_list : sequence    :", mad_iovec->sequence);

            i++;
        }while(tbx_slist_ref_forward(list));
    }
    DISP    ("#####################################");
    LOG_OUT();
}


static void
disp_r_msg_list(p_mad_adapter_t adapter){
    //p_mad_iovec_t mad_iovec = NULL;
    //p_tbx_slist_t list = NULL;
    //int i = 0;
    //LOG_IN();
    //
    //list = adapter->driver->r_msg_slist;
    //
    //DISP("");
    //DISP("");
    //DISP    ("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    //DISP    ("! ADAPTER->r_msg_slist");
    //
    //if(!list->length){
    //    DISP("! r_msg_list vide");
    //} else {
    //    tbx_slist_ref_to_head(list);
    //    do{
    //        mad_iovec = tbx_slist_ref_get(list);
    //        if(!mad_iovec)
    //            break;
    //
    //        DISP("!");
    //        DISP_VAL("! element", i);
    //        DISP_VAL("! r_msg_list : channel_id  :", mad_iovec->channel->id);
    //        DISP_VAL("! r_msg_list : remote_rank :", mad_iovec->remote_rank);
    //        DISP_VAL("! r_msg_list : sequence    :", mad_iovec->sequence);
    //
    //        i++;
    //    }while(tbx_slist_ref_forward(list));
    //}
    //DISP    ("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    //
    //LOG_OUT();
}






/**************************************************/
/**************************************************/
/**************************************************/

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

    //DISP("-->begin_packing");

    // lock the connection
    if (connection->lock == tbx_true)
        FAILURE("mad_begin_packing: connection dead lock");
    connection->lock = tbx_true;

    if (interface->new_message)
        interface->new_message(connection);

    // initialize the sequence
    connection->sequence = 1;

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

    remote_rank = connection->remote_rank;
    channel     = connection->channel;
    adapter     = channel->adapter;
    driver      = adapter->driver;
    seq         = connection->sequence;
    connection->sequence++;

    //DISP("-->pack");

    // create a mad_iovec
    mad_iovec = mad_iovec_create(seq);
    mad_iovec->remote_rank = remote_rank;
    mad_iovec->area_nb_seg[0] = 1;
    mad_iovec->channel = channel;
    mad_iovec->send_mode = send_mode;
    mad_iovec->receive_mode = receive_mode;
    mad_iovec_add_data2(mad_iovec, buffer, buffer_length, 0);

    tbx_slist_append(driver->s_msg_slist, mad_iovec);
    tbx_slist_append(connection->packs_list, mad_iovec);

    TBX_GET_TICK(fin_pack);
    
    //disp_s_msg_list(adapter);
    //DISP("<--pack");
    LOG_OUT();
}

//void
//mad_extended_pack(p_mad_connection_t   connection,
//                  void                *buffer,
//                  size_t               buffer_length,
//                  mad_send_mode_t      send_mode,
//                  mad_receive_mode_t   receive_mode,
//                  mad_send_mode_t      next_send_mode,
//                  mad_receive_mode_t   next_receive_mode){
//    // au niveau du canal,
//    // stocker le mad_iovec en cours pour chaque connexion
//    // à chaque pack, on met à jour le mad_iovec courant
//
//    p_mad_iovec_t             mad_iovec   = NULL;
//    p_mad_channel_t           channel     = NULL;
//    p_mad_adapter_t           adapter     = NULL;
//    p_mad_driver_t            driver      = NULL;
//    ntbx_process_lrank_t      remote_rank = -1;
//    unsigned int              seq         = -1;
//    LOG_IN();
//
//    remote_rank = connection->remote_rank;
//    channel     = connection->channel;
//    adapter     = channel->adapter;
//    driver      = adapter->driver;
//    seq         = connection->sequence;
//    connection->sequence++;
//
//    // create a mad_iovec
//    mad_iovec = mad_iovec_create(channel->id, seq);
//    mad_iovec->remote_rank = remote_rank;
//    mad_iovec->area_nb_seg[0] = 1;
//    mad_iovec->channel = channel;
//    mad_iovec->send_mode = send_mode;
//    mad_iovec->receive_mode = receive_mode;
//    mad_iovec->next_send_mode = next_send_mode;
//    mad_iovec->next_receive_mode = next_receive_mode;
//    mad_iovec_add_data(mad_iovec, buffer, buffer_length, 0);
//
//    tbx_slist_append(driver->s_msg_slist, mad_iovec);
//    tbx_slist_append(connection->packs_list, mad_iovec);
//
//    LOG_OUT();
//}

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

    // flush packs
    while(packs_list->length){
        mad_s_make_progress(adapter);
        mad_r_make_progress(adapter);
    }

    //DISP("packs_list épuisée");

    if (interface->finalize_message)
        interface->finalize_message(connection);

    // reset the sequence
    connection->sequence = 0;

    // unlock the connection
    connection->lock = tbx_false;
    LOG_OUT();

    //DISP("<--end_packing");
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

    //DISP("---------------->mad_wait_packs");

    // flush packs
    while(connection->need_reception){
        //while(connection->packs_list->length){
        TBX_GET_TICK(t1);
        mad_s_make_progress(adapter);
        TBX_GET_TICK(t2);
        mad_r_make_progress(adapter);
        TBX_GET_TICK(t3);
        //}

        chrono_s_mkp_2        += TBX_TIMING_DELAY(t1, t2);
        chrono_r_mkp_2        += TBX_TIMING_DELAY(t2, t3);
        nb_chronos_mad_r_mkp_2++;
        nb_chronos_mad_s_mkp_2++;
    }

    while(connection->packs_list->length){
        TBX_GET_TICK(t1);
        mad_s_make_progress(adapter);
        TBX_GET_TICK(t2);

        chrono_s_mkp_2        += TBX_TIMING_DELAY(t1, t2);
        nb_chronos_mad_s_mkp_2++;
    }

    //DISP("<---------------mad_wait_packs");
    LOG_OUT();
}

p_mad_connection_t
mad_begin_unpacking(p_mad_channel_t channel){
    p_mad_connection_t       connection = NULL;
    p_mad_driver_interface_t interface = NULL;
    LOG_IN();

    //DISP("-->begin_unpacking");

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

    //disp_msg_list(channel->adapter);

    //DISP("<--begin_unpacking");

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
    //DISP("-->unpack");
    remote_rank = connection->remote_rank;
    channel     = connection->channel;
    adapter     = channel->adapter;
    driver      = adapter->driver;
    seq         = channel->sequence;

    channel->sequence++;

    // create a mad_iovec
    mad_iovec = mad_iovec_create(seq);
    mad_iovec->remote_rank = remote_rank;
    mad_iovec->send_mode = send_mode;
    mad_iovec->receive_mode = receive_mode;
    mad_iovec->area_nb_seg[0] = 1;
    mad_iovec->channel = channel;
    mad_iovec_add_data2(mad_iovec, buffer, buffer_length, 0);

    tbx_slist_append(channel->unpacks_list, mad_iovec);
    //tbx_slist_append(driver->r_msg_slist, mad_iovec);

    if(receive_mode == mad_receive_EXPRESS){
        while(channel->unpacks_list->length){
            mad_r_make_progress(adapter);
            mad_s_make_progress(adapter);
        }
    }
    LOG_OUT();
    //DISP("<--unpack");
}


//void
//mad_extended_unpack(p_mad_connection_t    connection,
//            void                *buffer,
//            size_t               buffer_length,
//            mad_send_mode_t      send_mode,
//            mad_receive_mode_t   receive_mode,
//            mad_send_mode_t      next_send_mode,
//            mad_receive_mode_t   next_receive_mode){
//    // au niveau de madeleine,
//    // stocker un mad_iovec en construction pour chaque canal
//    // à chaque unpack, on le met à jour
//
//    p_mad_channel_t           channel     = NULL;
//    p_mad_adapter_t           adapter     = NULL;
//    p_mad_driver_t            driver      = NULL;
//    ntbx_process_lrank_t      remote_rank = -1;
//    p_mad_iovec_t             mad_iovec   = NULL;
//    unsigned int              seq         = -1;
//    LOG_IN();
//
//    remote_rank = connection->remote_rank;
//    channel     = connection->channel;
//    adapter     = channel->adapter;
//    driver      = adapter->driver;
//    seq         = channel->sequence;
//    channel->sequence++;
//
//    // create a mad_iovec
//    mad_iovec = mad_iovec_create(channel->id, seq);
//    mad_iovec->remote_rank = remote_rank;
//    mad_iovec->area_nb_seg[0] = 1;
//    mad_iovec->channel = channel;
//    mad_iovec->send_mode = send_mode;
//    mad_iovec->receive_mode = receive_mode;
//    mad_iovec->next_send_mode = next_send_mode;
//    mad_iovec->next_receive_mode = next_receive_mode;
//    mad_iovec_add_data(mad_iovec, buffer, buffer_length, 0);
//
//    tbx_slist_append(driver->r_msg_slist, mad_iovec);
//    tbx_slist_append(channel->unpacks_list, mad_iovec);
//
//    LOG_OUT();
//}


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

    //DISP("-->end_unpacking");

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
    //DISP("<--end_unpacking");
}

//int nb_chronos_mad_r_mkp = 0;
//int nb_chronos_mad_s_mkp = 0;
//double chrono_r_mkp = 0.0;
//double chrono_s_mkp = 0.0;

void
mad_wait_unpacks(p_mad_connection_t connection){
    p_mad_channel_t channel = NULL;
    p_mad_adapter_t adapter = NULL;
    p_mad_driver_t driver = NULL;
    tbx_tick_t        t1;
    tbx_tick_t        t2;
    tbx_tick_t        t3;
     LOG_IN();

    //DISP("----------------->mad_wait_unpacks");

    channel = connection->channel;
    adapter = channel->adapter;
    driver = adapter->driver;


    while(channel->need_send){
        //while(channel->unpacks_list->length){
        TBX_GET_TICK(t1);
        mad_r_make_progress(adapter);
        TBX_GET_TICK(t2);
        mad_s_make_progress(adapter);
        TBX_GET_TICK(t3);


        //chrono_r_mkp        += TBX_TIMING_DELAY(t1, t2);
        ////chrono_s_mkp        += TBX_TIMING_DELAY(t2, t3);
        //nb_chronos_mad_r_mkp++;
        //nb_chronos_mad_s_mkp++;
        //}
    }


    while(channel->unpacks_list->length){
        TBX_GET_TICK(t1);
        mad_r_make_progress(adapter);
        TBX_GET_TICK(t2);

        //chrono_r_mkp        += TBX_TIMING_DELAY(t1, t2);
        //nb_chronos_mad_r_mkp++;
    }

    //DISP("<-----------------mad_wait_unpacks");
    LOG_OUT();
}
