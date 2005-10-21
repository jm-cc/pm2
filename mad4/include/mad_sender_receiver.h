#ifndef MAD_SENDER_RECEIVER_H
#define MAD_SENDER_RECEIVER_H

void
mad_s_make_progress(p_mad_adapter_t);

void
mad_r_make_progress(p_mad_adapter_t);


void submit_send(p_mad_adapter_t,
                 p_mad_track_t,
                 p_mad_iovec_t);

#endif // MAD_SENDER_RECEIVER_H
