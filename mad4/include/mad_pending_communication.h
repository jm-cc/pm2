#ifndef MAD_PENDING_COMMUNICATION_H
#define MAD_PENDING_COMMUNICATION_H

typedef struct s_mad_track_t *p_mad_track_t ;

//typedef struct s_mad_track_t{
//    uint32_t       id;
//    tbx_bool_t     pre_posted;
//
//    p_mad_track_t  remote_tracks[32];
//
//    p_mad_driver_specific_t specific;
//}mad_track_t;

//typedef struct s_mad_track_set_t{
//    // tableau d'association methode d'envoi/piste
//    // clé : cpy/rdv/rdma -> tableau de track
//    p_tbx_htable_t tracks_htable;
//    p_mad_track_t *tracks_tab;
//
//    uint32_t       nb_track;
//
//    p_tbx_slist_t  pipeline; // liste de mad_iovec
//    p_mad_iovec_t *pipeline;
//    uint32_t       pipeline_length;
//    uint32_t       pipeline_cur_nb_elm;
//    tbx_bool_t     started;
//
//    p_mad_driver_specific_t specific;
//}mad_track_set_t, *p_mad_track_set_t;

/*******************************************/

///typedef struct s_mad_s_track_t{
///    uint32_t       id;
///    tbx_bool_t     pre_posted;
///
///    p_mad_track_t  remote_tracks[32];
///
///    p_mad_driver_specific_t specific;
///}mad_s_track_t;


///typedef struct s_mad_s_track_set_t{
///    // tableau d'association methode d'envoi/piste
///    // clé : cpy/rdv/rdma -> tableau de track
///    p_tbx_htable_t tracks_htable;
///    p_mad_track_t *tracks_tab;
///
///    uint32_t       nb_track;
///
///    p_mad_iovec_t *pipeline;
///    uint32_t       pipeline_length;
///    uint32_t       pipeline_cur_nb_elm;
///    tbx_bool_t     started;
///
///    p_mad_driver_specific_t specific;
///}mad_s_track_set_t, *p_mad_s_track_set_t;


/*********************************/
typedef struct s_mad_track_t{
    uint32_t       id;
    //char *methode_transfert = "cpy" | "rdv" | "rdma"

    tbx_bool_t     pre_posted;

    p_mad_pipeline_t pipeline;

    p_mad_track_t  remote_tracks[32];

    p_mad_driver_specific_t specific;
}mad_track_t;

typedef struct s_mad_track_set_t{
    p_tbx_htable_t tracks_htable;
    p_mad_track_t *tracks_tab;

    uint32_t       nb_track;

    uint32_t max_nb_pending_iov; //-> 1 en emission,
                                 //   nb_track en reception
    uint32_t cur_nb_pending_iov;

    p_mad_pipeline_t in_more;

    p_mad_driver_specific_t specific;
}mad_track_set_t, *p_mad_track_set_t;



/******************************/

typedef struct s_mad_iovec{
    ntbx_process_lrank_t    my_rank;
    ntbx_process_lrank_t    remote_rank;

    p_mad_channel_t         channel; // utile en réception
    sequence_t              sequence;

    size_t                  length;
    unsigned int            total_nb_seg;

    unsigned int            area_nb_seg[32];
    struct iovec            data       [32];

    mad_send_mode_t         send_mode;
    mad_receive_mode_t      receive_mode;

    mad_send_mode_t         next_send_mode;
    mad_receive_mode_t      next_receive_mode;

    //char *methode_transfert

    p_mad_track_t track;

    p_mad_driver_specific_t specific;
} mad_iovec_t, *p_mad_iovec_t;

#endif // MAD_PENDING_COMMUNICATION_H
