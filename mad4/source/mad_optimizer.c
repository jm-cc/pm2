#include <stdlib.h>
#include <stdio.h>

#include "madeleine.h"


typedef enum e_mad_iovec_track_mode {
    MAD_RDV     = 1,
    MAD_CPY     = 2,
    MAD_RDMA    = 3
} mad_iovec_track_mode_t, *p_mad_iovec_track_mode_t;

#define NB_RECEIVE_MODE 2
#define NB_TRACK_MODE   3

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
authorized(p_mad_driver_t driver,
           p_mad_iovec_t mad_iovec,
           p_mad_iovec_t next_mad_iovec){
    mad_send_mode_t first_send_mode;
    mad_receive_mode_t first_receive_mode;
    mad_iovec_track_mode_t first_track_mode;
    mad_send_mode_t next_send_mode;
    mad_receive_mode_t next_receive_mode;
    mad_iovec_track_mode_t next_track_mode;

    int i = 0;
    int j = 0;

    LOG_IN();
    first_send_mode    = mad_iovec->send_mode;
    first_receive_mode = mad_iovec->receive_mode;
    if(driver->interface->need_rdv(mad_iovec))
        first_track_mode = MAD_RDV;
    else
        first_track_mode = MAD_CPY;

    next_send_mode    = next_mad_iovec->send_mode;
    next_receive_mode = next_mad_iovec->receive_mode;;
    if(driver->interface->need_rdv(next_mad_iovec))
        next_track_mode = MAD_RDV;
    else
        next_track_mode = MAD_CPY;

    i = (first_send_mode - 1) * (NB_RECEIVE_MODE * NB_TRACK_MODE)
        + (first_receive_mode - 1) * NB_TRACK_MODE
        + (first_track_mode - 1);

//    DISP("#####");
//    DISP_VAL("first_send_mode", first_send_mode);
//    DISP_VAL("first_receive_mode", first_receive_mode);
//    DISP_VAL("NB_RECEIVE_MODE", NB_RECEIVE_MODE);
//    DISP_VAL("NB_TRACK_MODE", NB_TRACK_MODE);

    j = (next_send_mode - 1) * (NB_RECEIVE_MODE * NB_TRACK_MODE)
        + (next_receive_mode - 1) * NB_TRACK_MODE
        + (next_track_mode - 1);

//    DISP("");
//    DISP_VAL("next_send_mode", next_send_mode);
//
//
//    DISP_VAL("i------------>", i);
//    DISP_VAL("j------------>", j);
//    DISP_VAL("authorized -->", constraints[i][j]);
//    DISP("#####");

    LOG_OUT();
    return constraints[i][j];
}

void
initialize_tracks(p_mad_adapter_t adapter){
    p_mad_track_set_t track_set = NULL;
    p_mad_track_t track0 = NULL;
    p_mad_track_t track1 = NULL;
    p_mad_driver_interface_t interface = NULL;
    LOG_IN();

    if(strcmp(adapter->driver->device_name, "mx") == 0){
        interface = adapter->driver->interface;


        /* Emisssion */
        adapter->s_track_set = TBX_MALLOC(sizeof(mad_track_set_t));
        track_set = adapter->s_track_set;
        track_set->nb_track = 2;
        track_set->tracks_htable = tbx_htable_empty_table();
        track_set->tracks_tab = TBX_MALLOC(track_set->nb_track
                                           * sizeof(p_mad_track_t));
        track_set->in_more = mad_pipeline_create(1);
        track_set->max_nb_pending_iov = 1;

        track0 = TBX_MALLOC(sizeof(mad_track_t));
        track0->id = 0;
        track0->pre_posted = tbx_true;
        track0->pipeline = mad_pipeline_create(1);
        track0->status = MAD_MKP_NOTHING_TO_DO;
        tbx_htable_add(track_set->tracks_htable,
                       "cpy", track0);
        track_set->tracks_tab[0] = track0;


        track1 = TBX_MALLOC(sizeof(mad_track_t));
        track1->id = 1;
        track1->pre_posted = tbx_false;
        track1->pipeline = mad_pipeline_create(1);
        track1->status = MAD_MKP_NOTHING_TO_DO;
        tbx_htable_add(track_set->tracks_htable,
                       "rdv", track1);
        track_set->tracks_tab[1] = track1;

        track_set->cpy_track = track0;
        track_set->rdv_track = track1;

        /* Réception */
        adapter->r_track_set = TBX_MALLOC(sizeof(mad_track_set_t));
        track_set = adapter->r_track_set;
        track_set->nb_track = 2;
        track_set->tracks_htable = tbx_htable_empty_table();
        track_set->tracks_tab = TBX_MALLOC(track_set->nb_track
                                           * sizeof(p_mad_track_t));
        track_set->in_more = mad_pipeline_create(0);


        track0 = TBX_MALLOC(sizeof(mad_track_t));
        track0->id = 0;
        track0->pre_posted = tbx_true;
        track0->pipeline = mad_pipeline_create(1);
        track0->status = MAD_MKP_NOTHING_TO_DO;
        tbx_htable_add(track_set->tracks_htable,
                       "cpy", track0);
        track_set->tracks_tab[0] = track0;


        track1 = TBX_MALLOC(sizeof(mad_track_t));
        track1->id = 1;
        track1->pre_posted = tbx_false;
        track1->pipeline = mad_pipeline_create(1);
        track1->status = MAD_MKP_NOTHING_TO_DO;
        tbx_htable_add(track_set->tracks_htable,
                       "rdv", track1);
        track_set->tracks_tab[1] = track1;

        track_set->cpy_track = track0;
        track_set->rdv_track = track1;

        interface->open_track(adapter, 0);
        interface->open_track(adapter, 1);

        interface->add_pre_posted(adapter, track0);
    } else {
        FAILURE("pilote réseau non supporté");
    }
    LOG_OUT();
}


static void
mad_iovec_begin_with_data(p_mad_iovec_t mad_iovec){
    unsigned int nb_seg = 0;
    rank_t my_rank = 0;
    unsigned int i = 0;

    p_mad_channel_t    channel = NULL;
    channel_id_t channel_id = 0;
    sequence_t sequence = 0;
    length_t length = 0;

    LOG_IN();
    nb_seg = mad_iovec->total_nb_seg;
    channel = mad_iovec->channel;
    my_rank = (rank_t)channel->process_lrank;

    for(i = (nb_seg + 2) - 1; i > 1; i--){
        mad_iovec->data[i] = mad_iovec->data[i - 2];
    }

    channel_id = mad_iovec->channel->id;
    sequence = mad_iovec->sequence;
    length = mad_iovec->length;

    mad_iovec_begin_struct_iovec(mad_iovec,
                                 my_rank);

    mad_iovec_add_data_header(mad_iovec, 1,
                              channel_id,
                              sequence,
                              length);
    LOG_OUT();
}

static void
mad_iovec_continue_with_data(p_mad_iovec_t mad_iovec,
                             p_mad_iovec_t mad_iovec_data){
    channel_id_t channel_id = 0;
    sequence_t sequence = 0;
    length_t length = 0;
    unsigned int index = 0;
    LOG_IN();

    channel_id = mad_iovec_data->channel->id;
    sequence   = mad_iovec_data->sequence;
    length     = mad_iovec_data->length;
    index = mad_iovec->total_nb_seg;

    mad_iovec_add_data_header(mad_iovec, index,
                              channel_id,
                              sequence,
                              length);
    index++;
    mad_iovec_add_data(mad_iovec,
                       mad_iovec_data->data[0].iov_base,
                       length,
                       index);
    LOG_OUT();
}


static p_mad_iovec_t
mad_iovec_begin_with_rdv(p_mad_iovec_t large_mad_iovec){
    rank_t my_rank = 0;
    p_mad_channel_t    channel = NULL;
    channel_id_t channel_id = 0;
    sequence_t sequence = 0;
    length_t length = 0;
    p_mad_iovec_t mad_iovec = NULL;

    LOG_IN();
    channel = large_mad_iovec->channel;
    my_rank = (rank_t)channel->process_lrank;

    channel_id = large_mad_iovec->channel->id;
    sequence   = large_mad_iovec->sequence;
    length     = large_mad_iovec->length;

    mad_iovec = mad_iovec_create(sequence);
    mad_iovec->channel = large_mad_iovec->channel;
    mad_iovec->remote_rank = large_mad_iovec->remote_rank;

    mad_iovec_begin_struct_iovec(mad_iovec,
                                 my_rank);

    mad_iovec_add_rdv(mad_iovec, 1,
                      channel_id,
                      sequence);

    tbx_slist_append(channel->adapter->waiting_acknowlegment_list,
                     large_mad_iovec);
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
    LOG_IN();

    channel_id = large_mad_iovec->channel->id;
    sequence   = large_mad_iovec->sequence;
    length     = large_mad_iovec->length;
    index      = mad_iovec->total_nb_seg;

    mad_iovec_add_rdv(mad_iovec, index,
                      channel_id,
                      sequence);

    channel = large_mad_iovec->channel;
    tbx_slist_append(channel->adapter->waiting_acknowlegment_list,
                     large_mad_iovec);
    LOG_OUT();
}

static p_mad_iovec_t
search_new(p_mad_driver_t driver){
    p_mad_iovec_t mad_iovec = NULL;
    p_mad_iovec_t mad_iovec_prev = NULL;
    p_mad_iovec_t mad_iovec_cur = NULL;
    tbx_bool_t express = tbx_false;
    //uint32_t limit_size = 0;
    LOG_IN();
    //limit_size = driver->interface->gather_scatter_length_max();

    if(driver->s_msg_slist->length){
        mad_iovec_cur = tbx_slist_extract(driver->s_msg_slist);
    }
    if(!mad_iovec_cur)
        goto end;

   //DISP("-----------------");
    if(driver->interface->need_rdv(mad_iovec_cur)){
      //DISP("OPTIMIZER : begin with rdv");
        mad_iovec = mad_iovec_begin_with_rdv(mad_iovec_cur);
    } else {
       //DISP("OPTIMIZER : begin with data");
        mad_iovec = mad_iovec_cur;
        mad_iovec_begin_with_data(mad_iovec);
    }

    if(mad_iovec_cur->receive_mode == mad_receive_EXPRESS)
        express = tbx_true;



    mad_iovec_prev = mad_iovec_cur;

    while(mad_iovec->total_nb_seg <= 32){
        if(tbx_slist_is_nil(driver->s_msg_slist)){
            //DISP("plus d'iovec");
            break; //goto end;
        }

        mad_iovec_cur = tbx_slist_index_get(driver->s_msg_slist, 0);
        if(!mad_iovec_cur){
            //DISP("mad_iovec non trouvé");
            break; //goto end;
        }


        // si de même destination, si cela entre dans le pack courant et si on est autorisé à le prendre
        if((mad_iovec_cur->remote_rank == mad_iovec->remote_rank)
           //&& (mad_iovec->length + mad_iovec_cur->length < limit_size)
           && (authorized(driver, mad_iovec_prev, mad_iovec_cur))){
            mad_iovec_cur = tbx_slist_extract(driver->s_msg_slist);

            if(driver->interface->need_rdv(mad_iovec_cur)){
               //DISP("OPTIMIZER : continue with rdv");
                mad_iovec_continue_with_rdv(mad_iovec, mad_iovec_cur);
            } else {
               //DISP("OPTIMIZER : continue with data");
                mad_iovec_continue_with_data(mad_iovec, mad_iovec_cur);

                if(express){
                    break;
                }
            }

            if(mad_iovec_cur->receive_mode == mad_receive_EXPRESS){
                express = tbx_true;
            }
        } else {

            /*****/
            //if(mad_iovec_cur->remote_rank != mad_iovec->remote_rank)
            //DISP("mad_iovec_cur->remote_rank != mad_iovec->remote_rank");
            //else{ //if(!authorized(driver, mad_iovec_prev, mad_iovec_cur))
            //DISP("!authorized(driver, mad_iovec_prev, mad_iovec_cur)");
            //}
            /*****/

            break; //goto end;
        }
        mad_iovec_prev = mad_iovec_cur;
    }
   //DISP("-----------------");
 end:
    LOG_OUT();
    return mad_iovec;
}

tbx_bool_t
mad_s_optimize(p_mad_adapter_t adapter){
    p_mad_iovec_t mad_iovec = NULL;
    p_mad_track_t track;

    LOG_IN();

    //DISP("-->optimize");

    if(!tbx_slist_is_nil(adapter->s_ready_msg_list)){
        mad_iovec = tbx_slist_extract(adapter->s_ready_msg_list);
        track = mad_iovec->track;
    } else {
        mad_iovec = search_new(adapter->driver);
        if(mad_iovec){
            track = adapter->s_track_set->cpy_track;
            mad_iovec->track = track;
        }
    }

    if(mad_iovec){
        submit_send(adapter, track, mad_iovec);
        //DISP("<--optimize : nvl iovec soumis");
        return tbx_true;
    }
    //DISP("<--optimize : rien");
    LOG_OUT();
    return tbx_false;
}
