/*
 * This file has been imported from PVFS2 implementation of BMI
 *
 * (C) 2001 Clemson University and The University of Chicago
 *
 * See COPYING in top-level directory.
 * NewMadeleine
 * Copyright (C) 2009 (see AUTHORS file)
 */

#ifndef __BMI_METHOD_CALLBACK_H
#define __BMI_METHOD_CALLBACK_H

/* BMI is only available for 'huge tags' (ie. at least 64 bits) */
#ifdef NM_TAGS_AS_INDIRECT_HASH

#include "bmi-method-support.h"

BMI_addr_t bmi_method_addr_reg_callback(bmi_method_addr_p map);
int bmi_method_addr_forget_callback(BMI_addr_t addr);

#endif /* NM_TAGS_AS_INDIRECT_HASH */

#endif /* __BMI_METHOD_CALLBACK_H */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 expandtab
 */
