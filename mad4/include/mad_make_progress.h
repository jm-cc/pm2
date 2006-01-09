#ifndef MAD_MAKE_PROGRESS_H
#define MAD_MAKE_PROGRESS_H

typedef enum e_mad_mkp_status {
    MAD_MKP_NOTHING_TO_DO,
    MAD_MKP_PROGRESS,
    MAD_MKP_NO_PROGRESS
} mad_mkp_status_t, *p_mad_mkp_status_t;


//mad_mkp_status_t
//mad_make_progress(p_mad_adapter_t,
//                  p_mad_track_t,
//                  tbx_bool_t);

//mad_mkp_status_t
//s_mad_make_progress(p_mad_adapter_t,
//                    p_mad_track_t);

mad_mkp_status_t
s_mad_make_progress(p_mad_adapter_t);

p_mad_iovec_t
r_mad_make_progress(p_mad_adapter_t adapter,
                    p_mad_track_t track);

#endif // MAD_MAKE_PROGRESS_H
