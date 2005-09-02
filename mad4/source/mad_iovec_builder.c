#include "madeleine.h"
#include <assert.h>

static p_tbx_memory_t mad_iovec_key;
static p_tbx_memory_t mad_iovec_header_key;

typedef enum e_mad_iovec_type_control {
    MAD_IOVEC_GLOBAL_HEADER = 1,
    MAD_IOVEC_DATA_HEADER   = 2,
    MAD_IOVEC_RDV           = 3,
    MAD_IOVEC_ACK           = 4,
    MAD_IOVEC_DATA_TREATED    = 5,
    MAD_IOVEC_CONTROL_TREATED = 6
} mad_iovec_type_control_t, *p_mad_iovec_type_control_t;

#define get_type(header)       ( header->tab[0]        & 0xFF)
#define get_rank(header)       ((header->tab[0] >> 8)  & 0xFFFF)
#define get_nb_seg(header)     ((header->tab[0] >> 24) & 0xFF)
#define get_channel_id(header) ((header->tab[0] >> 8)  & 0xFF)
#define get_sequence(header)   ((header->tab[0] >> 16) & 0xFF)
#define get_length(header)     ( header->tab[1] )

#define get_treated_length(header) ((header->tab[0] >> 8)  & 0xFFFFFF)
#define get_remote_rank(header)    ( header->tab[1] )

#define reset(header) do{ header->tab[0] = 0; } while(0)
#define set_type(header, type)             do{ header->tab[0] &= 0xFFFFFF00; header->tab[0] |= type; } while(0)
#define set_rank(header, rank)             do{ header->tab[0] &= 0xFF0000FF; header->tab[0] |= (((uint32_t) rank) << 8); } while(0)
#define set_nb_seg(header, nb_seg)         do{ header->tab[0] &= 0x00FFFFFF; header->tab[0] |= (((uint32_t) nb_seg) << 24); } while(0)
#define set_channel_id(header, channel_id) do{ assert(channel_id < 2); header->tab[0] &= 0xFFFF00FF; header->tab[0] |= (((uint32_t) channel_id) << 8); } while(0)
#define set_sequence(header, sequence)     do{ header->tab[0] &= 0xFF00FFFF; header->tab[0] |= (((uint32_t) sequence) << 16); } while(0)
#define set_length(header, length)         do{ header->tab[1] = length; } while(0)

#define set_treated_length(header, length) do{ header->tab[0] &= 0x000000FF; header->tab[0] |= (((uint32_t) length) << 8); } while(0)
#define set_remote_rank(header, remote_rank) do{ header->tab[1] &= remote_rank; } while(0)

void
mad_iovec_init_allocators(void){
    LOG_IN();
    tbx_malloc_init(&mad_iovec_key,        sizeof(mad_iovec_t),            512, "iovecs");
    tbx_malloc_init(&mad_iovec_header_key, 2 * sizeof(mad_iovec_header_t), 512, "iovec headers");

    //tbx_malloc_init(&mad_iovec_key,        sizeof(mad_iovec_t),            1024);
    //tbx_malloc_init(&mad_iovec_header_key, 2 * sizeof(mad_iovec_header_t), 512);

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
mad_iovec_create(unsigned int   sequence){
    p_mad_iovec_t    mad_iovec   = NULL;

    LOG_IN();
    mad_iovec                  = tbx_malloc(mad_iovec_key);
    mad_iovec->length          = 0;
    mad_iovec->sequence        = sequence;
    mad_iovec->total_nb_seg    = 0;

    memset(mad_iovec->area_nb_seg, 0, 32 * sizeof(unsigned int));
    LOG_OUT();

    return mad_iovec;
}

void
mad_iovec_free_data(p_mad_iovec_t mad_iovec){
    struct iovec *data = NULL;
    unsigned int total_nb_seg = 0;
    unsigned int i = 0;

    LOG_IN();
    data = mad_iovec->data;
    total_nb_seg = mad_iovec->total_nb_seg;

    while(i < total_nb_seg){
        p_mad_iovec_header_t header = data[i].iov_base;
        uint32_t type = get_type(header);

        switch(type){
        case MAD_IOVEC_DATA_HEADER :
            //DISP("FREE : MAD_IOVEC_DATA_HEADER");
            tbx_free(mad_iovec_header_key, header);
            header = NULL;
            i += 2;
            break;

        case MAD_IOVEC_GLOBAL_HEADER :
        case MAD_IOVEC_RDV:
        case MAD_IOVEC_ACK:
            //DISP("FREE : MAD_IOVEC_GLOBA_HEADER/RDV/ACK");
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

unsigned int
mad_iovec_begin_struct_iovec(p_mad_iovec_t mad_iovec,
                             rank_t my_rank){
    p_mad_iovec_header_t header = NULL;
    LOG_IN();

    header  = tbx_malloc(mad_iovec_header_key);
    reset(header);
    set_type(header, MAD_IOVEC_GLOBAL_HEADER);
    set_rank(header, my_rank);

    mad_iovec->data[0].iov_base = header;
    mad_iovec->data[0].iov_len  = MAD_IOVEC_GLOBAL_HEADER_SIZE;

    mad_iovec->total_nb_seg ++;
    set_nb_seg(header, mad_iovec->total_nb_seg);

    mad_iovec->length += MAD_IOVEC_GLOBAL_HEADER_SIZE;
    LOG_OUT();
    return mad_iovec->total_nb_seg;
}

static void
mad_iovec_inc_nb_seg(p_mad_iovec_t mad_iovec){
    p_mad_iovec_header_t global_header = NULL;
    nb_seg_t nb_seg = 0;

    LOG_IN();
    global_header = mad_iovec->data[0].iov_base;
    nb_seg = 1 + get_nb_seg(global_header);
    mad_iovec->total_nb_seg++;
    assert(nb_seg == mad_iovec->total_nb_seg);
    set_nb_seg(global_header, nb_seg);
    LOG_OUT();
}

unsigned int
mad_iovec_add_data_header(p_mad_iovec_t mad_iovec,
                          unsigned int index,
                          channel_id_t msg_channel_id,
                          sequence_t msg_seq,
                          length_t msg_len){
    p_mad_iovec_header_t header = NULL;

    LOG_IN();
    header  = tbx_malloc(mad_iovec_header_key);

    reset(header);
    set_type(header, MAD_IOVEC_DATA_HEADER);
    set_channel_id(header, msg_channel_id);
    set_sequence(header, msg_seq);
    set_length(header, msg_len);

    mad_iovec->data[index].iov_base = header;
    mad_iovec->data[index].iov_len  = MAD_IOVEC_DATA_HEADER_SIZE;

    mad_iovec->length += MAD_IOVEC_DATA_HEADER_SIZE;

    mad_iovec_inc_nb_seg(mad_iovec);
    LOG_OUT();

    return mad_iovec->total_nb_seg;
}

unsigned int
mad_iovec_add_rdv(p_mad_iovec_t mad_iovec,
                  unsigned int index,
                  channel_id_t msg_channel_id,
                  sequence_t msg_seq){
    p_mad_iovec_header_t header = NULL;

    LOG_IN();
    header  = tbx_malloc(mad_iovec_header_key);

    reset(header);
    set_type(header, MAD_IOVEC_RDV);
    set_channel_id(header, msg_channel_id);
    set_sequence(header, msg_seq);

    mad_iovec->data[index].iov_base = header;
    mad_iovec->data[index].iov_len  = MAD_IOVEC_RDV_SIZE;

    mad_iovec->length += MAD_IOVEC_RDV_SIZE;

    mad_iovec_inc_nb_seg(mad_iovec);
    LOG_OUT();

    return mad_iovec->total_nb_seg;
}

unsigned int
mad_iovec_add_ack(p_mad_iovec_t mad_iovec,
                  unsigned int index,
                  channel_id_t msg_channel_id,
                  sequence_t msg_seq){
    p_mad_iovec_header_t header = NULL;

    LOG_IN();
    header    = tbx_malloc(mad_iovec_header_key);

    reset(header);
    set_type(header, MAD_IOVEC_ACK);
    set_channel_id(header, msg_channel_id);
    set_sequence(header, msg_seq);

    mad_iovec->data[index].iov_base = header;
    mad_iovec->data[index].iov_len  = MAD_IOVEC_ACK_SIZE;

    mad_iovec->length += MAD_IOVEC_ACK_SIZE;

    mad_iovec_inc_nb_seg(mad_iovec);
    LOG_OUT();

    return mad_iovec->total_nb_seg;
}

void
mad_iovec_add_data(p_mad_iovec_t mad_iovec,
                   void *data,
                   size_t length,
                   unsigned int index){
    LOG_IN();
    mad_iovec->data[index].iov_len = length;
    mad_iovec->data[index].iov_base = data;

    mad_iovec->length += length;
    mad_iovec_inc_nb_seg(mad_iovec);
    LOG_OUT();
}

void
mad_iovec_add_data2(p_mad_iovec_t mad_iovec,
                    void *data,
                    size_t length,
                    unsigned int index){
    LOG_IN();
    mad_iovec->data[index].iov_len = length;
    mad_iovec->data[index].iov_base = data;

    mad_iovec->length += length;
    mad_iovec->total_nb_seg++;
    LOG_OUT();
}


/*************************************************/
/*************************************************/
/*************************************************/
p_mad_iovec_t
mad_iovec_get(p_tbx_slist_t list,
              channel_id_t  channel_id,
              rank_t remote_rank,
              sequence_t  sequence){
    p_mad_iovec_t     mad_iovec = NULL;
    tbx_slist_index_t idx       = -1;

    LOG_IN();
    if(!list->length){
        goto end;
    }

    tbx_slist_ref_to_head(list);
    do{
        idx++;
        mad_iovec = tbx_slist_ref_get(list);
        if(!mad_iovec)
            break;

        if(mad_iovec->channel->id == channel_id
           && mad_iovec->remote_rank == remote_rank
           && mad_iovec->sequence == sequence)
            return tbx_slist_index_extract(list, idx);
    }while(tbx_slist_ref_forward(list));

 end:
    LOG_OUT();

    return NULL;
}

static
p_mad_iovec_t
mad_iovec_get2(p_tbx_slist_t list,
              channel_id_t  channel_id,
              rank_t remote_rank,
              sequence_t  sequence){
    p_mad_iovec_t     mad_iovec = NULL;
    tbx_slist_index_t idx       = -1;

    LOG_IN();
    if(!list->length){
        goto end;
    }

    {
        DISP("get :");
        DISP_VAL("channel_id  :", channel_id);
        DISP_VAL("remote_rank :", remote_rank);
        DISP_VAL("sequence    :", sequence);
    }


    tbx_slist_ref_to_head(list);
    do{
        idx++;
        mad_iovec = tbx_slist_ref_get(list);
        if(!mad_iovec)
            break;

        DISP("");
        DISP_VAL("cur_mad_iovec->channel_id  :", mad_iovec->channel->id);
        DISP_VAL("cur_mad_iovec->remote_rank :", mad_iovec->remote_rank);
        DISP_VAL("cur_mad_iovec->sequence    :", mad_iovec->sequence);


        if(mad_iovec->channel->id == channel_id
           && mad_iovec->remote_rank == remote_rank
           && mad_iovec->sequence == sequence)
            return tbx_slist_index_extract(list, idx);
    }while(tbx_slist_ref_forward(list));

    DISP("get : not found");

 end:
    LOG_OUT();

    return NULL;
}







p_mad_iovec_t
mad_iovec_get_smaller(p_tbx_slist_t list,
                      channel_id_t  channel_id,
                      rank_t remote_rank,
                      sequence_t  sequence,
                      size_t length){
    p_mad_iovec_t     mad_iovec = NULL;
    tbx_slist_index_t idx       = -1;

    LOG_IN();
    if(!list->length){
        goto end;
    }

    tbx_slist_ref_to_head(list);
    do{
        idx++;
        mad_iovec = tbx_slist_ref_get(list);
        if(!mad_iovec)
            break;

        if(mad_iovec->channel->id == channel_id
           && mad_iovec->remote_rank == remote_rank
           && mad_iovec->sequence == sequence
           && mad_iovec->length <= length)
            return tbx_slist_index_extract(list, idx);
    }while(tbx_slist_ref_forward(list));

 end:
    LOG_OUT();

    return NULL;
}


p_mad_iovec_t
mad_iovec_search(p_tbx_slist_t list,
                 channel_id_t  channel_id,
                 rank_t remote_rank,
                 sequence_t  sequence){
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

    p_mad_iovec_t mad_iovec   = NULL;
    p_tbx_slist_t r_msg_slist = NULL;

    channel_id_t msg_channel_id = 0;
    sequence_t   msg_seq        = 0;
    length_t     msg_len        = 0;

    void *       data    = NULL;
    tbx_bool_t   treated = tbx_false;

    LOG_IN();
    r_msg_slist = adapter->driver->r_msg_slist;

    //if((long)header % 4)
    //    FAILURE("header/channel id non aligné");

    msg_channel_id = get_channel_id(header);
    msg_seq        = get_sequence(header);
    msg_len        = get_length(header);

    // recherche du petit associé
    mad_iovec = mad_iovec_get(r_msg_slist,
                              msg_channel_id,
                              remote_rank,
                              msg_seq);
    if(mad_iovec){
        p_mad_channel_t channel = NULL;

        // identification du unpack associé
        channel = mad_iovec->channel;
        mad_iovec_get(channel->unpacks_list,
                      msg_channel_id,
                      remote_rank,
                      msg_seq);

        // transfert des données en zone utilisateur
        data = (void *)header;
        data += MAD_IOVEC_DATA_HEADER_SIZE;

        memcpy(mad_iovec->data[0].iov_base,
               data,
               mad_iovec->data[0].iov_len);

        // on marque le segment "traité"
        set_type(header, MAD_IOVEC_DATA_TREATED);
        set_treated_length(header,
                           MAD_IOVEC_DATA_HEADER_SIZE
                           + mad_iovec->data[0].iov_len);
        treated = tbx_true;

        // on libère le mad_iovec
        mad_iovec_free(mad_iovec, tbx_false);

    } else {
        treated = tbx_false;
    }
    LOG_OUT();
    return treated;
}

static void
mad_iovec_treat_ack(p_mad_adapter_t adapter,
                    rank_t remote_rank,
                    p_mad_iovec_header_t header){
    p_mad_driver_interface_t interface = NULL;
    p_tbx_htable_t tracks_htable = NULL;

    channel_id_t msg_channel_id = 0;
    sequence_t msg_seq = 0;

    p_mad_iovec_t mad_iovec = NULL;
    LOG_IN();
    interface = adapter->driver->interface;
    tracks_htable = adapter->r_track_set->tracks_htable;

    msg_channel_id = get_channel_id(header);
    msg_seq        = get_sequence(header);

    // recherche du mad_iovec parmi ceux à acquitter
    mad_iovec = mad_iovec_get(adapter->waiting_acknowlegment_list,
                              msg_channel_id,
                              remote_rank,
                              msg_seq);
    if(!mad_iovec)
        FAILURE("mad_iovec with rdv not found");

    // choix de la piste à emprunter
    mad_iovec->track = tbx_htable_get(tracks_htable, "rdv");

    // si la piste est libre, on envoie directement
    if(!mad_iovec->track->pipeline->cur_nb_elm){
        mad_pipeline_add(mad_iovec->track->pipeline, mad_iovec);
        interface->isend(mad_iovec->track,
                         remote_rank,
                         mad_iovec->data,
                         mad_iovec->total_nb_seg);

    } else { // sinon on met en attente
        tbx_slist_append(adapter->s_ready_msg_list, mad_iovec);
    }

    // on marque le segment "traité"
    set_type(header, MAD_IOVEC_CONTROL_TREATED);
    set_treated_length(header, MAD_IOVEC_ACK_SIZE);
    LOG_OUT();
}


static void
mad_iovec_treat_rdv(p_mad_adapter_t adapter,
                    rank_t destination,
                    p_mad_iovec_header_t header){
    p_mad_driver_interface_t interface = NULL;
    p_tbx_htable_t tracks_htable = NULL;
    p_mad_track_t track = NULL;

    p_mad_iovec_t  large_iov   = NULL;
    p_mad_iovec_t  ack    = NULL;
    rank_t         my_rank           = 0;

    channel_id_t   msg_channel_id = 0;
    sequence_t     msg_seq = 0;


    p_mad_madeleine_t madeleine = NULL;
    p_mad_channel_t channel = NULL;
    p_mad_iovec_header_t rdv = NULL;

    LOG_IN();
    interface = adapter->driver->interface;

    tracks_htable = adapter->r_track_set->tracks_htable;
    //!!! On suppose qu'il n'y a qu'une piste "rdv"
    track = tbx_htable_get(tracks_htable, "rdv");


    msg_channel_id = get_channel_id(header);
    msg_seq        = get_sequence(header);
    my_rank = adapter->driver->madeleine->session->process_rank;



    //Si la piste est occupée, on stocke directement le rdv
    if(track->pipeline->cur_nb_elm){
        goto stock;
    }

    // recherche du mad_iovec parmi tous les unpacks
    large_iov = mad_iovec_get(adapter->driver->r_msg_slist,
                              msg_channel_id,
                              destination,
                              msg_seq);

    if(large_iov){
        // dépot de la réception associée
        large_iov->track = track;
        mad_pipeline_add(track->pipeline, large_iov);
        interface->irecv(track,
                         large_iov->data,
                         large_iov->total_nb_seg);

        // création de l'acquittement
        ack = mad_iovec_create(0);
        ack->remote_rank = destination;
        ack->channel = large_iov->channel;
        ack->track = tbx_htable_get(tracks_htable, "cpy");

        mad_iovec_begin_struct_iovec(ack, my_rank);
        mad_iovec_add_ack(ack, 1, msg_channel_id, msg_seq);

        tbx_slist_append(adapter->s_ready_msg_list, ack);
        goto finalize;
    }


 stock:
    // stockage de la demande de rdv
    madeleine = mad_get_madeleine();
    channel = madeleine->channel_tab[msg_channel_id];

    rdv = tbx_malloc(mad_iovec_header_key);

    set_type       (rdv, MAD_IOVEC_RDV);
    set_channel_id (rdv, msg_channel_id);
    set_sequence   (rdv, msg_seq);
    set_remote_rank(rdv, destination);

    tbx_slist_append(adapter->rdv, rdv);



 finalize:
    // on marque le segment "traité"
    set_type(header, MAD_IOVEC_CONTROL_TREATED);
    set_treated_length(header, MAD_IOVEC_RDV_SIZE);
    LOG_OUT();
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
    p_mad_madeleine_t madeleine = NULL;
    p_mad_driver_interface_t interface   = NULL;

    p_mad_iovec_header_t header = NULL;
    type_t               type   = 0;
    nb_seg_t             nb_seg = 0;

    channel_id_t  channel_id  = 0;
    p_mad_channel_t channel = NULL;
    p_mad_connection_t cnx = NULL;
    rank_t        remote_rank = 0;
    sequence_t    sequence    = 0;

    p_mad_iovec_t tmp = NULL;

    unsigned int i = 0;
    LOG_IN();
    remote_rank = mad_iovec->remote_rank;
    interface = adapter->driver->interface;



    //if(interface->need_rdv(mad_iovec)){
    //    cnx = tbx_darray_get(mad_iovec->channel->out_connection_darray,
    //                         remote_rank);
    //
    //
    //
    //    tmp = mad_iovec_get(cnx->packs_list,
    //                        mad_iovec->channel->id,
    //                        remote_rank,
    //                        mad_iovec->sequence);
    //
    //    if(!tmp)
    //        FAILURE("gros iovec envoyé non retrouvé");
    //
    //    //DISP("RETRAIT d'un gros dans cnx->packs_list");
    //
    //    mad_iovec_free(tmp, tbx_false);
    //
    //} else {
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

                //DISP("RETRAIT d'un petit dans cnx->packs_list");

                //---->mad_iovec_free(tmp, tbx_false);

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
        //mad_iovec_free(tmp, tbx_true);
        //}

    LOG_OUT();
}

//*********************************************************


tbx_bool_t
mad_iovec_exploit_msg(p_mad_adapter_t a,
                      void * msg){
    p_mad_iovec_header_t header        = NULL;
    type_t type = 0;
    rank_t  source         = 0;
    nb_seg_t nb_seg = 0;
    unsigned int nb_seg_not_treated = 0;
    tbx_bool_t treated = tbx_true;
    LOG_IN();
    //mad_iovec_print_msg_ligth(msg);
    //mad_iovec_print_msg(msg);

    header = (p_mad_iovec_header_t)msg;

    type = get_type(header);
    assert (type == MAD_IOVEC_GLOBAL_HEADER);

    source      = get_rank(header);

    nb_seg = get_nb_seg(header);
    assert (nb_seg > 0);

    header     += MAD_IOVEC_GLOBAL_HEADER_NB_ELM;

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
    LOG_OUT();
    return treated;
}


/*******************/

static tbx_bool_t
mad_iovec_treat_data_header2(p_mad_adapter_t adapter,
                             rank_t remote_rank,
                             p_mad_iovec_header_t header){

    p_mad_iovec_t mad_iovec   = NULL;
    p_tbx_slist_t r_msg_slist = NULL;

    channel_id_t msg_channel_id = 0;
    sequence_t   msg_seq        = 0;
    length_t     msg_len        = 0;

    void *       data    = NULL;
    tbx_bool_t   treated = tbx_false;

    LOG_IN();
    r_msg_slist = adapter->driver->r_msg_slist;

    //if((long)header % 4)
    //    FAILURE("header/channel id non aligné");

    msg_channel_id = get_channel_id(header);
    msg_seq        = get_sequence(header);
    msg_len        = get_length(header);

    // recherche du petit associé
    mad_iovec = mad_iovec_get2(r_msg_slist,
                              msg_channel_id,
                              remote_rank,
                              msg_seq);
    if(mad_iovec){
        p_mad_channel_t channel = NULL;

        // identification du unpack associé
        channel = mad_iovec->channel;
        mad_iovec_get(channel->unpacks_list,
                      msg_channel_id,
                      remote_rank,
                      msg_seq);

        // transfert des données en zone utilisateur
        data = (void *)header;
        data += MAD_IOVEC_DATA_HEADER_SIZE;

        memcpy(mad_iovec->data[0].iov_base,
               data,
               mad_iovec->data[0].iov_len);

        // on marque le segment "traité"
        set_type(header, MAD_IOVEC_DATA_TREATED);
        set_treated_length(header,
                           MAD_IOVEC_DATA_HEADER_SIZE
                           + mad_iovec->data[0].iov_len);
        treated = tbx_true;

        // on libère le mad_iovec
        mad_iovec_free(mad_iovec, tbx_false);

    } else {
        treated = tbx_false;
    }
    LOG_OUT();
    return treated;
}


tbx_bool_t
mad_iovec_exploit_msg2(p_mad_adapter_t a,
                       void * msg){
    p_mad_iovec_header_t header        = NULL;
    type_t type = 0;
    rank_t  source         = 0;
    nb_seg_t nb_seg = 0;
    unsigned int nb_seg_not_treated = 0;
    tbx_bool_t treated = tbx_true;
    LOG_IN();
    //mad_iovec_print_msg_ligth(msg);
    mad_iovec_print_msg(msg);

    header = (p_mad_iovec_header_t)msg;

    type = get_type(header);
    assert (type == MAD_IOVEC_GLOBAL_HEADER);

    source      = get_rank(header);

    nb_seg = get_nb_seg(header);
    assert (nb_seg > 0);

    header     += MAD_IOVEC_GLOBAL_HEADER_NB_ELM;

    while(--nb_seg){
        length_t msg_len = 0;

        type = get_type(header);
        switch(type){

        case MAD_IOVEC_DATA_HEADER :
            if(!mad_iovec_treat_data_header2(a, source, header)){
                treated = tbx_false;
                DISP_VAL("treated", treated);
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
    LOG_OUT();
    return treated;
}






/**************************************************************/
//tbx_bool_t
//mad_iovec_check_unexpected_rdv(p_mad_adapter_t adapter,
//                               p_mad_iovec_t mad_iovec){
//    p_tbx_slist_t unexpected_rdv = NULL;
//    p_mad_iovec_header_t rdv = NULL;
//    tbx_slist_index_t idx       = -1;
//    tbx_bool_t found = tbx_false;
//
//
//    //DISP("-->mad_iovec_check_unexpected_rdv");
//
//    LOG_IN();
//    unexpected_rdv = mad_iovec->channel->rdv;
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
//        if(get_channel_id(rdv) == mad_iovec->channel->id
//           && get_remote_rank(rdv) == mad_iovec->remote_rank
//           && get_sequence(rdv) == mad_iovec->sequence){
//            p_mad_iovec_t  ack     = NULL;
//            rank_t         my_rank = 0;
//
//            //DISP("TROUVÉ");
//            tbx_slist_index_extract(unexpected_rdv, idx);
//
//            // post de la zone
//            mad_iovec->track = tbx_htable_get(adapter->r_track_set->tracks_htable, "rdv");
//            if(!mad_iovec->track)
//                FAILURE("Track not found");
//
//            tbx_slist_append(adapter->r_ready_msg_list, mad_iovec);
//            // post de l'acquittement
//            ack = mad_iovec_create(mad_iovec->channel->id,
//                                   0); // sequence
//            ack->remote_rank = mad_iovec->remote_rank;
//            ack->channel     = mad_iovec->channel;
//            ack->track = tbx_htable_get(adapter->s_track_set->tracks_htable, "cpy");
//            if(!ack->track)
//                FAILURE("Track not found");
//
//            my_rank = adapter->driver->madeleine->session->process_rank;
//            mad_iovec_begin_struct_iovec(ack, my_rank);
//            mad_iovec_add_ack(ack, 1,
//                              mad_iovec->channel->id,
//                              mad_iovec->sequence);
//
//            tbx_slist_append(adapter->s_ready_msg_list, ack);
//            found = tbx_true;
//
//            goto end;
//        }
//
//
//    }while(tbx_slist_ref_forward(unexpected_rdv));
//
// end:
//    //DISP("<--mad_iovec_check_unexpected_rdv");
//    LOG_OUT();
//
//    return found;
//}

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
