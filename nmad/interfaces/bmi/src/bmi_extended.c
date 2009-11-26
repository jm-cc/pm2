/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <nm_public.h>
#include "bmi.h"
#include "bmi_extended.h"
#include "nmad_bmi_interface.h"

extern nm_session_t p_core;
extern p_tbx_memory_t nm_bmi_mem;

/** Checks to see if a particular message has completed.
 *
 *  \return 0 on success, -errno on failure.
 */
int BMI_wait(bmi_op_id_t id,                
	     int *outcount,                 
	     bmi_error_code_t * error_code, //unused
	     bmi_size_t * actual_size,      
	     void **user_ptr,               //unused
	     int max_idle_time_ms,          //unused with the present implementation
	     bmi_context_id context_id){

    int ret = -1;

    if(outcount)
	*outcount=0;
    if(actual_size)
	*actual_size=0;

    if(id->status == SEND ) {
	ret = nm_sr_swait(p_core, &id->request);
	if(outcount)
	    *outcount = id->request.req.pack.len;
	if(actual_size)
	    *actual_size = id->request.req.pack.len;
    } else { //RECV
	ret = nm_sr_rwait(p_core, &id->request);
	if(outcount) {
	    *outcount = id->request.req.unpack.cumulated_len;
	}
	if(actual_size) {
	    *actual_size = id->request.req.unpack.cumulated_len;
	}
    }
    if(ret == NM_ESUCCESS) {
       ret = 0;
       tbx_free(nm_bmi_mem, id);
    }
    return (ret);

}


static bmi_unexpected_callback_t unexpected_callback = NULL;

/** Function called by NewMadeleine when an unexpected message arrives
 */
void __bmi_event_notifier(nm_sr_event_t event, const nm_sr_event_info_t*event_info)
{

    /* First, check whether the message is unexpected in the BMI meaning (ie. no 
     * BMI_post_recv will ever be called for this message) or if the application 
     * is only late
     */
    enum bnm_msg_type type = BNM_MATCH_GET_MSG_TYPE(event_info->recv_unexpected.tag);

    if(type == BNM_MSG_UNEXPECTED) {
	struct BMI_unexpected_info info = {
	    .error_code=0,
	    .addr=NULL, 	/* todo: add a magic bmi_gate_to_addr() function */
	    .buffer=NULL,	/* ie. the application have to call post_recv */
	    .size=event_info->recv_unexpected.len,
	    .tag=event_info->recv_unexpected.tag};

	(*unexpected_callback)(&info);	
    }
}

/** Register a callback that is invoked each time an unexpected message arrives.
 * 
 * \return 0 on success, -1 on failure.
 */
int BMI_register_unexpected_callback(bmi_unexpected_callback_t callback)
{
    if(unexpected_callback)
	/* another callback is already registered ! */
	return -1;
    unexpected_callback = callback;
    nm_sr_monitor(p_core, 
		  NM_SR_EVENT_RECV_UNEXPECTED, 
		  __bmi_event_notifier);
    return 0;
}

/** Unregister the unexpected callback.
 * 
 * \return 0 on success, -1 on failure.
 */
int BMI_unregister_unexpected_callback(bmi_unexpected_callback_t callback)
{
    if(unexpected_callback == callback) {
	unexpected_callback = NULL;
	return 0;
    }
    return -1;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 expandtab
 */
