/*
 * NewMadeleine
 * Copyright (C) 2014-2015 (see AUTHORS file)
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

#include "nm_coll.h"


/* ********************************************************* */

/** maximum tag usable by enduser for p2p */
#define NM_COLL_TAG_MASK_P2P      0x7FFFFFFF
/** base tag used for collectives */
#define NM_COLL_TAG_BASE          0xFF000000
/** comm_create */
#define NM_COLL_TAG_COMM_CREATE ( NM_COLL_TAG_BASE | 0x01 )
/** barrier */
#define NM_COLL_TAG_BARRIER     ( NM_COLL_TAG_BASE | 0x02 )
/** bcast */
#define NM_COLL_TAG_BCAST       ( NM_COLL_TAG_BASE | 0x03 )
/** gather */
#define NM_COLL_TAG_GATHER      ( NM_COLL_TAG_BASE | 0x04 )
