#ifndef MAD_PENDING_COMMUNICATION_H
#define MAD_PENDING_COMMUNICATION_H

typedef struct s_mad_iovec {
    ntbx_process_lrank_t    my_rank; // rank_t
    ntbx_process_lrank_t    remote_rank; //rank_t

    p_mad_channel_t         channel;
    channel_id_t            channel_id;
    sequence_t              sequence;

    size_t                  length;
    unsigned int            total_nb_seg;

    unsigned int            area_nb_seg[32];
    struct iovec            data       [32];

    mad_send_mode_t         send_mode;
    mad_receive_mode_t      receive_mode;

    mad_send_mode_t         next_send_mode;
    mad_receive_mode_t      next_receive_mode;
} mad_iovec_t, *p_mad_iovec_t;

typedef struct s_mad_on_going{
    unsigned int  status;
    unsigned int  area_id;
    p_mad_link_t  lnk;
    p_mad_iovec_t mad_iovec;
    p_mad_driver_specific_t specific;
}mad_on_going_t, *p_mad_on_going_t;

typedef struct s_mad_pipeline_t{
    p_mad_on_going_t *on_goings;
    unsigned int      nb_elm;
    int               index_first;
    int               index_last;
}mad_pipeline_t, *p_mad_pipeline_t;

typedef struct s_mad_track_t{
    p_mad_pipeline_t *pipelines;
    unsigned int      nb_elm;
}mad_track_t, *p_mad_track_t;

#endif // MAD_PENDING_COMMUNICATION_H
