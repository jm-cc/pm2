#include <stdlib.h>
#include <stdio.h>
#include <sched.h>

#include "madeleine.h"


void
mad_s_make_progress(p_mad_driver_t driver,
                    p_mad_adapter_t adapter){
    p_mad_driver_interface_t interface = NULL;
    //p_mad_adapter_t   adapter   = NULL;
    LOG_IN();
    //DISP("-->s_mkp");
    //adapter =  tbx_htable_get(driver->adapter_htable, "default");

    interface = driver->interface;
    interface->s_make_progress(adapter);
    LOG_OUT();
    //DISP("<--s_mkp");
}



void
mad_r_make_progress(p_mad_driver_t driver,
                    p_mad_adapter_t adapter,
                    p_mad_channel_t channel){
    p_mad_driver_interface_t interface = NULL;
    //p_mad_adapter_t   adapter   = NULL;
    LOG_IN();
    //DISP("-->r_mkp");
    //adapter =  tbx_htable_get(driver->adapter_htable, "default");

    interface = driver->interface;
    interface->r_make_progress(adapter, channel);
    LOG_OUT();
    //DISP("<--r_mkp");
}
