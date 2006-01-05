#include <stdlib.h>
#include <stdio.h>
#include "madeleine.h"

p_mad_iovec_t
r_mad_make_progress(p_mad_adapter_t adapter,
                    p_mad_track_t track){
    p_mad_driver_interface_t interface = NULL;
    p_mad_iovec_t mad_iovec = NULL;
    LOG_IN();
    interface = adapter->driver->interface;

    mad_iovec = interface->r_test(track);

    LOG_OUT();
    return mad_iovec;
}



mad_mkp_status_t
s_mad_make_progress(p_mad_adapter_t adapter){
    p_mad_driver_interface_t interface = NULL;
    tbx_bool_t               progress = tbx_false;
    mad_mkp_status_t         status = MAD_MKP_NO_PROGRESS;

    p_mad_track_set_t track_set = NULL;

    LOG_IN();
    interface = adapter->driver->interface;

    track_set = adapter->s_track_set;

    //progress = interface->s_test(track);
    progress = interface->s_test(track_set);

    if(progress){
        status = MAD_MKP_PROGRESS;
    } else {
        status = MAD_MKP_NO_PROGRESS;
    }
    LOG_OUT();
    return status;
}
