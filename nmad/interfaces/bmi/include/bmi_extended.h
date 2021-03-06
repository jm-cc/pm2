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

/** \defgroup bmiint BMI network interface
 *
 *  The BMI extended interface provides additional functionality used 
 *  for communication between clients and servers.  Both clients and 
 *  servers use this interface, and the default flow protocol also builds 
 *  on BMI. This extended interface provides NewMadeleine-specific 
 *  functionalities that are not available in the original BMI interface
 * @{
 */

#ifndef __BMI_EXTENDED_H
#define __BMI_EXTENDED_H

#include <nm_public.h>

/* BMI is only available for 'huge tags' (ie. at least 64 bits) */
#ifdef NM_TAGS_AS_INDIRECT_HASH

#include "bmi-types.h"

typedef void (*bmi_unexpected_callback_t)(struct BMI_unexpected_info*);

/** Checks to see if a particular message has completed.
 *
 *  \return 0 on success, -errno on failure.
 */
int BMI_wait(bmi_op_id_t id,                
	     int *outcount,                 
	     bmi_error_code_t * error_code,
	     bmi_size_t * actual_size,
	     void **user_ptr,
	     int max_idle_time_ms,
	     bmi_context_id context_id);

/** Register a callback that is invoked each time an unexpected message arrives.
 * 
 * \return 0 on success, -1 on failure.
 */
int BMI_register_unexpected_callback(bmi_unexpected_callback_t callback);

/** Unregister the unexpected callback.
 * 
 * \return 0 on success, -1 on failure.
 */
int BMI_unregister_unexpected_callback(bmi_unexpected_callback_t callback);

#endif /* NM_TAGS_AS_INDIRECT_HASH */

#endif /* __BMI_EXTENDED_H */

/* @} */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 expandtab
 */
