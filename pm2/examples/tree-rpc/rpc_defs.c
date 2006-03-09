
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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

#define PM2_RPC_DEFS_C
#include "rpc_defs.h"

/* FAC */

PACK_REQ_STUB(DICHOTOMY)
   old_mad_pack_int(MAD_BY_COPY, &arg->inf, 1);
   old_mad_pack_int(MAD_BY_COPY, &arg->sup, 1);
END_STUB

UNPACK_REQ_STUB(DICHOTOMY)
   old_mad_unpack_int(MAD_BY_COPY, &arg->inf, 1);
   old_mad_unpack_int(MAD_BY_COPY, &arg->sup, 1);
END_STUB

PACK_RES_STUB(DICHOTOMY)
   old_mad_pack_int(MAD_BY_COPY, &arg->res, 1);
END_STUB

UNPACK_RES_STUB(DICHOTOMY)
   old_mad_unpack_int(MAD_BY_COPY, &arg->res, 1);
END_STUB


