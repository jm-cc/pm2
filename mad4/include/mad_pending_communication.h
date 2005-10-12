#ifndef MAD_PENDING_COMMUNICATION_H
#define MAD_PENDING_COMMUNICATION_H

#define NB_ENTRIES 32
#define NB_DEST    32

typedef struct s_mad_track_t *p_mad_track_t ;


typedef struct s_mad_iovec{
    ntbx_process_lrank_t    my_rank;
    ntbx_process_lrank_t    remote_rank;

    p_mad_channel_t         channel; // pour retrouver la
                                     // liste des unpack

    sequence_t              sequence;

    size_t                  length;
    unsigned int            nb_packs; // nb d'entrées relatives
                                      // à des données
    unsigned int            total_nb_seg; // données + contrôles

    tbx_bool_t              need_rdv;

    mad_send_mode_t         send_mode;
    mad_receive_mode_t      receive_mode;
    int                     matrix_entrie;

    p_mad_track_t           track;

    struct iovec            data [NB_ENTRIES];

    p_mad_driver_specific_t specific;
} mad_iovec_t, *p_mad_iovec_t;



typedef struct s_mad_track_t{
    uint32_t         id;
    tbx_bool_t       pre_posted;
    p_mad_track_t    remote_tracks[NB_DEST];
    p_mad_driver_specific_t specific;
}mad_track_t;

typedef struct s_mad_track_set_t{
    p_tbx_htable_t tracks_htable;
    p_mad_track_t *tracks_tab;
    uint32_t       nb_track;

    int status; // nothing, progress ou no_progress

    // Pour l'émission
    p_mad_iovec_t cur;
    p_mad_iovec_t next;

    // Pour la réception
    p_mad_iovec_t *reception_curs; //taille = nb_tracks = 2
    uint32_t       nb_pending; // nb de pistes en
                               // attente de réception

    p_mad_track_t  cpy_track;
    p_mad_track_t  rdv_track;

    p_mad_driver_specific_t specific;
}mad_track_set_t, *p_mad_track_set_t;

#endif // MAD_PENDING_COMMUNICATION_H
