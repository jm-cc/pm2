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

#include "bmi.h"
#include <nm_public.h>

extern nm_core_t p_core;

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
    if(ret == NM_ESUCCESS)
       ret = 0;
    return (ret);

}
	
/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 expandtab
 */
