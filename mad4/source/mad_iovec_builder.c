#include "madeleine.h"
#include <assert.h>

static p_tbx_memory_t mad_iovec_key;
static p_tbx_memory_t mad_iovec_header_key;

typedef enum e_mad_iovec_type_control {
    MAD_IOVEC_GLOBAL_HEADER   = 1,
    MAD_IOVEC_DATA_HEADER     = 2,
    MAD_IOVEC_RDV             = 3,
    MAD_IOVEC_ACK             = 4,
    MAD_IOVEC_DATA_TREATED    = 5,
    MAD_IOVEC_CONTROL_TREATED = 6,
    MAD_IOVEC_BLOCKED         = 7,
    MAD_IOVEC_UNBLOCKED       = 8
} mad_iovec_type_control_t, *p_mad_iovec_type_control_t;

typedef enum e_mad_iovec_track_mode {
    MAD_RDV     = 1,
    MAD_CPY     = 2,
    MAD_RDMA    = 3
} mad_iovec_track_mode_t, *p_mad_iovec_track_mode_t;

#define NB_RECEIVE_MODE 2
#define NB_TRACK_MODE   3



int    nb_chronos_s_check = 0;
double chrono_s_check     = 0.0;

//int    nb_chronos_exploit    = 0;
//double chrono_exploit_1_2    = 0.0;
//
int    nb_chronos_treat_data = 0;
double chrono_treat_data_1_2 = 0.0;
double chrono_treat_data_2_3 = 0.0;
double chrono_treat_data_3_4 = 0.0;
double chrono_treat_data_5_6 = 0.0;
double chrono_treat_data_6_7 = 0.0;
double chrono_treat_data_8_9 = 0.0;
double chrono_treat_data_1_9 = 0.0;




#define get_type(header)       ( header->tab[0]        & 0xFF)
#define get_rank(header)       ((header->tab[0] >> 8)  & 0xFFFF)
#define get_nb_seg(header)     ((header->tab[0] >> 24) & 0xFF)
#define get_channel_id(header) ((header->tab[0] >> 8)  & 0xFF)
#define get_sequence(header)   ((header->tab[0] >> 16) & 0xFF)
#define get_length(header)     ( header->tab[1] )

#define get_treated_length(header) ((header->tab[0] >> 8)  & 0xFFFFFF)
#define get_remote_rank(header)    ( header->tab[1] )

#define reset(header) do{ header->tab[0] = 0; } while(0)
#define set_type(header, type)               do{ header->tab[0] &= 0xFFFFFF00; header->tab[0] |= type; } while(0)
#define set_rank(header, rank)               do{ header->tab[0] &= 0xFF0000FF; header->tab[0] |= (((uint32_t) rank) << 8); } while(0)
#define set_nb_seg(header, nb_seg)           do{ header->tab[0] &= 0x00FFFFFF; header->tab[0] |= (((uint32_t) nb_seg) << 24); } while(0)
#define set_channel_id(header, channel_id)   do{ assert(channel_id < 2); header->tab[0] &= 0xFFFF00FF; header->tab[0] |= (((uint32_t) channel_id) << 8); } while(0)
#define set_sequence(header, sequence)       do{ header->tab[0] &= 0xFF00FFFF; header->tab[0] |= (((uint32_t) sequence) << 16); } while(0)
#define set_length(header, length)           do{ header->tab[1] = length; } while(0)

#define set_treated_length(header, length)   do{ header->tab[0] &= 0x000000FF; header->tab[0] |= (((uint32_t) length) << 8); } while(0)
#define set_remote_rank(header, remote_rank) do{ header->tab[1] &= remote_rank; } while(0)

void
mad_iovec_init_allocators(void){
    LOG_IN();
    tbx_malloc_init(&mad_iovec_key,        sizeof(mad_iovec_t),            512, "iovecs");
    tbx_malloc_init(&mad_iovec_header_key, 2 * sizeof(mad_iovec_header_t), 512, "iovec headers");
    LOG_OUT();
}

void
mad_iovec_allocators_exit(void){
    LOG_IN();
    tbx_malloc_clean(mad_iovec_key);
    tbx_malloc_clean(mad_iovec_header_key);
    LOG_OUT();
}

p_mad_iovec_t
mad_iovec_create(ntbx_process_lrank_t remote_rank,
                 p_mad_channel_t  channel,
                 unsigned int     sequence,
                 tbx_bool_t       need_rdv,
                 mad_send_mode_t    send_mode,
                 mad_receive_mode_t receive_mode){
    p_mad_iovec_t    mad_iovec   = NULL;
    ntbx_process_lrank_t my_rank = -1;
    mad_iovec_track_mode_t track_mode;
    LOG_IN();
    mad_iovec                  = tbx_malloc(mad_iovec_key);

    if(channel)
        my_rank = channel->process_lrank;
    mad_iovec->my_rank         = my_rank;
    mad_iovec->remote_rank     = remote_rank;
    mad_iovec->channel         = channel;
    mad_iovec->need_rdv         = need_rdv;
    mad_iovec->send_mode       = send_mode;
    mad_iovec->receive_mode    = receive_mode;
    mad_iovec->length          = 0;
    mad_iovec->sequence        = sequence;
    mad_iovec->total_nb_seg    = 0;
    mad_iovec->nb_packs        = 0;

    track_mode = (need_rdv) ? MAD_RDV : MAD_CPY;

    mad_iovec->matrix_entrie  =
        (send_mode - 1) * (NB_RECEIVE_MODE * NB_TRACK_MODE)
        + (receive_mode - 1) * NB_TRACK_MODE
        + (track_mode - 1);

    LOG_OUT();
    return mad_iovec;
}

void
mad_iovec_free_data(p_mad_iovec_t mad_iovec){
    struct iovec *data = NULL;
    unsigned int total_nb_seg = 0;
    unsigned int i = 0;
    p_mad_channel_t channel = NULL;

    LOG_IN();
    data = mad_iovec->data;
    total_nb_seg = mad_iovec->total_nb_seg;

    channel = mad_iovec->channel;

    while(i < total_nb_seg){
        p_mad_iovec_header_t header = data[i].iov_base;
        uint32_t type = get_type(header);

        switch(type){
        case MAD_IOVEC_DATA_HEADER :
            tbx_free(mad_iovec_header_key, header);
            header = NULL;
            i += 2;
            break;

        case MAD_IOVEC_ACK:
            channel->need_send--;
        case MAD_IOVEC_GLOBAL_HEADER :
        case MAD_IOVEC_RDV:
            tbx_free(mad_iovec_header_key, header);
            header = NULL;
            i++;
            break;

        default:
            FAILURE("invalid format");
        }
    }
    LOG_OUT();
}

void
mad_iovec_free(p_mad_iovec_t mad_iovec, tbx_bool_t headers){
    LOG_IN();
    if(headers)
        mad_iovec_free_data(mad_iovec);

    tbx_free(mad_iovec_key, mad_iovec);
    mad_iovec = NULL;
    LOG_OUT();
}

void
mad_iovec_begin_struct_iovec(p_mad_iovec_t mad_iovec){
    p_mad_iovec_header_t header = NULL;
    LOG_IN();
    header  = tbx_malloc(mad_iovec_header_key);
    reset(header);
    set_type(header, MAD_IOVEC_GLOBAL_HEADER);
    set_rank(header, mad_iovec->my_rank);

    mad_iovec->data[0].iov_base = header;
    mad_iovec->data[0].iov_len  = MAD_IOVEC_GLOBAL_HEADER_SIZE;

    mad_iovec->total_nb_seg ++;
    set_nb_seg(header, mad_iovec->total_nb_seg);

    mad_iovec->length += MAD_IOVEC_GLOBAL_HEADER_SIZE;
    LOG_OUT();
}

//static void
//mad_iovec_inc_nb_seg(p_mad_iovec_t mad_iovec){
//    p_mad_iovec_header_t global_header = NULL;
//    nb_seg_t             nb_seg        = 0;
//    LOG_IN();
//    global_header = mad_iovec->data[0].iov_base;
//    nb_seg        = 1 + get_nb_seg(global_header);
//    mad_iovec->total_nb_seg++;
//    //assert(nb_seg == mad_iovec->total_nb_seg);
//    set_nb_seg(global_header, nb_seg);
//    LOG_OUT();
//}

void
mad_iovec_update_global_header(p_mad_iovec_t mad_iovec){
    p_mad_iovec_header_t global_header = NULL;
    LOG_IN();
    global_header = mad_iovec->data[0].iov_base;
    set_nb_seg(global_header, mad_iovec->total_nb_seg);
    LOG_OUT();
}


//unsigned int
//mad_iovec_add_data_header(p_mad_iovec_t mad_iovec,
//                          unsigned int index,
//                          channel_id_t msg_channel_id,
//                          sequence_t msg_seq,
//                          length_t msg_len){
//    p_mad_iovec_header_t header = NULL;
//    LOG_IN();
//    header  = tbx_malloc(mad_iovec_header_key);
//
//    reset(header);
//    set_type(header, MAD_IOVEC_DATA_HEADER);
//    set_channel_id(header, msg_channel_id);
//    set_sequence(header, msg_seq);
//    set_length(header, msg_len);
//
//    mad_iovec->data[index].iov_base = header;
//    mad_iovec->data[index].iov_len  = MAD_IOVEC_DATA_HEADER_SIZE;
//
//    mad_iovec->length += MAD_IOVEC_DATA_HEADER_SIZE;
//
//    mad_iovec_inc_nb_seg(mad_iovec);
//    LOG_OUT();
//    return mad_iovec->total_nb_seg;
//}

void
mad_iovec_add_data_header(p_mad_iovec_t mad_iovec,
                          channel_id_t msg_channel_id,
                          sequence_t   msg_seq,
                          length_t     msg_len){
    p_mad_iovec_header_t header = NULL;
    unsigned int index = 0;

    LOG_IN();
    header  = tbx_malloc(mad_iovec_header_key);

    reset(header);
    set_type(header, MAD_IOVEC_DATA_HEADER);
    set_channel_id(header, msg_channel_id);
    set_sequence(header, msg_seq);
    set_length(header, msg_len);

    index = mad_iovec->total_nb_seg;
    mad_iovec->data[index].iov_base = header;
    mad_iovec->data[index].iov_len  = MAD_IOVEC_DATA_HEADER_SIZE;

    //DISP_VAL("add data header à l'index", index);

    mad_iovec->length += MAD_IOVEC_DATA_HEADER_SIZE;

    mad_iovec->total_nb_seg++;
    LOG_OUT();
}



void
mad_iovec_add_rdv(p_mad_iovec_t mad_iovec,
                  channel_id_t msg_channel_id,
                  sequence_t msg_seq){
    p_mad_iovec_header_t header = NULL;
    unsigned int index = 0;
    LOG_IN();
    header  = tbx_malloc(mad_iovec_header_key);

    reset(header);
    set_type(header, MAD_IOVEC_RDV);
    set_channel_id(header, msg_channel_id);
    set_sequence(header, msg_seq);

    index = mad_iovec->total_nb_seg;
    mad_iovec->data[index].iov_base = header;
    mad_iovec->data[index].iov_len  = MAD_IOVEC_RDV_SIZE;
    //DISP_VAL("add rdv à l'index", index);

    mad_iovec->length += MAD_IOVEC_RDV_SIZE;

    mad_iovec->total_nb_seg++;
    LOG_OUT();
}

void
mad_iovec_add_ack(p_mad_iovec_t mad_iovec,
                  channel_id_t msg_channel_id,
                  sequence_t msg_seq){
    p_mad_iovec_header_t header = NULL;
    unsigned int index = 0;
    LOG_IN();
    header    = tbx_malloc(mad_iovec_header_key);

    reset(header);
    set_type(header, MAD_IOVEC_ACK);
    set_channel_id(header, msg_channel_id);
    set_sequence(header, msg_seq);

    index = mad_iovec->total_nb_seg;
    mad_iovec->data[index].iov_base = header;
    mad_iovec->data[index].iov_len  = MAD_IOVEC_ACK_SIZE;
    //DISP_VAL("add ack à l'index", index);

    mad_iovec->length += MAD_IOVEC_ACK_SIZE;

    mad_iovec->total_nb_seg++;
    LOG_OUT();
}

void
mad_iovec_add_data(p_mad_iovec_t mad_iovec,
                   void *data,
                   size_t length){
    unsigned int index = 0;
    LOG_IN();
    index = mad_iovec->total_nb_seg;
    mad_iovec->data[index].iov_len = length;
    mad_iovec->data[index].iov_base = data;
    //DISP_VAL("add data à l'index", index);

    mad_iovec->length += length;
    mad_iovec->total_nb_seg++;
    LOG_OUT();
}

void
mad_iovec_add_data2(p_mad_iovec_t mad_iovec,
                    void *data,
                    size_t length,
                    int index){
    LOG_IN();
    //index = mad_iovec->total_nb_seg;
    mad_iovec->data[index].iov_len = length;
    mad_iovec->data[index].iov_base = data;
    //DISP_VAL("add data2 à l'index", index);

    mad_iovec->length += length;
    mad_iovec->total_nb_seg++;
    LOG_OUT();
}


// Pour le contrôle de flux sur les unexpected
unsigned int
mad_iovec_add_blocked(p_mad_iovec_t mad_iovec,
                      unsigned int index,
                      channel_id_t msg_channel_id){
    p_mad_iovec_header_t header = NULL;

    LOG_IN();
    header    = tbx_malloc(mad_iovec_header_key);

    reset(header);
    set_type(header, MAD_IOVEC_BLOCKED);
    set_channel_id(header, msg_channel_id);

    mad_iovec->data[index].iov_base = header;
    mad_iovec->data[index].iov_len  = MAD_IOVEC_BLOCKED_SIZE;

    mad_iovec->length += MAD_IOVEC_BLOCKED_SIZE;

    mad_iovec->total_nb_seg++;
    LOG_OUT();

    return mad_iovec->total_nb_seg;
}

unsigned int
mad_iovec_add_unblocked(p_mad_iovec_t mad_iovec,
                        unsigned int index,
                        channel_id_t msg_channel_id){
    p_mad_iovec_header_t header = NULL;

    LOG_IN();
    header    = tbx_malloc(mad_iovec_header_key);

    reset(header);
    set_type(header, MAD_IOVEC_UNBLOCKED);
    set_channel_id(header, msg_channel_id);

    mad_iovec->data[index].iov_base = header;
    mad_iovec->data[index].iov_len  = MAD_IOVEC_UNBLOCKED_SIZE;

    mad_iovec->length += MAD_IOVEC_UNBLOCKED_SIZE;

    mad_iovec->total_nb_seg++;
    LOG_OUT();

    return mad_iovec->total_nb_seg;
}


/*************************************************/
/*************************************************/
/*************************************************/
p_mad_iovec_t
mad_iovec_get(p_tbx_slist_t list,
              channel_id_t  channel_id,
              rank_t        remote_rank,
              sequence_t    sequence){
    p_mad_iovec_t mad_iovec = NULL;
    LOG_IN();
    if(!list->length){
        goto end;
    }

    tbx_slist_ref_to_head(list);
    do{
        mad_iovec = tbx_slist_ref_get(list);
        if(!mad_iovec)
            break;

        if(mad_iovec->channel->id == channel_id
           && mad_iovec->remote_rank == remote_rank
           && mad_iovec->sequence == sequence){
            tbx_slist_ref_extract_and_forward(list, &mad_iovec);
            goto end;
        }
    }while(tbx_slist_ref_forward(list));

 end:
    LOG_OUT();
    return mad_iovec;
}

p_mad_iovec_t
mad_iovec_get_smaller(p_tbx_slist_t list,
                      channel_id_t  channel_id,
                      rank_t        remote_rank,
                      sequence_t    sequence,
                      size_t        length){
    p_mad_iovec_t mad_iovec = NULL;
    LOG_IN();
    if(!list->length){
        goto end;
    }

    tbx_slist_ref_to_head(list);
    do{
        mad_iovec = tbx_slist_ref_get(list);
        if(!mad_iovec)
            break;

        if(mad_iovec->channel->id == channel_id
           && mad_iovec->remote_rank == remote_rank
           && mad_iovec->sequence == sequence
           && mad_iovec->length <= length){
            tbx_slist_ref_extract_and_forward(list, &mad_iovec);
            goto end;
        }
    }while(tbx_slist_ref_forward(list));

 end:
    LOG_OUT();
    return mad_iovec;
}


p_mad_iovec_t
mad_iovec_search(p_tbx_slist_t list,
                 channel_id_t  channel_id,
                 rank_t        remote_rank,
                 sequence_t    sequence){
    p_mad_iovec_t mad_iovec = NULL;
    LOG_IN();
    if(!list->length){
        goto end;
    }

    tbx_slist_ref_to_head(list);
    do{
        mad_iovec = tbx_slist_ref_get(list);
        if(!mad_iovec)
            break;

        if(mad_iovec->channel->id == channel_id
           && mad_iovec->remote_rank == remote_rank
           && mad_iovec->sequence == sequence)
            return mad_iovec;
    }while(tbx_slist_ref_forward(list));

 end:
    LOG_OUT();
    return NULL;
}

/*************************************************/
/*************************************************/
/*************************************************/
static tbx_bool_t
mad_iovec_treat_data_header(p_mad_adapter_t adapter,
                            rank_t remote_rank,
                            p_mad_iovec_header_t header){
    p_mad_madeleine_t madeleine = NULL;
    p_mad_iovec_t mad_iovec   = NULL;
    channel_id_t msg_channel_id = 0;
    sequence_t   msg_seq        = 0;
    length_t     msg_len        = 0;
    void *       data    = NULL;
    tbx_bool_t   treated = tbx_false;
    p_mad_channel_t channel = NULL;

    tbx_tick_t        t1;
    tbx_tick_t        t2;
    tbx_tick_t        t3;
    tbx_tick_t        t4;
    tbx_tick_t        t5;
    tbx_tick_t        t6;
    tbx_tick_t        t7;
    tbx_tick_t        t8;
    tbx_tick_t        t9;

    LOG_IN();
    msg_channel_id = get_channel_id(header);
    msg_seq        = get_sequence(header);
    msg_len        = get_length(header);
    madeleine = mad_get_madeleine();
    channel = madeleine->channel_tab[msg_channel_id];

    TBX_GET_TICK(t1);
    // recherche du petit associé
    mad_iovec = mad_iovec_get(channel->unpacks_list,
                              msg_channel_id,
                              remote_rank,
                              msg_seq);
    TBX_GET_TICK(t2);

    if(mad_iovec){
        // transfert des données en zone utilisateur
        data = (void *)header;
        data += MAD_IOVEC_DATA_HEADER_SIZE;

        TBX_GET_TICK(t3);
        memcpy(mad_iovec->data[0].iov_base,
               data,
               mad_iovec->data[0].iov_len);
        TBX_GET_TICK(t4);

        // on marque le segment "traité"
        TBX_GET_TICK(t5);
        set_type(header, MAD_IOVEC_DATA_TREATED);
        TBX_GET_TICK(t6);
        set_treated_length(header,
                           MAD_IOVEC_DATA_HEADER_SIZE
                           + mad_iovec->data[0].iov_len);
        TBX_GET_TICK(t7);
        treated = tbx_true;

        // on libère le mad_iovec
        TBX_GET_TICK(t8);
        mad_iovec_free(mad_iovec, tbx_false);
        TBX_GET_TICK(t9);

        chrono_treat_data_1_2 += TBX_TIMING_DELAY(t1, t2);
        chrono_treat_data_2_3 += TBX_TIMING_DELAY(t2, t3);
        chrono_treat_data_3_4 += TBX_TIMING_DELAY(t3, t4);
        chrono_treat_data_5_6 += TBX_TIMING_DELAY(t5, t6);
        chrono_treat_data_6_7 += TBX_TIMING_DELAY(t6, t7);
        chrono_treat_data_8_9 += TBX_TIMING_DELAY(t8, t9);
        chrono_treat_data_1_9 += TBX_TIMING_DELAY(t1, t9);
        nb_chronos_treat_data++;
    } else {
        p_mad_driver_t driver = NULL;
        int max_unexpected = 0;

        FAILURE("UNEXPECTED");

        driver = adapter->driver;
        max_unexpected = driver->max_unexpected ;

        mad_pipeline_add(channel->unexpected, mad_iovec);
        if(channel->unexpected->length == max_unexpected)
            // envoi d'un message de controle
            channel->blocked = tbx_true;
        treated = tbx_false;
    }
    LOG_OUT();
    return treated;
}

static void
mad_iovec_treat_ack(p_mad_adapter_t adapter,
                    rank_t remote_rank,
                    p_mad_iovec_header_t header){
//    p_mad_driver_interface_t interface     = NULL;
//    p_mad_track_set_t        r_track_set   = NULL;
//    p_mad_track_t            track         = NULL;
//    p_mad_pipeline_t         pipeline      = NULL;
//    channel_id_t msg_channel_id = 0;
//    sequence_t   msg_seq = 0;
//    p_mad_iovec_t mad_iovec = NULL;
//    LOG_IN();
//    interface     = adapter->driver->interface;
//    r_track_set   = adapter->r_track_set;
//    msg_channel_id = get_channel_id(header);
//    msg_seq        = get_sequence(header);
//
//    // recherche du mad_iovec parmi ceux à acquitter
//    mad_iovec = mad_iovec_get(adapter->waiting_acknowlegment_list,
//                              msg_channel_id,
//                              remote_rank,
//                              msg_seq);
//    if(!mad_iovec)
//        FAILURE("mad_iovec with rdv not found");
//
//    // choix de la piste à emprunter
//    track            = r_track_set->rdv_track;
//    mad_iovec->track = track;
//    pipeline         = track->pipeline;
//
//    // si la piste est libre, on envoie directement
//    if(!pipeline->cur_nb_elm){
//        p_mad_madeleine_t madeleine = NULL;
//        p_mad_channel_t channel = NULL;
//        p_mad_connection_t cnx = NULL;
//
//        mad_pipeline_add(pipeline, mad_iovec);
//        interface->isend(track,
//                         remote_rank,
//                         mad_iovec->data,
//                         mad_iovec->total_nb_seg);
//
//        madeleine = mad_get_madeleine();
//        channel = madeleine->channel_tab[msg_channel_id];
//        cnx = tbx_darray_get(channel->out_connection_darray,
//                             remote_rank);
//        cnx->need_reception--;
//
//    } else { // sinon on met en attente
//        tbx_slist_append(adapter->s_ready_msg_list, mad_iovec);
//    }
//
//    // on marque le segment "traité"
//    set_type(header, MAD_IOVEC_CONTROL_TREATED);
//    set_treated_length(header, MAD_IOVEC_ACK_SIZE);
//    LOG_OUT();
}


static void
mad_iovec_treat_rdv(p_mad_adapter_t adapter,
                    rank_t destination,
                    p_mad_iovec_header_t header){
//    p_mad_driver_t           driver        = NULL;
//    p_mad_driver_interface_t interface     = NULL;
//    p_mad_track_set_t        r_track_set   = NULL;
//    p_mad_track_t            track         = NULL;
//    p_mad_pipeline_t         pipeline      = NULL;
//    p_mad_iovec_t  large_iov = NULL;
//    p_mad_iovec_t  ack       = NULL;
//    rank_t         my_rank   = 0;
//    channel_id_t   msg_channel_id = 0;
//    sequence_t     msg_seq        = 0;
//    p_mad_madeleine_t    madeleine = NULL;
//    p_mad_channel_t      channel   = NULL;
//    p_mad_iovec_header_t rdv       = NULL;
//
//    LOG_IN();
//    driver    = adapter->driver;
//    interface = driver->interface;
//    r_track_set   = adapter->r_track_set;
//    track = r_track_set->rdv_track;
//    pipeline      = track->pipeline;
//    msg_channel_id = get_channel_id(header);
//    msg_seq        = get_sequence(header);
//    my_rank        = driver->madeleine->session->process_rank;
//    madeleine = mad_get_madeleine();
//    channel = madeleine->channel_tab[msg_channel_id];
//
//    //Si la piste est occupée, on stocke directement le rdv
//    if(pipeline->cur_nb_elm){
//        goto stock;
//    }
//
//    // recherche du mad_iovec parmi tous les unpacks
//    large_iov = mad_iovec_search(channel->unpacks_list,
//                                 msg_channel_id,
//                                 destination,
//                                 msg_seq);
//    if(large_iov){
//        // dépot de la réception associée
//        large_iov->track = track;
//        mad_pipeline_add(pipeline, large_iov);
//        interface->irecv(track,
//                         large_iov->data,
//                         large_iov->total_nb_seg);
//
//        // création de l'acquittement
//        ack = mad_iovec_create(destination, large_iov->channel,
//                               0, 0, 0);
//        ack->track   = adapter->r_track_set->cpy_track;
//
//        mad_iovec_begin_struct_iovec(ack, my_rank);
//        mad_iovec_add_ack(ack, 1, msg_channel_id, msg_seq);
//        ack->nb_packs++;
//
//        tbx_slist_append(adapter->s_ready_msg_list, ack);
//        driver->nb_pack_to_send++;
//
//        /** On a besoin de faire progresser le côté émetteur **/
//        channel->need_send++;
//
//        goto finalize;
//    }
// stock:
//    // stockage de la demande de rdv
//    madeleine = mad_get_madeleine();
//    channel = madeleine->channel_tab[msg_channel_id];
//
//    rdv = tbx_malloc(mad_iovec_header_key);
//
//    set_type       (rdv, MAD_IOVEC_RDV);
//    set_channel_id (rdv, msg_channel_id);
//    set_sequence   (rdv, msg_seq);
//    set_remote_rank(rdv, destination);
//
//    tbx_slist_append(adapter->rdv, rdv);
//
// finalize:
//    // on marque le segment "traité"
//    set_type(header, MAD_IOVEC_CONTROL_TREATED);
//    set_treated_length(header, MAD_IOVEC_RDV_SIZE);
//    LOG_OUT();
}

//*******************************************************************

void
mad_iovec_print_msg(void * msg){
    p_mad_iovec_header_t header        = NULL;
    type_t type = 0;
    nb_seg_t nb_seg = 0;

    LOG_IN();
    header = (p_mad_iovec_header_t)msg;
    DISP("-------------------");
    DISP_VAL("global header -> type  ", get_type(header));
    DISP_VAL("              -> rank  ", get_rank(header));
    DISP_VAL("              -> nb_seg", get_nb_seg(header));

    type = get_type(header);
    assert (type == MAD_IOVEC_GLOBAL_HEADER);

    nb_seg = get_nb_seg(header);
    assert (nb_seg > 0);

    header     += MAD_IOVEC_GLOBAL_HEADER_NB_ELM;

    while(--nb_seg){
        length_t msg_len = 0;

        type = get_type(header);
        switch(type){

        case MAD_IOVEC_DATA_HEADER :
            DISP("RECEPTION : DATA_HEADER");
            DISP_VAL("-> type ", get_type(header));
            DISP_VAL("-> ch id", get_channel_id(header));
            DISP_VAL("-> seq  ", get_sequence(header));
            DISP_VAL("-> len  ", get_length(header));

            msg_len = get_length(header);
            header += MAD_IOVEC_DATA_HEADER_NB_ELM;
            {
                void * temp = header;
                temp += msg_len;
                header = (p_mad_iovec_header_t)temp;
            }
            assert (nb_seg > 1);
            nb_seg--;
            break;

        case MAD_IOVEC_RDV:
            DISP("RECEPTION : RDV");
            DISP_VAL("-> type ", get_type(header));
            DISP_VAL("-> ch id", get_channel_id(header));
            DISP_VAL("-> seq  ", get_sequence(header));

            header += MAD_IOVEC_RDV_NB_ELM;
            break;

       case MAD_IOVEC_ACK:
           DISP("RECEPTION : ACK");
           DISP_VAL("-> type ", get_type(header));
           DISP_VAL("-> ch id", get_channel_id(header));
           DISP_VAL("-> seq  ", get_sequence(header));

           header += MAD_IOVEC_ACK_NB_ELM;
           break;

        case MAD_IOVEC_DATA_TREATED:
            DISP("RECEPTION : TREATED");
            DISP_VAL("->length", get_treated_length(header));

            {
                void * temp = header;
                temp += get_treated_length(header);
                header = (p_mad_iovec_header_t)temp;
            }
            nb_seg--;
            break;

        case MAD_IOVEC_CONTROL_TREATED:
            DISP("RECEPTION : TREATED");
            DISP_VAL("->length", get_treated_length(header));

            {
                void * temp = header;
                temp += get_treated_length(header);
                header = (p_mad_iovec_header_t)temp;
            }
            break;

        default:
            DISP("INVALID DATA");
            DISP_VAL("-> type ", get_type(header));
            DISP_VAL("-> ch id", get_channel_id(header));
            DISP_VAL("-> seq  ", get_sequence(header));
            FAILURE("invalid mad_iovec header");
        }
    }
    LOG_OUT();
    DISP("------------------------------------------");
    DISP("------------------------------------------");
}

static void
mad_iovec_print_msg_ligth(void * msg){
    p_mad_iovec_header_t header        = NULL;
    type_t type = 0;
    nb_seg_t nb_seg = 0;

    LOG_IN();
    header = (p_mad_iovec_header_t)msg;
    type = get_type(header);
    assert (type == MAD_IOVEC_GLOBAL_HEADER);

    nb_seg = get_nb_seg(header);
    assert (nb_seg > 0);

    header     += MAD_IOVEC_GLOBAL_HEADER_NB_ELM;

    while(--nb_seg){
        length_t msg_len = 0;

        type = get_type(header);
        switch(type){

        case MAD_IOVEC_DATA_HEADER :
            DISP("RECEPTION : DATA_HEADER");
            msg_len = get_length(header);
            header += MAD_IOVEC_DATA_HEADER_NB_ELM;
            {
                void * temp = header;
                temp += msg_len;
                header = (p_mad_iovec_header_t)temp;
            }
            assert (nb_seg > 1);
            nb_seg--;
            break;

        case MAD_IOVEC_RDV:
            DISP("RECEPTION : RDV");
            header += MAD_IOVEC_RDV_NB_ELM;
            break;

       case MAD_IOVEC_ACK:
           DISP("RECEPTION : ACK");
           header += MAD_IOVEC_ACK_NB_ELM;
           break;

        case MAD_IOVEC_DATA_TREATED:
            DISP("RECEPTION : DATA TREATED");
            header += MAD_IOVEC_DATA_HEADER_NB_ELM;
            {
                void * temp = header;
                temp += get_treated_length(header);
                header = (p_mad_iovec_header_t)temp;
            }
            nb_seg--;
            break;

        case MAD_IOVEC_CONTROL_TREATED:
            DISP("RECEPTION : TREATED");
            header += MAD_IOVEC_ACK_NB_ELM;
            break;

        default:
            FAILURE("invalid mad_iovec header");
        }
    }
    LOG_OUT();
}


void
mad_iovec_print_iovec(struct iovec *iovec){
    unsigned int i = 0;
    p_mad_iovec_header_t header        = NULL;
    type_t type = 0;
    nb_seg_t nb_seg = 0;

    LOG_IN();
    header = (p_mad_iovec_header_t)iovec[i++].iov_base;
    DISP_VAL("global header -> type  ", get_type(header));
    DISP_VAL("              -> rank  ", get_rank(header));
    DISP_VAL("              -> nb_seg", get_nb_seg(header));

    type = get_type(header);
    assert (type == MAD_IOVEC_GLOBAL_HEADER);

    nb_seg = get_nb_seg(header);
    assert (nb_seg > 0);


    header = iovec[i].iov_base;

    while(--nb_seg){
        type = get_type(header);
        switch(type){

        case MAD_IOVEC_DATA_HEADER :
            DISP("DATA_HEADER");
            DISP_VAL("-> type ", get_type(header));
            DISP_VAL("-> ch id", get_channel_id(header));
            DISP_VAL("-> seq  ", get_sequence(header));
            DISP_VAL("-> len  ", get_length(header));

            DISP_STR("DATA", iovec[i+1].iov_base);

            i += 2;
            header = iovec[i].iov_base;

            assert (nb_seg > 1);
            nb_seg--;
            break;

        case MAD_IOVEC_RDV:
            DISP("RDV");
            DISP_VAL("-> type ", get_type(header));
            DISP_VAL("-> ch id", get_channel_id(header));
            DISP_VAL("-> seq  ", get_sequence(header));

            header = iovec[++i].iov_base;
            break;

       case MAD_IOVEC_ACK:
           DISP("ACK");
           DISP_VAL("-> type ", get_type(header));
           DISP_VAL("-> ch id", get_channel_id(header));
           DISP_VAL("-> seq  ", get_sequence(header));

           header = iovec[++i].iov_base;
           break;

        case MAD_IOVEC_DATA_TREATED:
            DISP("DATA TREATED");
            i += 2;
            header = iovec[i].iov_base;
            break;

        case MAD_IOVEC_CONTROL_TREATED:
            DISP("CONTROL TREATED");
            header = iovec[++i].iov_base;
            break;


        default:
            FAILURE("invalid iovec header");
        }
    }
    LOG_OUT();
    DISP("------------------------------------------");
    DISP("------------------------------------------");
}

void
mad_iovec_print(p_mad_iovec_t mad_iovec){
    struct iovec *iovec = NULL;
    LOG_IN();

    iovec = (struct iovec *)mad_iovec->data;
    mad_iovec_print_iovec(iovec);
    LOG_OUT();
}

//**********************************************************
void
mad_iovec_s_check(p_mad_adapter_t adapter,
                  p_mad_iovec_t mad_iovec){
    p_mad_madeleine_t        madeleine = NULL;
    p_mad_driver_interface_t interface   = NULL;
    p_mad_iovec_header_t header = NULL;
    type_t               type   = 0;
    nb_seg_t             nb_seg = 0;
    channel_id_t       channel_id  = 0;
    p_mad_channel_t    channel = NULL;
    p_mad_connection_t cnx = NULL;
    rank_t        remote_rank = 0;
    sequence_t    sequence    = 0;
    p_mad_iovec_t tmp = NULL;
    unsigned int i = 0;

    tbx_tick_t tick_debut;
    tbx_tick_t tick_fin;
    LOG_IN();

    nb_chronos_s_check++;
    TBX_GET_TICK(tick_debut);

    remote_rank = mad_iovec->remote_rank;
    interface   = adapter->driver->interface;

    madeleine = mad_get_madeleine();
    header = (p_mad_iovec_header_t)mad_iovec->data[i++].iov_base;

    type = get_type(header);
    assert (type == MAD_IOVEC_GLOBAL_HEADER);

    nb_seg = get_nb_seg(header);
    assert (nb_seg > 0);

    header = mad_iovec->data[i].iov_base;

    while(--nb_seg){
        type = get_type(header);
        switch(type){

        case MAD_IOVEC_DATA_HEADER :
            channel_id = get_channel_id(header);
            channel = madeleine->channel_tab[channel_id];

            cnx = tbx_darray_get(channel->out_connection_darray,
                                 remote_rank);

            sequence   = get_sequence(header);

            tmp = mad_iovec_get(cnx->packs_list,
                                channel_id,
                                remote_rank,
                                sequence);

            if(!tmp)
                FAILURE("iovec envoyé non retrouvé");

            mad_iovec_free(tmp, tbx_false);

            i += 2;
            header = mad_iovec->data[i].iov_base;

            assert (nb_seg > 1);
            nb_seg--;
            break;

        case MAD_IOVEC_RDV:
        case MAD_IOVEC_ACK:
            header = mad_iovec->data[++i].iov_base;
            break;

        default:
            FAILURE("invalid mad_iovec header");
        }
    }
    LOG_OUT();
    TBX_GET_TICK(tick_fin);
    chrono_s_check += TBX_TIMING_DELAY(tick_debut, tick_fin);
}

tbx_bool_t
mad_iovec_exploit_msg(p_mad_adapter_t a,
                      void * msg){
    p_mad_iovec_header_t header        = NULL;
    type_t type = 0;
    rank_t  source         = 0;
    nb_seg_t nb_seg = 0;
    unsigned int nb_seg_not_treated = 0;
    tbx_bool_t treated = tbx_true;

    tbx_tick_t        t1;
    tbx_tick_t        t2;
    LOG_IN();
    header = (p_mad_iovec_header_t)msg;

    type = get_type(header);
    assert (type == MAD_IOVEC_GLOBAL_HEADER);

    source      = get_rank(header);

    nb_seg = get_nb_seg(header);
    assert (nb_seg > 0);

    header     += MAD_IOVEC_GLOBAL_HEADER_NB_ELM;

    TBX_GET_TICK(t1);
    while(--nb_seg){
        length_t msg_len = 0;

        type = get_type(header);
        switch(type){

        case MAD_IOVEC_DATA_HEADER :
            if(!mad_iovec_treat_data_header(a, source, header)){
                treated = tbx_false;
            }
            if(get_type(header) == MAD_IOVEC_DATA_HEADER){
                nb_seg_not_treated ++;
            }
            msg_len = get_length(header);
            header += MAD_IOVEC_DATA_HEADER_NB_ELM;
            {
                void * temp = header;
                temp += msg_len;
                header = (p_mad_iovec_header_t)temp;
            }
            assert (nb_seg > 1);
            nb_seg--;
            break;

       case MAD_IOVEC_RDV:
           mad_iovec_treat_rdv(a, source, header);
           header += MAD_IOVEC_RDV_NB_ELM;
           break;

       case MAD_IOVEC_ACK:
           mad_iovec_treat_ack(a, source, header);
           header += MAD_IOVEC_ACK_NB_ELM;
           break;

        case MAD_IOVEC_DATA_TREATED:
            msg_len = get_treated_length(header);
            {
                void * temp = header;
                temp += msg_len;
                header = (p_mad_iovec_header_t)temp;
            }
            nb_seg--;
            break;

        case MAD_IOVEC_CONTROL_TREATED:
            msg_len = get_treated_length(header);
            {
                void * temp = header;
                temp += msg_len;
                header = (p_mad_iovec_header_t)temp;
            }
            break;

        default:
            FAILURE("exploit_msg : invalid mad_iovec header");
        }
    }
    TBX_GET_TICK(t2);
    LOG_OUT();
    return treated;
}


void
mad_iovec_search_rdv(p_mad_adapter_t adapter,
                     p_mad_track_t track){

//    p_tbx_slist_t unexpected_rdv = NULL;
//    p_mad_iovec_header_t rdv = NULL;
//    tbx_slist_index_t idx       = -1;
//    p_mad_iovec_t large_iov = NULL;
//    p_mad_iovec_t mad_iovec = NULL;
//    p_mad_iovec_t new_mad_iovec = NULL;
//
//    channel_id_t   rdv_channel_id  = 0;
//    int            rdv_remote_rank = 0;
//    sequence_t     rdv_seq         = 0;
//    rank_t         my_rank         = 0;
//
//    LOG_IN();
//
//    unexpected_rdv = adapter->rdv;
//
//    if(!unexpected_rdv->length){
//        //DISP("unexpected_rdv list vide");
//        goto end;
//    }
//
//    tbx_slist_ref_to_head(unexpected_rdv);
//    do{
//        idx++;
//        rdv = tbx_slist_ref_get(unexpected_rdv);
//        if(!rdv)
//            break;
//
//        rdv_channel_id  = get_channel_id(rdv);
//        rdv_remote_rank = get_remote_rank(rdv);
//        rdv_seq         = get_sequence(rdv);
//
//        // recherche du gros associé
//        DISP("search_rdv : get sur r_msg_slist");
//        large_iov = mad_iovec_get(adapter->driver->r_msg_slist,
//                                  rdv_channel_id,
//                                  rdv_remote_rank,
//                                  rdv_seq);
//
//        if(large_iov){
//            //DISP("POST De la reception d'UN GROS");
//            large_iov->track = track;
//            mad_pipeline_add(track->pipeline, large_iov);
//            adapter->driver->interface->irecv(track,
//                                              large_iov->data,
//                                              large_iov->total_nb_seg);
//
//
//            // on crée un nouvel iovec
//            DISP("search_rdv : CREATE MAD_IOVEC");
//            new_mad_iovec = mad_iovec_create(rdv_channel_id,
//                                             0); // sequence
//            new_mad_iovec->remote_rank = rdv_remote_rank;
//
//            my_rank = adapter->driver->madeleine->session->process_rank;
//            mad_iovec_begin_struct_iovec(new_mad_iovec, my_rank);
//
//
//            //DISP("DEPOT D'UN ACK");
//            mad_iovec_add_ack(new_mad_iovec, 1, rdv_channel_id, rdv_seq);
//            new_mad_iovec->channel = large_iov->channel;
//            new_mad_iovec->track = tbx_htable_get(adapter->s_track_set->tracks_htable, "cpy");
//            new_mad_iovec->total_nb_seg = 2;
//            new_mad_iovec->area_nb_seg[0] = 2;
//
//            tbx_slist_append(adapter->s_ready_msg_list, new_mad_iovec);
//            break;
//        }
//
//    }while(tbx_slist_ref_forward(unexpected_rdv));
//
// end:
    LOG_OUT();
}
