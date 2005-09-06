#ifndef MAD_MAKE_PROGRESS_H
#define MAD_MAKE_PROGRESS_H

typedef enum e_mad_mkp_status {
    MAD_MKP_NOTHING_TO_DO,
    MAD_MKP_PROGRESS,
    MAD_MKP_NO_PROGRESS
} mad_mkp_status_t, *p_mad_mkp_status_t;


mad_mkp_status_t
mad_make_progress(p_mad_adapter_t,
                  p_mad_track_t);

#endif // MAD_MAKE_PROGRESS_H
