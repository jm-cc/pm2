#ifndef MAD_IOVEC_BUILDER_H
#define MAD_IOVEC_BUILDER_H

/** Initialisation **/
void
mad_iovec_allocator_init(void);

void
mad_iovec_allocator_exit(void);


/** Création/Libération de structure mad_iovec **/
p_mad_iovec_t
mad_iovec_create(ntbx_process_lrank_t remote_rank,
                 p_mad_channel_t  channel,
                 unsigned int     sequence,
                 tbx_bool_t       need_rdv,
                 mad_send_mode_t    send_mode,
                 mad_receive_mode_t receive_mode);

void
mad_iovec_free(p_mad_iovec_t);


/** Construction de la partie data du mad_iovec **/
void
mad_iovec_begin_struct_iovec(p_mad_iovec_t mad_iovec);

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

void
mad_iovec_add_data(p_mad_iovec_t mad_iovec,
                   void *data,
                   size_t length);

void
mad_iovec_add_data_at_index(p_mad_iovec_t mad_iovec,
                            void *data,
                            size_t length,
                            int index);

void
mad_iovec_add_blocked(p_mad_iovec_t mad_iovec,
                      channel_id_t msg_channel_id);

void
mad_iovec_add_unblocked(p_mad_iovec_t mad_iovec,
                        channel_id_t msg_channel_id);

void
mad_iovec_update_global_header(p_mad_iovec_t);


/** Fonctions de recherche de mad_iovec dans une tbx_slist_t **/
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


/** Exploitation d'un message envoyé **/
void
mad_iovec_s_check(p_mad_adapter_t adapter,
                  p_mad_iovec_t mad_iovec);


/** Exploitation d'un message inattendu **/
tbx_bool_t
mad_iovec_exploit_msg(p_mad_adapter_t a,
                      void * msg);


/** **/
void
mad_iovec_search_rdv(p_mad_adapter_t adapter,
                     p_mad_track_t track);


/** Fonctions d'affichage **/
void
mad_iovec_print(p_mad_iovec_t mad_iovec);

#endif /* MAD_IOVEC_BUILDER_H */
