#include <stdlib.h>
#include <stdio.h>

#include "madeleine.h"

int    nb_chronos_optimize = 0;
double chrono_optimize_1_2 = 0.0;
double chrono_optimize_2_3 = 0.0;
double chrono_optimize_3_4 = 0.0;
double chrono_optimize_1_4 = 0.0;



#define Y tbx_true
#define N tbx_false

tbx_bool_t constraints[18][18] = {
    //     S_SAFER                       S_LATER                         S_CHEAPER

    //R_EXP         R_ChEAP           R_EXP         R_CHEAP           R_EXP         R_CHEAP
    //RV CP RDMA    RV CP R
    { N, N, N, /**/ N, N, N, /*||||*/ N, N, N, /**/ N, N, N, /*||||*/ N, N, N, /**/ N, N, N, /*||||*/},
    { N, N, N, /**/ N, N, N, /*||||*/ N, N, N, /**/ N, N, N, /*||||*/ N, N, N, /**/ N, N, N, /*||||*/},
    { N, N, N, /**/ N, N, N, /*||||*/ N, N, N, /**/ N, N, N, /*||||*/ N, N, N, /**/ N, N, N, /*||||*/},
    /*************************||||****************************||||************************,***||||*/
    { N, N, N, /**/ N, N, N, /*||||*/ N, N, N, /**/ N, N, N, /*||||*/ N, N, N, /**/ N, N, N, /*||||*/},
    { N, N, N, /**/ N, N, N, /*||||*/ N, N, N, /**/ N, N, N, /*||||*/ N, N, N, /**/ N, N, N, /*||||*/},
    { N, N, N, /**/ N, N, N, /*||||*/ N, N, N, /**/ N, N, N, /*||||*/ N, N, N, /**/ N, N, N, /*||||*/},
    /*--------------------------------------------------------------------------------------------*/
    /*--------------------------------------------------------------------------------------------*/
    /*--------------------------------------------------------------------------------------------*/
    { N, N, N, /**/ N, N, N, /*||||*/ N, N, N, /**/ N, N, N, /*||||*/ N, N, N, /**/ N, N, N, /*||||*/},
    { N, N, N, /**/ N, N, N, /*||||*/ N, N, N, /**/ N, N, N, /*||||*/ N, N, N, /**/ N, N, N, /*||||*/},
    { N, N, N, /**/ N, N, N, /*||||*/ N, N, N, /**/ N, N, N, /*||||*/ N, N, N, /**/ N, N, N, /*||||*/},
    /*************************||||****************************||||************************,***||||*/
    { N, N, N, /**/ N, N, N, /*||||*/ N, N, N, /**/ N, N, N, /*||||*/ N, N, N, /**/ N, N, N, /*||||*/},
    { N, N, N, /**/ N, N, N, /*||||*/ N, N, N, /**/ N, N, N, /*||||*/ N, N, N, /**/ N, N, N, /*||||*/},
    { N, N, N, /**/ N, N, N, /*||||*/ N, N, N, /**/ N, N, N, /*||||*/ N, N, N, /**/ N, N, N, /*||||*/},
    /*--------------------------------------------------------------------------------------------*/
    /*--------------------------------------------------------------------------------------------*/
    /*--------------------------------------------------------------------------------------------*/
    { N, N, N, /**/ N, N, N, /*||||*/ N, N, N, /**/ N, N, N, /*||||*/ N, Y, N, /**/ N, Y, N, /*||||*/},
    { N, N, N, /**/ N, N, N, /*||||*/ N, N, N, /**/ N, N, N, /*||||*/ N, Y, N, /**/ N, Y, N, /*||||*/},
    { N, N, N, /**/ N, N, N, /*||||*/ N, N, N, /**/ N, N, N, /*||||*/ N, N, N, /**/ N, N, N, /*||||*/},
    /*************************||||****************************||||************************,***||||*/
    { N, N, N, /**/ N, N, N, /*||||*/ N, N, N, /**/ N, N, N, /*||||*/ Y, Y, N, /**/ Y, Y, N, /*||||*/},
    { N, N, N, /**/ N, N, N, /*||||*/ N, N, N, /**/ N, N, N, /*||||*/ Y, Y, N, /**/ Y, Y, N, /*||||*/},
    { N, N, N, /**/ N, N, N, /*||||*/ N, N, N, /**/ N, N, N, /*||||*/ N, N, N, /**/ N, N, N, /*||||*/},
    /*--------------------------------------------------------------------------------------------*/
    /*--------------------------------------------------------------------------------------------*/
    /*--------------------------------------------------------------------------------------------*/
};

static tbx_bool_t
authorized(p_mad_iovec_t mad_iovec,
           p_mad_iovec_t next_mad_iovec){
    return constraints[mad_iovec->matrix_entrie][next_mad_iovec->matrix_entrie];
}

void
initialize_tracks(p_mad_adapter_t adapter){
    p_mad_track_set_t track_set = NULL;
    p_mad_track_t track0 = NULL;
    p_mad_track_t track1 = NULL;
    p_mad_driver_interface_t interface = NULL;
    int i = 0;
    LOG_IN();

    if(strcmp(adapter->driver->device_name, "mx") == 0){
        interface = adapter->driver->interface;

        /* Emisssion */
        adapter->s_track_set = TBX_MALLOC(sizeof(mad_track_set_t));

        track_set = adapter->s_track_set;
        track_set->status = MAD_MKP_NOTHING_TO_DO;
        track_set->nb_track = 2;
        track_set->tracks_htable = tbx_htable_empty_table();
        track_set->tracks_tab = TBX_MALLOC(track_set->nb_track
                                           * sizeof(p_mad_track_t));

        track_set->cur  = NULL;
        track_set->next = NULL;

        track0 = TBX_MALLOC(sizeof(mad_track_t));
        track0->id = 0;
        track0->pre_posted = tbx_true;
        tbx_htable_add(track_set->tracks_htable,
                       "cpy", track0);
        track_set->tracks_tab[0] = track0;


        track1 = TBX_MALLOC(sizeof(mad_track_t));
        track1->id = 1;
        track1->pre_posted = tbx_false;
        tbx_htable_add(track_set->tracks_htable,
                       "rdv", track1);
        track_set->tracks_tab[1] = track1;

        track_set->cpy_track = track0;
        track_set->rdv_track = track1;



        /*****************************************/
        /*****************************************/
        /*****************************************/

        /* Réception */
        adapter->r_track_set = TBX_MALLOC(sizeof(mad_track_set_t));
        track_set = adapter->r_track_set;
        track_set->status = MAD_MKP_NOTHING_TO_DO;
        track_set->nb_track = 2;
        track_set->tracks_htable = tbx_htable_empty_table();
        track_set->tracks_tab = TBX_MALLOC(track_set->nb_track
                                           * sizeof(p_mad_track_t));
        track_set->reception_curs = TBX_MALLOC(track_set->nb_track
                                               * sizeof(p_mad_iovec_t));
        for(i = 0; i < track_set->nb_track; i++){
            track_set->reception_curs[i] = NULL;
        }
        track_set->nb_pending = 0;

        track0 = TBX_MALLOC(sizeof(mad_track_t));
        track0->id = 0;
        track0->pre_posted = tbx_true;
        tbx_htable_add(track_set->tracks_htable,
                       "cpy", track0);
        track_set->tracks_tab[0] = track0;

        track1 = TBX_MALLOC(sizeof(mad_track_t));
        track1->id = 1;
        track1->pre_posted = tbx_false;
        tbx_htable_add(track_set->tracks_htable,
                       "rdv", track1);
        track_set->tracks_tab[1] = track1;

        track_set->cpy_track = track0;
        track_set->rdv_track = track1;

        interface->open_track(adapter, 0);
        interface->open_track(adapter, 1);

        interface->add_pre_posted(adapter, track_set, track0);

    } else {
        FAILURE("pilote réseau non supporté");
    }
    LOG_OUT();
}


static void
mad_iovec_begin_with_data(p_mad_iovec_t mad_iovec){
    unsigned int nb_seg = 0;
    rank_t my_rank = 0;
    p_mad_channel_t    channel = NULL;
    channel_id_t channel_id = 0;
    sequence_t sequence = 0;
    length_t length = 0;

    LOG_IN();
    nb_seg     = mad_iovec->total_nb_seg;
    channel    = mad_iovec->channel;
    channel_id = channel->id;
    my_rank    = (rank_t)channel->process_lrank;
    sequence   = mad_iovec->sequence;
    length     = mad_iovec->length;

    mad_iovec_begin_struct_iovec(mad_iovec);

    mad_iovec->total_nb_seg--;
    mad_iovec_add_data_header(mad_iovec, channel_id,
                              sequence, length);
    mad_iovec->total_nb_seg++;
    LOG_OUT();
}

static void
mad_iovec_continue_with_data(p_mad_iovec_t mad_iovec,
                             p_mad_iovec_t mad_iovec_data){
    channel_id_t channel_id = 0;
    sequence_t sequence = 0;
    length_t length = 0;
    LOG_IN();

    channel_id = mad_iovec_data->channel->id;
    sequence   = mad_iovec_data->sequence;
    length     = mad_iovec_data->length;

    mad_iovec_add_data_header(mad_iovec, channel_id,
                              sequence, length);

    mad_iovec->total_nb_seg--;
    mad_iovec_add_data(mad_iovec,
                       mad_iovec_data->data[2].iov_base,
                       length);
    mad_iovec->total_nb_seg++;
    LOG_OUT();
}


static p_mad_iovec_t
mad_iovec_begin_with_rdv(p_mad_iovec_t large_mad_iovec){
    rank_t remote_rank = 0;
    p_mad_channel_t    channel = NULL;
    p_mad_connection_t cnx = NULL;
    channel_id_t channel_id = 0;
    sequence_t sequence = 0;
    length_t length = 0;
    p_mad_iovec_t mad_iovec = NULL;

    LOG_IN();
    channel = large_mad_iovec->channel;
    remote_rank = large_mad_iovec->remote_rank;
    channel_id = large_mad_iovec->channel->id;
    sequence   = large_mad_iovec->sequence;
    length     = large_mad_iovec->length;

    mad_iovec = mad_iovec_create(remote_rank,
                                 large_mad_iovec->channel,
                                 sequence, tbx_false,
                                 large_mad_iovec->send_mode,
                                 large_mad_iovec->receive_mode);
    mad_iovec_begin_struct_iovec(mad_iovec);

    mad_iovec_add_rdv(mad_iovec, channel_id, sequence);

    tbx_slist_append(channel->adapter->waiting_acknowlegment_list,
                     large_mad_iovec);

    /** Note la réception prochaine d'un acquittement **/
    cnx = tbx_darray_get(channel->out_connection_darray,
                         remote_rank);
    cnx->need_reception++;

    LOG_OUT();
    return mad_iovec;
}

static void
mad_iovec_continue_with_rdv(p_mad_iovec_t mad_iovec,
                            p_mad_iovec_t large_mad_iovec){
    channel_id_t channel_id = 0;
    sequence_t sequence = 0;
    length_t length = 0;
    unsigned int index = 0;
    p_mad_channel_t channel = NULL;
    p_mad_connection_t cnx = NULL;
    rank_t remote_rank = 0;
    LOG_IN();

    channel_id = large_mad_iovec->channel->id;
    sequence   = large_mad_iovec->sequence;
    length     = large_mad_iovec->length;
    remote_rank = large_mad_iovec->remote_rank;
    index      = mad_iovec->total_nb_seg;

    mad_iovec_add_rdv(mad_iovec, channel_id, sequence);

    channel = large_mad_iovec->channel;
    tbx_slist_append(channel->adapter->waiting_acknowlegment_list,
                     large_mad_iovec);

    /** Note la réception prochaine d'un acquittement **/
    cnx = tbx_darray_get(channel->out_connection_darray,
                         remote_rank);
    cnx->need_reception++;


    LOG_OUT();
}

p_mad_iovec_t
mad_s_optimize(p_mad_adapter_t adapter){
    p_mad_driver_t driver = NULL;
    p_tbx_slist_t s_msg_slist = NULL;
    p_mad_iovec_t mad_iovec = NULL;
    p_mad_iovec_t mad_iovec_prev = NULL;
    p_mad_iovec_t mad_iovec_cur = NULL;
    tbx_bool_t    express = tbx_false;
    driver = adapter->driver;
    s_msg_slist = driver->s_msg_slist;

    tbx_tick_t t1;
    tbx_tick_t t2;
    tbx_tick_t t3;
    tbx_tick_t t4;
    LOG_IN();
    driver = adapter->driver;
    s_msg_slist = driver->s_msg_slist;

    TBX_GET_TICK(t1);
    s_msg_slist = driver->s_msg_slist;

    if(!s_msg_slist->length){
        goto end;
    }
    mad_iovec_cur = tbx_slist_extract(s_msg_slist);
    TBX_GET_TICK(t2);
    if(!s_msg_slist->length){
        goto end;
    }
    mad_iovec_cur = tbx_slist_extract(s_msg_slist);
    TBX_GET_TICK(t2);
    if(mad_iovec_cur->need_rdv){
        //DISP("begin_with_rdv");
        mad_iovec = mad_iovec_begin_with_rdv(mad_iovec_cur);
    } else {
        //DISP("begin_with_data");
        mad_iovec = mad_iovec_cur;
        mad_iovec_begin_with_data(mad_iovec);
        mad_iovec->nb_packs++;
    }
    TBX_GET_TICK(t3);

    if(mad_iovec_cur->receive_mode == mad_receive_EXPRESS)
        express = tbx_true;

    mad_iovec_prev = mad_iovec_cur;

    while(s_msg_slist->length && mad_iovec->total_nb_seg <= 32){
        mad_iovec_cur = tbx_slist_index_get(s_msg_slist, 0);

        // si même destination et si on est autorisé à le prendre
        if((mad_iovec_cur->remote_rank == mad_iovec->remote_rank)
           && (authorized(mad_iovec_prev, mad_iovec_cur))){
            mad_iovec_cur = tbx_slist_extract(s_msg_slist);

            if(mad_iovec_cur->need_rdv){
                //DISP("continue_with_rdv");
                mad_iovec_continue_with_rdv(mad_iovec, mad_iovec_cur);
            } else {
                //DISP("continue_with_data");
                mad_iovec_continue_with_data(mad_iovec, mad_iovec_cur);
                mad_iovec->nb_packs++;

                if(express){
                    break;
                }
            }

            if(mad_iovec_cur->receive_mode == mad_receive_EXPRESS){
                express = tbx_true;
            }
        } else {
            break;
        }
        mad_iovec_prev = mad_iovec_cur;
        mad_iovec->matrix_entrie = mad_iovec_cur->matrix_entrie;
    }
    mad_iovec->track = adapter->s_track_set->cpy_track;
    mad_iovec_update_global_header(mad_iovec);
    driver->nb_pack_to_send -= mad_iovec->nb_packs;
    TBX_GET_TICK(t4);

    nb_chronos_optimize ++;
    chrono_optimize_1_2 += TBX_TIMING_DELAY(t1, t2);
    chrono_optimize_2_3 += TBX_TIMING_DELAY(t2, t3);
    chrono_optimize_3_4 += TBX_TIMING_DELAY(t3, t4);
    chrono_optimize_1_4 += TBX_TIMING_DELAY(t1, t4);

 end:
    LOG_OUT();
    return mad_iovec;
}

