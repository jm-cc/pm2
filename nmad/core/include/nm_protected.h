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

#ifndef NM_PROTECTED_H
#define NM_PROTECTED_H

#define NUMBER_OF_GATES          255
#define NUMBER_OF_DRIVERS        255
#define NUMBER_OF_PROTOCOLS      255
#define NUMBER_OF_TRACKS         255

#include "nm_log.h"
#include "nm_errno.h"
#include "nm_sched_ops.h"
#include "nm_sched.h"

#include "nm_drv_cap.h"
#include "nm_drv_ops.h"
#include "nm_drv.h"

#include "nm_trk_cap.h"
#include "nm_trk.h"
#include "nm_trk_rq.h"

#include "nm_gate.h"

#include "nm_pkt_head.h"
#include "nm_pkt_wrap.h"

#include "nm_cnx_rq.h"

#include "nm_proto_cap.h"
#include "nm_proto_id.h"
#include "nm_proto_ops.h"
#include "nm_proto.h"

#include "nm_core.h"

#include <assert.h>

#include "nm_core_inline.h"

#include "nm_network_sampling.h"

#endif /* NM_PROTECTED_H */
