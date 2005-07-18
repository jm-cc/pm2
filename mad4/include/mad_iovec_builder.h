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
    MAD_IOVEC_NOT_READY_NB_ELM     = 1
} mad_iovec_control_nb_elm_t, *p_mad_iovec_control_nb_elm_t;

typedef enum e_mad_iovec_control_size {
    MAD_IOVEC_GLOBAL_HEADER_SIZE = MAD_IOVEC_GLOBAL_HEADER_NB_ELM * sizeof(mad_iovec_header_t),
    MAD_IOVEC_DATA_HEADER_SIZE   = MAD_IOVEC_DATA_HEADER_NB_ELM   * sizeof(mad_iovec_header_t),
    MAD_IOVEC_RDV_SIZE           = MAD_IOVEC_RDV_NB_ELM           * sizeof(mad_iovec_header_t),
    MAD_IOVEC_ACK_SIZE           = MAD_IOVEC_ACK_NB_ELM           * sizeof(mad_iovec_header_t),
    MAD_IOVEC_NOT_READY_SIZE     = MAD_IOVEC_NOT_READY_NB_ELM     * sizeof(mad_iovec_header_t)
} mad_iovec_control_size_t, *p_mad_iovec_control_size_t;

p_mad_iovec_t
mad_iovec_create(unsigned int   channel_id,
                 unsigned int   seq);

void
mad_iovec_free(p_mad_iovec_t, tbx_bool_t);

void
mad_iovec_exit(p_mad_iovec_t);

void
mad_iovec_init_allocators(void);

void
mad_iovec_allocators_exit();

unsigned int
mad_iovec_begin_struct_iovec(p_mad_iovec_t mad_iovec,
                             rank_t my_rank);
unsigned int
mad_iovec_add_header_data(p_mad_iovec_t mad_iovec,
                          unsigned int index,
                          channel_id_t msg_channel_id,
                          sequence_t msg_seq_nb,
                          length_t msg_len);

unsigned int
mad_iovec_add_rdv(p_mad_iovec_t mad_iovec,
                  unsigned int index,
                  channel_id_t msg_channel_id,
                  sequence_t msg_seq_nb);


unsigned int
mad_iovec_add_ack(p_mad_iovec_t mad_iovec,
                  unsigned int index,
                  channel_id_t msg_channel_id,
                  sequence_t msg_seq_nb);


unsigned int
mad_iovec_add_not_ready(p_mad_iovec_t mad_iovec,
                        unsigned int index,
                        channel_id_t msg_channel_id,
                        sequence_t msg_seq_nb);


void
mad_iovec_add_data(p_mad_iovec_t mad_iovec,
                   void *data,
                   size_t length,
                   unsigned int index);

void
mad_iovec_add_data2(p_mad_iovec_t mad_iovec,
                    void *data,
                    size_t length,
                    unsigned int index);

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

void
mad_iovec_exploit_og(p_mad_adapter_t a,
                     p_mad_on_going_t og);


void
mad_iovec_exploit_msg(p_mad_adapter_t a,
                      void * msg);

void
mad_iovec_print(p_mad_iovec_t mad_iovec);

void
mad_iovec_s_check(p_tbx_slist_t list,
                  rank_t remote_rank,
                  size_t length,
                  p_mad_iovec_t mad_iovec);
#endif /* MAD_IOVEC_BUILDER_H */
