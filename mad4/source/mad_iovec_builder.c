#include "madeleine.h"
#include <assert.h>

static p_tbx_memory_t mad_iovec_key;
static p_tbx_memory_t mad_iovec_header_key;

typedef enum e_mad_iovec_type_control {
    MAD_IOVEC_GLOBAL_HEADER = 1,
    MAD_IOVEC_DATA_HEADER   = 2,
    MAD_IOVEC_RDV           = 3,
    MAD_IOVEC_ACK           = 5,
    MAD_IOVEC_NOT_READY     = 4
} mad_iovec_type_control_t, *p_mad_iovec_type_control_t;

#define get_type(header)       ( header->tab[0]        & 0xFF)
#define get_rank(header)       ((header->tab[0] >> 8)  & 0xFFFF)
#define get_nb_seg(header)     ((header->tab[0] >> 24) & 0xFF)
#define get_channel_id(header) ((header->tab[0] >> 8)  & 0xFF)
#define get_sequence(header)   ((header->tab[0] >> 16) & 0xFF)
#define get_length(header)     ( header->tab[1] )

#define reset(header) do{ header->tab[0] = 0; } while(0)
#define set_type(header, type)             do{ header->tab[0] &= 0xFFFFFF00; header->tab[0] |= type; } while(0)
#define set_rank(header, rank)             do{ header->tab[0] &= 0xFF0000FF; header->tab[0] |= (((uint32_t) rank) << 8);      } while(0)
#define set_nb_seg(header, nb_seg)         do{ header->tab[0] &= 0x00FFFFFF; header->tab[0] |= (((uint32_t) nb_seg) << 24);      } while(0)
#define set_channel_id(header, channel_id) do{ assert(channel_id < 2); header->tab[0] &= 0xFFFF00FF; header->tab[0] |= (((uint32_t) channel_id) << 8);} while(0)
#define set_sequence(header, sequence)     do{ header->tab[0] &= 0xFF00FFFF; header->tab[0] |= (((uint32_t) sequence) << 16); } while(0)
#define set_length(header, length)         do{ header->tab[1] = length;            } while(0)

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
mad_iovec_allocators_exit(void)
{
  LOG_IN();
  tbx_malloc_clean(mad_iovec_key);
  tbx_malloc_clean(mad_iovec_header_key);
  LOG_OUT();
}


p_mad_iovec_t
mad_iovec_create(unsigned int   channel_id,
                 unsigned int   sequence){
    p_mad_iovec_t    mad_iovec   = NULL;

    LOG_IN();
    mad_iovec                  = tbx_malloc(mad_iovec_key);
    mad_iovec->channel_id      = channel_id;
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
    unsigned int i =0;

    LOG_IN();
    data = mad_iovec->data;
    total_nb_seg = mad_iovec->total_nb_seg;

    while(i < total_nb_seg){
        p_mad_iovec_header_t header = data[i].iov_base;
        uint32_t type = get_type(header);

        switch(type){
        case MAD_IOVEC_DATA_HEADER :
            tbx_free(mad_iovec_header_key, header);
            header = NULL;
            i += 2;
            break;

        case MAD_IOVEC_GLOBAL_HEADER :
        case MAD_IOVEC_RDV:
        case MAD_IOVEC_ACK:
        case MAD_IOVEC_NOT_READY:
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

    //DISP_VAL("mad_iovec_begin_struct_iovec - nb_seg",
    //         get_nb_seg(header));

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


    //if(nb_seg !=  mad_iovec->total_nb_seg){
    //    DISP_VAL("mad_iovec_inc_nb_seg - get_nb_seg(global_header)",get_nb_seg(global_header));
    //    DISP_VAL("mad_iovec_inc_nb_seg - nb_seg",
    //             nb_seg);
    //    DISP_VAL("mad_iovec_inc_nb_seg - total_nb_seg",
    //             mad_iovec->total_nb_seg);
    //}


    assert(nb_seg == mad_iovec->total_nb_seg);
    set_nb_seg(global_header, nb_seg);
    LOG_OUT();
}

unsigned int
mad_iovec_add_header_data(p_mad_iovec_t mad_iovec,
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

unsigned int
mad_iovec_add_not_ready(p_mad_iovec_t mad_iovec,
                        unsigned int index,
                        channel_id_t msg_channel_id,
                        sequence_t msg_seq){
    p_mad_iovec_header_t header = NULL;

    LOG_IN();
    header    = tbx_malloc(mad_iovec_header_key);

    reset(header);
    set_type(header, MAD_IOVEC_NOT_READY);
    set_channel_id(header, msg_channel_id);
    set_sequence(header, msg_seq);

    mad_iovec->data[index].iov_base = header;
    mad_iovec->data[index].iov_len  = MAD_IOVEC_NOT_READY_SIZE;

    mad_iovec->length += MAD_IOVEC_NOT_READY_SIZE;

    mad_iovec_inc_nb_seg(mad_iovec);
    LOG_OUT();

    return mad_iovec->total_nb_seg;
}

// a renommer en translate
void
mad_iovec_add_data(p_mad_iovec_t mad_iovec,
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

void
mad_iovec_add_data2(p_mad_iovec_t mad_iovec,
                    void *data,
                    size_t length,
                    unsigned int index){
    LOG_IN();
    mad_iovec->data[index].iov_len = length;
    mad_iovec->data[index].iov_base = data;

    mad_iovec->length += length;
    //mad_iovec->total_nb_seg++;
    mad_iovec_inc_nb_seg(mad_iovec);
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

    //{
    //    DISP("get :");
    //    DISP_VAL("channel_id  :", channel_id);
    //    DISP_VAL("remote_rank :", remote_rank);
    //    DISP_VAL("sequence    :", sequence);
    //}


    tbx_slist_ref_to_head(list);
    do{
        idx++;
        mad_iovec = tbx_slist_ref_get(list);
        if(!mad_iovec)
            break;

        //DISP("");
        //DISP_VAL("cur_mad_iovec->channel_id  :", mad_iovec->channel_id);
        //DISP_VAL("cur_mad_iovec->remote_rank :", mad_iovec->remote_rank);
        //DISP_VAL("cur_mad_iovec->sequence    :", mad_iovec->sequence);

        if(mad_iovec->channel_id == channel_id
           && mad_iovec->remote_rank == remote_rank
           && mad_iovec->sequence == sequence)
            return tbx_slist_index_extract(list, idx);
    }while(tbx_slist_ref_forward(list));

    //DISP("get : not found");

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

    //{
    //    DISP("get_smaller :");
    //    DISP_VAL("channel_id  :", channel_id);
    //    DISP_VAL("remote_rank :", remote_rank);
    //    DISP_VAL("sequence    :", sequence);
    //}


    tbx_slist_ref_to_head(list);
    do{
        idx++;
        mad_iovec = tbx_slist_ref_get(list);
        if(!mad_iovec)
            break;

        //DISP("");
        //DISP_VAL("cur_mad_iovec->channel_id  :", mad_iovec->channel_id);
        //DISP_VAL("cur_mad_iovec->remote_rank :", mad_iovec->remote_rank);
        //DISP_VAL("cur_mad_iovec->sequence    :", mad_iovec->sequence);



        if(mad_iovec->channel_id == channel_id
           && mad_iovec->remote_rank == remote_rank
           && mad_iovec->sequence == sequence
           && mad_iovec->length <= length)
            return tbx_slist_index_extract(list, idx);
    }while(tbx_slist_ref_forward(list));

    //DISP("get_smaller : not found");
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

        if(mad_iovec->channel_id == channel_id
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
static void
mad_iovec_treat_header_data(p_mad_adapter_t adapter,
                            rank_t remote_rank,
                            p_mad_iovec_header_t header){

    p_mad_iovec_t mad_iovec = NULL;
    p_tbx_slist_t r_msg_slist = NULL;

    channel_id_t msg_channel_id = 0;
    sequence_t msg_seq = 0;
    length_t msg_len = 0;

    void *       data = NULL;

    LOG_IN();
    r_msg_slist = adapter->driver->r_msg_slist;
    msg_channel_id = get_channel_id(header);
    msg_seq        = get_sequence(header);
    msg_len        = get_length(header);

    // recherche du petit associé
    mad_iovec = mad_iovec_get(r_msg_slist,
                              msg_channel_id,
                              remote_rank,
                              msg_seq);
    //*DISP_VAL("treat_header_data : EXTRACTION : adapter->driver->r_msg_slist - len", tbx_slist_get_length(adapter->driver->r_msg_slist));

    if(mad_iovec){
        p_mad_channel_t channel = NULL;

        //DISP("reception de données");

        // on enlève de la liste des unpacks en cours
        channel = mad_iovec->channel;
        mad_iovec_get(channel->unpacks_list, msg_channel_id, remote_rank, msg_seq);
        //*DISP_VAL("treat_header_data : EXTRACTION : channel->unpacks_list - len", tbx_slist_get_length(channel->unpacks_list));

        mad_iovec_free(mad_iovec, tbx_false);

        data = (void *)header;
        data += MAD_IOVEC_DATA_HEADER_SIZE;

        memcpy(mad_iovec->data[0].iov_base,
               data,
               mad_iovec->data[0].iov_len);
    } else {
        p_mad_iovec_t temp = NULL;
        //DISP("Msg reçu mais sans unpack associé");

        temp = mad_iovec_create(msg_channel_id, msg_seq);
        temp->length          = msg_len;
        temp->remote_rank     = remote_rank;

        temp->data[0].iov_len  = msg_len;
        temp->data[0].iov_base = TBX_MALLOC(msg_len);

        data = (void *)header;
        data += MAD_IOVEC_DATA_HEADER_SIZE;

        memcpy(temp->data[0].iov_base, data, msg_len);

        /* je mets dans la liste des unexpected le pointeur sur la zone à retaiter
           on change la zone de réception du mad_iovec du on_going si ce n'est pas déja fait */


        tbx_slist_append(adapter->waiting_list, temp);
        //*DISP_VAL("treat_header_data : AJOUT : adapter->waiting_list - len", tbx_slist_get_length(adapter->waiting_list));

    }
    LOG_OUT();
}

static void
mad_iovec_treat_ack(p_mad_adapter_t adapter,
                    rank_t remote_rank,
                    p_mad_iovec_header_t header){
    p_mad_iovec_t mad_iovec = NULL;
    p_mad_driver_interface_t interface = NULL;

    channel_id_t msg_channel_id = 0;
    sequence_t msg_seq = 0;

    LOG_IN();
    msg_channel_id = get_channel_id(header);
    msg_seq        = get_sequence(header);

    mad_iovec = mad_iovec_get(adapter->large_mad_iovec_list,
                              msg_channel_id,
                              remote_rank,
                              msg_seq);
    //*DISP_VAL("treat_header_ack : EXTRACTION : adapter->large_mad_iovec_list - len", tbx_slist_get_length(adapter->large_mad_iovec_list));

    if(!mad_iovec)
        FAILURE("mad_iovec with rdv not found");

    interface = adapter->driver->interface;

    //DISP("###########DEPOT D'UN GROS");
    tbx_slist_append(adapter->sending_list, mad_iovec);
    //*DISP_VAL("treat_header_ack : AJOUT : adapter->sending_list - len", tbx_slist_get_length(adapter->sending_list));
    LOG_OUT();
}

static void
mad_iovec_treat_not_ready(p_mad_adapter_t adapter,
                          rank_t remote_rank,
                          p_mad_iovec_header_t header){

    p_mad_iovec_t mad_iovec = NULL;
    channel_id_t msg_channel_id = 0;
    sequence_t msg_seq = 0;

    LOG_IN();
    msg_channel_id = get_channel_id(header);
    msg_seq        = get_sequence(header);

    mad_iovec = mad_iovec_get(adapter->large_mad_iovec_list,
                              msg_channel_id,
                              remote_rank,
                              msg_seq);
    //*DISP_VAL("treat_header_not_ready : EXTRACTION : adapter->large_mad_iovec_list - len", tbx_slist_get_length(adapter->large_mad_iovec_list));

    if(mad_iovec){
        // on remet le mad_iovec dans la liste des prets
        tbx_slist_append(adapter->driver->s_msg_slist, mad_iovec);
        //*DISP_VAL("treat_header_not_ready : AJOUT : adapter->driver->s_msg_slist - len", tbx_slist_get_length(adapter->driver->s_msg_slist));
    } else {
        FAILURE("mad_iovec_treat_not_ready : mad_iovec with rdv not found");
    }
    LOG_OUT();
}

static void
mad_iovec_treat_rdv(p_mad_adapter_t adapter,
                    rank_t destination,
                    p_mad_iovec_header_t header){
    p_mad_iovec_t  large_iov   = NULL;
    p_mad_iovec_t  mad_iovec    = NULL;
    rank_t         my_rank           = 0;

    channel_id_t   msg_channel_id = 0;
    sequence_t     msg_seq = 0;

    LOG_IN();
    msg_channel_id = get_channel_id(header);
    msg_seq        = get_sequence(header);

    my_rank = adapter->driver->madeleine->session->process_rank;

    // recherche du gros associé

    //{
    //   DISP("treat_rdv :");
    //   DISP_VAL("channel_id  :", msg_channel_id);
    //   DISP_VAL("remote_rank :", destination);
    //   DISP_VAL("sequence    :", msg_seq);
    //
    //   if(adapter->driver->r_msg_slist->length){
    //        tbx_slist_ref_to_head(adapter->driver->r_msg_slist);
    //        do{
    //            mad_iovec = tbx_slist_ref_get(adapter->driver->r_msg_slist);
    //            if(!mad_iovec)
    //                break;
    //
    //            DISP("");
    //            DISP_VAL("cur_mad_iovec->channel_id  :", mad_iovec->channel_id);
    //            DISP_VAL("cur_mad_iovec->remote_rank :", mad_iovec->remote_rank);
    //            DISP_VAL("cur_mad_iovec->sequence    :", mad_iovec->sequence);
    //
    //            if(mad_iovec->channel_id == msg_channel_id
    //               && mad_iovec->remote_rank == destination
    //               && mad_iovec->sequence == msg_seq){
    //                DISP("TROUVE");
    //                break;
    //            }
    //        }while(tbx_slist_ref_forward(adapter->driver->r_msg_slist));
    //    } else {
    //        DISP("adapter->driver->r_msg_slist->length = 0");
    //    }
    //}


     large_iov = mad_iovec_get(adapter->driver->r_msg_slist,
                               msg_channel_id,
                               destination,
                               msg_seq);

    if(large_iov){
        //DISP("POST D'UN GROS");
        tbx_slist_append(adapter->receiving_list, large_iov);
    }

    // on crée un nouvel iovec
    mad_iovec = mad_iovec_create(msg_channel_id,
                                 0); // sequence
    mad_iovec->remote_rank = destination;
    mad_iovec_begin_struct_iovec(mad_iovec, my_rank);

    if(large_iov){
        //DISP("DEPOT D'UN ACK");
        mad_iovec_add_ack(mad_iovec, 1, msg_channel_id, msg_seq);
        mad_iovec->channel = large_iov->channel;
    } else {
        p_mad_madeleine_t madeleine = NULL;
        p_mad_channel_t * channel_tab = NULL;
        p_mad_channel_t channel = NULL;

        //DISP("DEPOT D'UN NOT READY");
        madeleine = mad_get_madeleine();
        channel_tab = madeleine->channel_tab;
        channel = channel_tab[mad_iovec->channel_id];
        if(!channel){
            FAILURE("mad_iovec_treat_rdv : channel not found");
        }

        mad_iovec_add_not_ready(mad_iovec, 1, msg_channel_id,
                                msg_seq);
        mad_iovec->channel = channel;
    }
    mad_iovec->total_nb_seg = 2;
    mad_iovec->area_nb_seg[0] = 2;

    tbx_slist_append(adapter->sending_list, mad_iovec);
    //*DISP_VAL("treat_header_rdv : AJOUT : adapter->sending_list - len", tbx_slist_get_length(adapter->sending_list));

    LOG_OUT();
    //DISP("---------------------------");
}

void
mad_iovec_exploit_og(p_mad_adapter_t a,
                      p_mad_on_going_t og){
    p_mad_iovec_t mad_iovec      = NULL;

    LOG_IN();
    mad_iovec        = og->mad_iovec;
    mad_iovec_exploit_msg(a, mad_iovec->data[0].iov_base);
    LOG_OUT();
}


static void
mad_iovec_print_msg(void * msg){
    p_mad_iovec_header_t header        = NULL;
    type_t type = 0;
    nb_seg_t nb_seg = 0;

    LOG_IN();
    header = (p_mad_iovec_header_t)msg;
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

        case MAD_IOVEC_NOT_READY:
            DISP("RECEPTION : NOT_READY");
            DISP_VAL("-> type ", get_type(header));
            DISP_VAL("-> ch id", get_channel_id(header));
            DISP_VAL("-> seq  ", get_sequence(header));

            header += MAD_IOVEC_NOT_READY_NB_ELM;
            break;

        default:
            FAILURE("invalid mad_iovec header");
        }
    }
    LOG_OUT();
}

void
mad_iovec_print(p_mad_iovec_t mad_iovec){
    unsigned int i = 0;
    p_mad_iovec_header_t header        = NULL;
    type_t type = 0;
    nb_seg_t nb_seg = 0;

    LOG_IN();
    header = (p_mad_iovec_header_t)mad_iovec->data[i++].iov_base;
    DISP_VAL("global header -> type  ", get_type(header));
    DISP_VAL("              -> rank  ", get_rank(header));
    DISP_VAL("              -> nb_seg", get_nb_seg(header));

    type = get_type(header);
    assert (type == MAD_IOVEC_GLOBAL_HEADER);

    nb_seg = get_nb_seg(header);
    assert (nb_seg > 0);


    header = mad_iovec->data[i].iov_base;

    while(--nb_seg){
        type = get_type(header);
        switch(type){

        case MAD_IOVEC_DATA_HEADER :
            DISP("RECEPTION : DATA_HEADER");
            DISP_VAL("-> type ", get_type(header));
            DISP_VAL("-> ch id", get_channel_id(header));
            DISP_VAL("-> seq  ", get_sequence(header));
            DISP_VAL("-> len  ", get_length(header));

            DISP("RECEPTION : DATA");
            DISP_STR("data", mad_iovec->data[i+1].iov_base);

            i += 2;
            header = mad_iovec->data[i].iov_base;

            assert (nb_seg > 1);
            nb_seg--;
            break;

        case MAD_IOVEC_RDV:
            DISP("RECEPTION : RDV");
            DISP_VAL("-> type ", get_type(header));
            DISP_VAL("-> ch id", get_channel_id(header));
            DISP_VAL("-> seq  ", get_sequence(header));

            header = mad_iovec->data[++i].iov_base;
            break;

       case MAD_IOVEC_ACK:
           DISP("RECEPTION : ACK");
           DISP_VAL("-> type ", get_type(header));
           DISP_VAL("-> ch id", get_channel_id(header));
           DISP_VAL("-> seq  ", get_sequence(header));

           header = mad_iovec->data[++i].iov_base;
           break;

        case MAD_IOVEC_NOT_READY:
            DISP("RECEPTION : NOT_READY");
            DISP_VAL("-> type ", get_type(header));
            DISP_VAL("-> ch id", get_channel_id(header));
            DISP_VAL("-> seq  ", get_sequence(header));

            header = mad_iovec->data[++i].iov_base;
            break;

        default:
            FAILURE("invalid mad_iovec header");
        }
    }
    LOG_OUT();
}

void
mad_iovec_s_check(p_tbx_slist_t list,
                  rank_t remote_rank,
                  size_t length,
                  p_mad_iovec_t mad_iovec){
    unsigned int i = 0;
    p_mad_iovec_header_t header        = NULL;
    type_t type = 0;
    nb_seg_t nb_seg = 0;

    channel_id_t channel_id = 0;
    sequence_t   sequence = 0;

    p_mad_iovec_t tmp = NULL;
    LOG_IN();
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
            sequence = get_sequence(header);

            tmp = mad_iovec_get_smaller(list,
                                        channel_id,
                                        remote_rank,
                                         sequence,
                                        length);

            mad_iovec_free(tmp, tbx_false);

            i += 2;
            header = mad_iovec->data[i].iov_base;

            assert (nb_seg > 1);
            nb_seg--;
            break;

        case MAD_IOVEC_RDV:
        case MAD_IOVEC_ACK:
        case MAD_IOVEC_NOT_READY:
            header = mad_iovec->data[++i].iov_base;
            break;

        default:
            FAILURE("invalid mad_iovec header");
        }
    }
    LOG_OUT();
}




void
mad_iovec_exploit_msg(p_mad_adapter_t a,
                      void * msg){
    p_mad_iovec_header_t header        = NULL;
    type_t type = 0;
    rank_t  source         = 0;
    nb_seg_t nb_seg = 0;

    LOG_IN();
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
            //DISP("RECEPTION : HEADER_DATA");
            mad_iovec_treat_header_data(a, source, header);

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
           //DISP("RECEPTION : RDV");
           mad_iovec_treat_rdv(a, source, header);
           header += MAD_IOVEC_RDV_NB_ELM;
           break;

       case MAD_IOVEC_ACK:
           //DISP("RECEPTION : ACK");
           mad_iovec_treat_ack(a, source, header);
           header += MAD_IOVEC_ACK_NB_ELM;
           break;

        case MAD_IOVEC_NOT_READY:
            //DISP("RECEPTION : NOT_READY");
            mad_iovec_treat_not_ready(a, source, header);
            header += MAD_IOVEC_NOT_READY_NB_ELM;
            break;

        default:
            FAILURE("invalid mad_iovec header");
        }
    }
    LOG_OUT();
}
