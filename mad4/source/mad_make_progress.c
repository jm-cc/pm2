#include <stdlib.h>
#include <stdio.h>
#include "madeleine.h"

mad_mkp_status_t
mad_make_progress(p_mad_adapter_t adapter,
                  p_mad_track_t track){
    p_mad_driver_interface_t interface = NULL;
    tbx_bool_t progress = tbx_false;
    mad_mkp_status_t status = MAD_MKP_NO_PROGRESS;
    LOG_IN();
    interface = adapter->driver->interface;

    progress = interface->test(track);
    if(progress){
        status = MAD_MKP_PROGRESS;
    } else {
        status = MAD_MKP_NO_PROGRESS;
    }
    LOG_OUT();
    return status;
}
