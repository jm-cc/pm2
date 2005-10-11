#ifndef MAD_IOVEC_BUILDER_H
#define MAD_IOVEC_BUILDER_H

typedef struct s_mad_iovec_header{
    uint32_t tab[1];
}mad_iovec_header_t, *p_mad_iovec_header_t;


typedef enum e_mad_iovec_control_nb_elm {
    MAD_IOVEC_GLOBAL_HEADER_NB_ELM = 1,
    MAD_IOVEC_DATA_HEADER_NB_ELM   = 2,
    MAD_IOVEC_RDV_NB_ELM           = 1,
    MAD_IOVEC_ACK_NB_ELM           = 1,
    //MAD_IOVEC_DATA_TREATED_NB_ELM    = 1,
    //MAD_IOVEC_CONTROL_TREATED_NB_ELM = 1,
    MAD_IOVEC_BLOCKED_NB_ELM       = 1,
    MAD_IOVEC_UNBLOCKED_NB_ELM     = 1
} mad_iovec_control_nb_elm_t, *p_mad_iovec_control_nb_elm_t;

typedef enum e_mad_iovec_control_size {
    MAD_IOVEC_GLOBAL_HEADER_SIZE = MAD_IOVEC_GLOBAL_HEADER_NB_ELM * sizeof(mad_iovec_header_t),
    MAD_IOVEC_DATA_HEADER_SIZE   = MAD_IOVEC_DATA_HEADER_NB_ELM   * sizeof(mad_iovec_header_t),
    MAD_IOVEC_RDV_SIZE           = MAD_IOVEC_RDV_NB_ELM           * sizeof(mad_iovec_header_t),
    MAD_IOVEC_ACK_SIZE           = MAD_IOVEC_ACK_NB_ELM           * sizeof(mad_iovec_header_t),
    MAD_IOVEC_BLOCKED_SIZE       = MAD_IOVEC_BLOCKED_NB_ELM       * sizeof(mad_iovec_header_t),
    MAD_IOVEC_UNBLOCKED_SIZE     = MAD_IOVEC_UNBLOCKED_NB_ELM     * sizeof(mad_iovec_header_t)
} mad_iovec_control_size_t, *p_mad_iovec_control_size_t;

//p_mad_iovec_t
//mad_iovec_create(unsigned int   seq);

p_mad_iovec_t
mad_iovec_create(ntbx_process_lrank_t remote_rank,
                 p_mad_channel_t  channel,
                 unsigned int     sequence,
                 tbx_bool_t       need_rdv,
                 mad_send_mode_t    send_mode,
                 mad_receive_mode_t receive_mode);

void
mad_iovec_free(p_mad_iovec_t, tbx_bool_t);

void
mad_iovec_exit(p_mad_iovec_t);

void
mad_iovec_init_allocators(void);

void
mad_iovec_allocators_exit();

//unsigned int
//mad_iovec_begin_struct_iovec(p_mad_iovec_t mad_iovec,
//                             rank_t my_rank);
void
mad_iovec_begin_struct_iovec(p_mad_iovec_t mad_iovec);

//unsigned int
//mad_iovec_add_data_header(p_mad_iovec_t mad_iovec,
//                          unsigned int index,
//                          channel_id_t msg_channel_id,
//                          sequence_t msg_seq_nb,
//                          length_t msg_len);
//
//unsigned int
//mad_iovec_add_rdv(p_mad_iovec_t mad_iovec,
//                  unsigned int index,
//                  channel_id_t msg_channel_id,
//                  sequence_t msg_seq_nb);
//
//
//unsigned int
//mad_iovec_add_ack(p_mad_iovec_t mad_iovec,
//                  unsigned int index,
//                  channel_id_t msg_channel_id,
//                  sequence_t msg_seq_nb);


void
mad_iovec_add_data_header(p_mad_iovec_t mad_iovec,
                          channel_id_t msg_channel_id,
                          sequence_t msg_seq_nb,
                          length_t msg_len);

void
mad_iovec_add_rdv(p_mad_iovec_t mad_iovec,
                  channel_id_t msg_channel_id,
                  sequence_t msg_seq_nb);


void
mad_iovec_add_ack(p_mad_iovec_t mad_iovec,
                  channel_id_t msg_channel_id,
                  sequence_t msg_seq_nb);


//void
//mad_iovec_add_data(p_mad_iovec_t mad_iovec,
//                   void *data,
//                   size_t length,
//                   unsigned int index);
//
//void
//mad_iovec_add_data2(p_mad_iovec_t mad_iovec,
//                    void *data,
//                    size_t length,
//                    unsigned int index);

void
mad_iovec_add_data(p_mad_iovec_t mad_iovec,
                   void *data,
                   size_t length);

void
mad_iovec_add_data2(p_mad_iovec_t mad_iovec,
                    void *data,
                    size_t length,
                    int index);



p_mad_iovec_t
mad_iovec_get(p_tbx_slist_t list,
              channel_id_t  channel_id,
              rank_t remote_rank,
              sequence_t  sequence);


p_mad_iovec_t
mad_iovec_get_smaller(p_tbx_slist_t list,
                      channel_id_t  channel_id,
                      rank_t remote_rank,
                      sequence_t  sequence,
                      size_t length);

p_mad_iovec_t
mad_iovec_search(p_tbx_slist_t list,
                 channel_id_t  channel_id,
                 rank_t remote_rank,
                 sequence_t  sequence);

tbx_bool_t
mad_iovec_exploit_msg(p_mad_adapter_t a,
                      void * msg);

void
mad_iovec_print(p_mad_iovec_t mad_iovec);

void
mad_iovec_print_msg(void * msg);


void
mad_iovec_print_iovec(struct iovec *iovec);

void
mad_iovec_s_check(p_mad_adapter_t adapter,
                  p_mad_iovec_t mad_iovec);

//tbx_bool_t
//mad_iovec_check_unexpected_rdv(p_mad_adapter_t adapter,
//                               p_mad_iovec_t mad_iovec);

void
mad_iovec_search_rdv(p_mad_adapter_t adapter,
                     p_mad_track_t track);



void
mad_iovec_update_global_header(p_mad_iovec_t);


#endif /* MAD_IOVEC_BUILDER_H */
