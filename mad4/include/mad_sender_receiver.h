#ifndef MAD_SENDER_RECEIVER_H
#define MAD_SENDER_RECEIVER_H


//typedef struct s_mad_iovec {
//    ntbx_process_lrank_t    my_rank;
//    ntbx_process_lrank_t    destination_rank;
//
//    unsigned int            user_channel_id;
//    unsigned int            seq_nb;
//
//    tbx_bool_t              on_going_emission;
//
//    size_t                  length;
//    unsigned int            nb_iovec_entries;
//
//    p_tbx_slist_t           emission_reception_flag_list;
//
//    p_mad_driver_specific_t specific;
//
//    unsigned int            areas[32];
//    struct iovec            data [32];
//} mad_iovec_t, *p_mad_iovec_t;


//typedef struct s_mad_pending_communication{
//    unsigned int  status;
//    unsigned int  area_id;
//
//    p_mad_iovec_t mad_iovec;
//
//    p_mad_driver_specific_t specific;
//}mad_pending_communication_t, *p_mad_pending_communication_t;


typedef enum e_mad_mx_mkp_status {
    MAD_MKP_NOTHING_TO_DO,
    MAD_MKP_PROGRESSION,
    MAD_MKP_NO_PROGRESSION
} mad_mx_mkp_status_t, *p_mad_mx_mkp_status_t;

void
mad_s_make_progress(p_mad_driver_t, p_mad_adapter_t);


void
mad_r_make_progress(p_mad_driver_t, p_mad_adapter_t, p_mad_channel_t);


//void
//mad_exploit_msg(p_mad_driver_t driver,
//                p_mad_on_going_t og);

#endif // MAD_SENDER_RECEIVER_H
