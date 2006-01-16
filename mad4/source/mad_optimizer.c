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
initialize_tracks_part1(p_mad_adapter_t adapter){
    p_mad_track_set_t track_set = NULL;
    p_mad_track_t track0 = NULL;
    p_mad_track_t track1 = NULL;
    p_mad_driver_interface_t interface = NULL;
    int nb_dest = 0;
    p_mad_madeleine_t madeleine = NULL;

    int i = 0;
    LOG_IN();
    //DISP("-->initialize_tracks_part1");

    interface = adapter->driver->interface;

    madeleine = mad_get_madeleine();
    nb_dest = tbx_slist_get_length(madeleine->dir->node_slist);

    if(strcmp(adapter->driver->device_name, "tcp") == 0){
        /* Emisssion */
        adapter->s_track_set     = TBX_MALLOC(sizeof(mad_track_set_t));

        track_set                = adapter->s_track_set;
        track_set->status        = MAD_MKP_NOTHING_TO_DO;
        track_set->nb_track      = 2;
        track_set->tracks_htable = tbx_htable_empty_table();
        track_set->tracks_tab    = TBX_MALLOC(track_set->nb_track
                                              * sizeof(p_mad_track_t));

        track_set->cur  = NULL;
        track_set->next = NULL;

        track_set->reception_tracks_in_use = NULL;
        track_set->nb_pending_reception    = 0;

        // ->track CPY
        track0                   = TBX_MALLOC(sizeof(mad_track_t));
        track0->id               = 0;
        track0->track_set        = track_set;
        track0->pre_posted       = tbx_true;

        track0->nb_dest          = nb_dest;
        track0->local_ports      = TBX_MALLOC(nb_dest * sizeof(p_mad_port_t));
        for(i = 0; i < nb_dest; i++){
            track0->local_ports[i] = TBX_MALLOC(sizeof(mad_port_t));
        }
        track0->remote_ports    = TBX_MALLOC(nb_dest * sizeof(p_mad_track_t));

        track0->pending_reception    = TBX_MALLOC(nb_dest * sizeof(p_mad_iovec_t));
        track0->nb_pending_reception = 0;

        tbx_htable_add(track_set->tracks_htable, "cpy", track0);
        track_set->tracks_tab[0] = track0;

        // ->track RDV
        track1                   = TBX_MALLOC(sizeof(mad_track_t));
        track1->id               = 1;
        track1->track_set        = track_set;
        track1->pre_posted       = tbx_false;

        track1->nb_dest          = nb_dest;
        track1->local_ports      = TBX_MALLOC(nb_dest * sizeof(p_mad_port_t));
        for(i = 0; i < nb_dest; i++){
            track1->local_ports[i] = TBX_MALLOC(sizeof(mad_port_t));
        }
        track1->remote_ports    = TBX_MALLOC(nb_dest * sizeof(p_mad_track_t));

        track1->pending_reception    = NULL;
        track1->nb_pending_reception = 0;

        tbx_htable_add(track_set->tracks_htable, "direct", track1);
        track_set->tracks_tab[1] = track1;



        track_set->cpy_track    = track0;
        track_set->rdv_track    = track1;

        if(interface->track_set_init)
            interface->track_set_init(track_set);

        /*****************************************/
        /*****************************************/
        /*****************************************/

        /* Réception */
        adapter->r_track_set      = TBX_MALLOC(sizeof(mad_track_set_t));

        track_set                 = adapter->r_track_set;
        track_set->status         = MAD_MKP_NOTHING_TO_DO;
        track_set->nb_track       = 2;
        track_set->tracks_htable  = tbx_htable_empty_table();
        track_set->tracks_tab     = TBX_MALLOC(track_set->nb_track
                                              * sizeof(p_mad_track_t));

        track_set->cur  = NULL;
        track_set->next = NULL;

        track_set->reception_tracks_in_use = TBX_MALLOC(track_set->nb_track
                                                        * sizeof(tbx_bool_t));
        for(i = 0; i < track_set->nb_track; i++){
            track_set->reception_tracks_in_use[i] = tbx_false;
        }
        track_set->nb_pending_reception    = 0;



        // ->track CPY
        track0                   = TBX_MALLOC(sizeof(mad_track_t));
        track0->id               = 0;
        track0->track_set        = track_set;
        track0->pre_posted       = tbx_true;

        track0->nb_dest          = nb_dest;
        track0->local_ports      = TBX_MALLOC(nb_dest * sizeof(p_mad_port_t));
        for(i = 0; i < nb_dest; i++){
            track0->local_ports[i] = TBX_MALLOC(sizeof(mad_port_t));
        }
        track0->remote_ports    = TBX_MALLOC(nb_dest * sizeof(p_mad_track_t));


        track0->pending_reception = TBX_MALLOC(nb_dest * sizeof(p_mad_iovec_t));
        track0->nb_pending_reception = 0;

        tbx_htable_add(track_set->tracks_htable, "cpy", track0);
        track_set->tracks_tab[0] = track0;


        // ->track RDV
        track1                   = TBX_MALLOC(sizeof(mad_track_t));
        track1->id               = 1;
        track1->track_set        = track_set;
        track1->pre_posted       = tbx_false;

        track1->nb_dest          = nb_dest;
        track1->local_ports      = TBX_MALLOC(nb_dest * sizeof(p_mad_port_t));
        for(i = 0; i < nb_dest; i++){
            track1->local_ports[i] = TBX_MALLOC(sizeof(mad_port_t));
        }
        track1->remote_ports    = TBX_MALLOC(nb_dest * sizeof(p_mad_track_t));


        track1->pending_reception = TBX_MALLOC(nb_dest * sizeof(p_mad_iovec_t));
        track1->nb_pending_reception = 0;

        tbx_htable_add(track_set->tracks_htable, "direct", track1);
        track_set->tracks_tab[1] = track1;


        track_set->cpy_track    = track0;
        track_set->rdv_track    = track1;

        interface->track_init(adapter, 0);
        interface->track_init(adapter, 1);


        //interface->add_pre_posted(adapter, track_set);
        if(interface->track_set_init)
            interface->track_set_init(track_set);



    }else if(strcmp(adapter->driver->device_name, "mx") == 0){
        p_mad_port_t port = NULL;

        /* Emisssion */
        adapter->s_track_set     = TBX_MALLOC(sizeof(mad_track_set_t));

        track_set                = adapter->s_track_set;
        track_set->status        = MAD_MKP_NOTHING_TO_DO;
        track_set->nb_track      = 2;
        track_set->tracks_htable = tbx_htable_empty_table();
        track_set->tracks_tab    = TBX_MALLOC(track_set->nb_track
                                              * sizeof(p_mad_track_t));

        track_set->cur  = NULL;
        track_set->next = NULL;

        track_set->reception_tracks_in_use = NULL;
        track_set->nb_pending_reception    = 0;

        // ->track CPY
        track0                   = TBX_MALLOC(sizeof(mad_track_t));
        track0->id               = 0;
        track0->track_set        = track_set;
        track0->pre_posted       = tbx_true;

        track0->nb_dest          = nb_dest;
        track0->local_ports      = TBX_MALLOC(nb_dest * sizeof(p_mad_port_t));

        port = TBX_MALLOC(sizeof(mad_port_t));
        for(i = 0; i < nb_dest; i++){
            track0->local_ports[i] = port;
        }
        track0->remote_ports    = TBX_MALLOC(nb_dest * sizeof(p_mad_track_t));

        track0->pending_reception    = TBX_MALLOC(nb_dest * sizeof(p_mad_iovec_t));
        track0->nb_pending_reception = 0;

        tbx_htable_add(track_set->tracks_htable, "cpy", track0);
        track_set->tracks_tab[0] = track0;

        // ->track RDV
        track1                   = TBX_MALLOC(sizeof(mad_track_t));
        track1->id               = 1;
        track1->track_set        = track_set;
        track1->pre_posted       = tbx_false;

        track1->nb_dest          = nb_dest;
        track1->local_ports      = TBX_MALLOC(nb_dest * sizeof(p_mad_port_t));
        port = TBX_MALLOC(sizeof(mad_port_t));
        for(i = 0; i < nb_dest; i++){
            track1->local_ports[i] = port;
        }
        track1->remote_ports    = TBX_MALLOC(nb_dest * sizeof(p_mad_track_t));

        track1->pending_reception    = NULL;
        track1->nb_pending_reception = 0;

        tbx_htable_add(track_set->tracks_htable, "direct", track1);
        track_set->tracks_tab[1] = track1;



        track_set->cpy_track    = track0;
        track_set->rdv_track    = track1;

        if(interface->track_set_init)
            interface->track_set_init(track_set);

        /*****************************************/
        /*****************************************/
        /*****************************************/

        /* Réception */
        adapter->r_track_set      = TBX_MALLOC(sizeof(mad_track_set_t));

        track_set                 = adapter->r_track_set;
        track_set->status         = MAD_MKP_NOTHING_TO_DO;
        track_set->nb_track       = 2;
        track_set->tracks_htable  = tbx_htable_empty_table();
        track_set->tracks_tab     = TBX_MALLOC(track_set->nb_track
                                              * sizeof(p_mad_track_t));

        track_set->cur  = NULL;
        track_set->next = NULL;

        track_set->reception_tracks_in_use = TBX_MALLOC(track_set->nb_track
                                                        * sizeof(tbx_bool_t));
        for(i = 0; i < track_set->nb_track; i++){
            track_set->reception_tracks_in_use[i] = tbx_false;
        }
        track_set->nb_pending_reception    = 0;



        // ->track CPY
        track0                   = TBX_MALLOC(sizeof(mad_track_t));
        track0->id               = 0;
        track0->track_set        = track_set;
        track0->pre_posted       = tbx_true;

        track0->nb_dest          = nb_dest;
        track0->local_ports      = TBX_MALLOC(nb_dest * sizeof(p_mad_port_t));
         port = TBX_MALLOC(sizeof(mad_port_t));
        for(i = 0; i < nb_dest; i++){
            track0->local_ports[i] = port;
        }
        track0->remote_ports    = TBX_MALLOC(nb_dest * sizeof(p_mad_track_t));


        track0->pending_reception = TBX_MALLOC(nb_dest * sizeof(p_mad_iovec_t));
        track0->nb_pending_reception = 0;

        tbx_htable_add(track_set->tracks_htable, "cpy", track0);
        track_set->tracks_tab[0] = track0;


        // ->track RDV
        track1                   = TBX_MALLOC(sizeof(mad_track_t));
        track1->id               = 1;
        track1->track_set        = track_set;
        track1->pre_posted       = tbx_false;

        track1->nb_dest          = nb_dest;
        track1->local_ports      = TBX_MALLOC(nb_dest * sizeof(p_mad_port_t));
         port = TBX_MALLOC(sizeof(mad_port_t));
        for(i = 0; i < nb_dest; i++){
            track1->local_ports[i] = port;
        }
        track1->remote_ports    = TBX_MALLOC(nb_dest * sizeof(p_mad_track_t));


        track1->pending_reception = TBX_MALLOC(nb_dest * sizeof(p_mad_iovec_t));
        track1->nb_pending_reception = 0;

        tbx_htable_add(track_set->tracks_htable, "direct", track1);
        track_set->tracks_tab[1] = track1;


        track_set->cpy_track    = track0;
        track_set->rdv_track    = track1;

        interface->track_init(adapter, 0);
        interface->track_init(adapter, 1);


        //interface->add_pre_posted(adapter, track_set);
        if(interface->track_set_init)
            interface->track_set_init(track_set);

    } else {
        FAILURE("pilote réseau non supporté");
    }

    //DISP("<--initialize_tracks_part1");
    LOG_OUT();
}


void
initialize_tracks_part2(p_mad_adapter_t adapter){
    p_mad_track_set_t track_set = NULL;
    p_mad_track_t     track = NULL;
    p_mad_driver_interface_t interface = NULL;
    LOG_IN();
    //DISP("-------------->initialize_tracks_part2");


    interface = adapter->driver->interface;

    track_set = adapter->r_track_set;
    track     = track_set->cpy_track;

    interface->init_pre_posted(adapter, track);

    //DISP("<--initialize_tracks_part2");

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

    mad_iovec_add_data(mad_iovec,
                       mad_iovec_data->data[2].iov_base,
                       length);
    LOG_OUT();
}


static p_mad_iovec_t
mad_iovec_begin_with_rdv(p_mad_iovec_t large_mad_iovec){
    p_mad_channel_t    channel    = NULL;
    channel_id_t       channel_id = 0;
    p_mad_connection_t cnx        = NULL;

    rank_t             remote_rank = 0;
    sequence_t         sequence = 0;
    mad_send_mode_t    send_mode;
    mad_receive_mode_t receive_mode;
    p_mad_iovec_t      rdv = NULL;

    LOG_IN();
    channel      = large_mad_iovec->channel;
    channel_id   = channel->id;
    remote_rank  = large_mad_iovec->remote_rank;
    sequence     = large_mad_iovec->sequence;
    send_mode    = large_mad_iovec->send_mode;
    receive_mode = large_mad_iovec->receive_mode;

    // création du mad_iovec contenant le RDV
    rdv = mad_iovec_create(remote_rank,
                           channel, sequence,
                           tbx_false,
                           send_mode, receive_mode);
    mad_iovec_begin_struct_iovec(rdv);
    mad_iovec_add_rdv(rdv, channel_id, sequence);


    tbx_slist_append(channel->adapter->waiting_acknowlegment_list,
                     large_mad_iovec);

    /** Note la réception prochaine d'un acquittement **/
    cnx = tbx_darray_get(channel->out_connection_darray,
                         remote_rank);
    cnx->need_reception++;

    LOG_OUT();
    return rdv;
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
    tbx_bool_t    blocked_cnx = tbx_true;
    tbx_bool_t    forward = tbx_true;

    LOG_IN();
    driver = adapter->driver;
    s_msg_slist = driver->s_msg_slist;

    if(!s_msg_slist->length){
        goto end;
    }

    tbx_slist_ref_to_head(s_msg_slist);
    do{
        mad_iovec_cur = (p_mad_iovec_t)s_msg_slist->ref->object;

        blocked_cnx = adapter->blocked_cnx[mad_iovec_cur->channel->id][mad_iovec_cur->remote_rank];

        if(!blocked_cnx){
            if(mad_iovec_cur->need_rdv){
                mad_iovec = mad_iovec_begin_with_rdv(mad_iovec_cur);
                driver->nb_pack_to_send++;
            } else {
                mad_iovec = mad_iovec_cur;
                mad_iovec_begin_with_data(mad_iovec);
                mad_iovec->nb_packs++;
            }
            if(!tbx_slist_ref_extract_and_forward(s_msg_slist, &mad_iovec_cur))
                goto end;

            break;
        }
    } while((forward = tbx_slist_ref_forward(s_msg_slist)));

    if(!forward)
        goto end;

    if(mad_iovec_cur->receive_mode == mad_receive_EXPRESS)
        express = tbx_true;

    //DISP("---------- Au premier tour ----------");
    mad_iovec_update_global_header(mad_iovec);
    //mad_iovec_print(mad_iovec);
    //DISP("-------------------------------------");

    mad_iovec_prev = mad_iovec_cur;

    do{
        mad_iovec_cur = (p_mad_iovec_t)s_msg_slist->ref->object;

        blocked_cnx = adapter->blocked_cnx[mad_iovec_cur->channel->id][mad_iovec_cur->remote_rank];

        // si même destination et si on est autorisé à le prendre
        if(!blocked_cnx
           && (mad_iovec_cur->remote_rank == mad_iovec->remote_rank)
           && (authorized(mad_iovec_prev, mad_iovec_cur))){
            forward = tbx_slist_ref_extract_and_forward(s_msg_slist, &mad_iovec_cur);

            if(mad_iovec_cur->need_rdv){
                //DISP("continue_with_rdv");
                mad_iovec_continue_with_rdv(mad_iovec, mad_iovec_cur);
                driver->nb_pack_to_send++;
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

            if(!forward)
                goto end;

        } else {
            if(!tbx_slist_ref_forward(s_msg_slist))
                goto end;
        }
        mad_iovec_prev = mad_iovec_cur;
        mad_iovec->matrix_entrie = mad_iovec_cur->matrix_entrie;
    }while(mad_iovec->total_nb_seg <= NB_ENTRIES);

 end:
    if(mad_iovec){
        mad_iovec->track = adapter->s_track_set->cpy_track;
        mad_iovec->need_rdv = tbx_false;
        mad_iovec_update_global_header(mad_iovec);
    }

    //if(mad_iovec){
    //    DISP("Produit par l'OPTIMISEUR");
    //    mad_iovec_print(mad_iovec);
    //}
    //if(mad_iovec){
    //    DISP_VAL("Optimizer : length", mad_iovec->length);
    //}

    LOG_OUT();
    return mad_iovec;
}

