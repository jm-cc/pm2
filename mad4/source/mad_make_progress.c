#include <stdlib.h>
#include <stdio.h>
#include "madeleine.h"

int nb_chronos_mkp = 0;
double chrono_mad_mkp = 0.0;

//double chrono_mkfp_1_2 = 0.0;
//double chrono_mkfp_2_3 = 0.0;
//double chrono_mkfp_3_4 = 0.0;
//double chrono_mkfp_4_5 = 0.0;
//double chrono_mkfp_1_5 = 0.0;
//
//double min_chrono_mkfp_1_2 = 1000.0;
//double min_chrono_mkfp_2_3 = 1000.0;
//double min_chrono_mkfp_3_4 = 1000.0;
//double min_chrono_mkfp_4_5 = 1000.0;
//double min_chrono_mkfp_1_5 = 1000.0;
//
//double max_chrono_mkfp_1_2 = 0.0;
//double max_chrono_mkfp_2_3 = 0.0;
//double max_chrono_mkfp_3_4 = 0.0;
//double max_chrono_mkfp_4_5 = 0.0;
//double max_chrono_mkfp_1_5 = 0.0;
//
//static double _chrono_mkfp_1_2 = 0.0;
//static double _chrono_mkfp_2_3 = 0.0;
//static double _chrono_mkfp_3_4 = 0.0;
//static double _chrono_mkfp_4_5 = 0.0;
//static double _chrono_mkfp_1_5 = 0.0;

mad_mkp_status_t
mad_make_progress(p_mad_adapter_t adapter,
                  p_mad_track_t track){
    p_mad_driver_interface_t interface = NULL;
    p_mad_pipeline_t pipeline = NULL;
    p_mad_iovec_t mad_iovec = NULL;
    tbx_bool_t progress = tbx_false;


    tbx_tick_t        tick_debut;
    tbx_tick_t        t2;
    tbx_tick_t        t3;
    tbx_tick_t        t4;
    tbx_tick_t        tick_fin;

    mad_mkp_status_t status = MAD_MKP_NO_PROGRESS;
    LOG_IN();
    nb_chronos_mkp++;
    TBX_GET_TICK(tick_debut);

    interface = adapter->driver->interface;
    pipeline = track->pipeline;
    if(!pipeline->cur_nb_elm){
        status = MAD_MKP_NOTHING_TO_DO;
        goto end;
    }

    // A REFAIRE de begin à end
    TBX_GET_TICK(t2);
    mad_iovec = mad_pipeline_get(pipeline);
    TBX_GET_TICK(t3);

    progress = interface->test(mad_iovec->track);
    TBX_GET_TICK(t4);
    if(progress){
        status = MAD_MKP_PROGRESS;
    } else {
        status = MAD_MKP_NO_PROGRESS;
    }

    TBX_GET_TICK(tick_fin);
    chrono_mad_mkp += TBX_TIMING_DELAY(tick_debut, tick_fin);

    //_chrono_mkfp_1_2 = TBX_TIMING_DELAY(tick_debut, t2);
    //_chrono_mkfp_2_3 = TBX_TIMING_DELAY(t2, t3);
    //_chrono_mkfp_3_4 = TBX_TIMING_DELAY(t3, t4);
    //_chrono_mkfp_4_5 = TBX_TIMING_DELAY(t4, tick_fin);
    //_chrono_mkfp_1_5 = TBX_TIMING_DELAY(tick_debut, tick_fin);
    //
    //
    //chrono_mkfp_1_2    += _chrono_mkfp_1_2;
    //chrono_mkfp_2_3    += _chrono_mkfp_2_3;
    //chrono_mkfp_3_4    += _chrono_mkfp_3_4;
    //chrono_mkfp_4_5    += _chrono_mkfp_4_5;
    //chrono_mkfp_1_5    += _chrono_mkfp_1_5;
    //
    //min_chrono_mkfp_1_2    = (min_chrono_mkfp_1_2 <= _chrono_mkfp_1_2) ?min_chrono_mkfp_1_2    : _chrono_mkfp_1_2;
    //min_chrono_mkfp_2_3    = (min_chrono_mkfp_2_3 <= _chrono_mkfp_2_3) ?min_chrono_mkfp_2_3    : _chrono_mkfp_2_3;
    //min_chrono_mkfp_3_4    = (min_chrono_mkfp_3_4 <= _chrono_mkfp_3_4) ?min_chrono_mkfp_3_4    : _chrono_mkfp_3_4;
    //min_chrono_mkfp_4_5    = (min_chrono_mkfp_4_5 <= _chrono_mkfp_4_5) ?min_chrono_mkfp_4_5    : _chrono_mkfp_4_5;
    //min_chrono_mkfp_1_5    = (min_chrono_mkfp_1_5 <= _chrono_mkfp_1_5) ?min_chrono_mkfp_1_5    : _chrono_mkfp_1_5;
    //
    //max_chrono_mkfp_1_2    = (max_chrono_mkfp_1_2 >= _chrono_mkfp_1_2) ?max_chrono_mkfp_1_2    : _chrono_mkfp_1_2;
    //max_chrono_mkfp_2_3    = (max_chrono_mkfp_2_3 >= _chrono_mkfp_2_3) ?max_chrono_mkfp_2_3    : _chrono_mkfp_2_3;
    //max_chrono_mkfp_3_4    = (max_chrono_mkfp_3_4 >= _chrono_mkfp_3_4) ?max_chrono_mkfp_3_4    : _chrono_mkfp_3_4;
    //max_chrono_mkfp_4_5    = (max_chrono_mkfp_4_5 >= _chrono_mkfp_4_5) ?max_chrono_mkfp_4_5    : _chrono_mkfp_4_5;
    //max_chrono_mkfp_1_5    = (max_chrono_mkfp_1_5 >= _chrono_mkfp_1_5) ?max_chrono_mkfp_1_5    : _chrono_mkfp_1_5;

 end:
    LOG_OUT();
    return status;
}



