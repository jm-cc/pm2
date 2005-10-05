#ifndef MAD_SENDER_RECEIVER_H
#define MAD_SENDER_RECEIVER_H

//typedef enum e_mad_mx_mkp_status {
//    MAD_MKP_NOTHING_TO_DO,
//    MAD_MKP_PROGRESSION,
//    MAD_MKP_NO_PROGRESSION
//} mad_mx_mkp_status_t, *p_mad_mx_mkp_status_t;



void
mad_s_make_progress(p_mad_adapter_t);

void
mad_r_make_progress(p_mad_adapter_t);


void submit_send(p_mad_adapter_t,
                 p_mad_track_t,
                 p_mad_iovec_t);
//void submit_wait_send(p_mad_pipeline_t, p_mad_iovec_t);
//void submit_rdma_send(p_mad_pipeline_t, p_mad_iovec_t);

//void submit_recv     (p_mad_pipeline_t, p_mad_iovec_t);
//void submit_wait_recv(p_mad_pipeline_t, p_mad_iovec_t);
//void submit_rdma_recv(p_mad_pipeline_t, p_mad_iovec_t);

#endif // MAD_SENDER_RECEIVER_H
