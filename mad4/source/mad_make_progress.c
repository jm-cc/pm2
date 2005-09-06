#include <stdlib.h>
#include <stdio.h>
#include "madeleine.h"

mad_mkp_status_t
mad_make_progress(p_mad_adapter_t adapter,
                  p_mad_track_t track){
    p_mad_driver_interface_t interface = NULL;
    p_mad_pipeline_t pipeline = NULL;
    p_mad_iovec_t mad_iovec = NULL;
    tbx_bool_t progress = tbx_false;

    mad_mkp_status_t status = MAD_MKP_NO_PROGRESS;
    LOG_IN();

    //DISP("-->mkp");

    interface = adapter->driver->interface;

    pipeline = track->pipeline;

    if(!pipeline->cur_nb_elm){
        status = MAD_MKP_NOTHING_TO_DO;
        goto end;
    }

    // A REFAIRE de begin à end
    mad_iovec = mad_pipeline_get(pipeline);

    progress = interface->test(mad_iovec->track);
    if(progress){
        status = MAD_MKP_PROGRESS;
    } else {
        status = MAD_MKP_NO_PROGRESS;
    }

 end:
    LOG_OUT();
    //DISP("<--mkp");
    return status;
}



