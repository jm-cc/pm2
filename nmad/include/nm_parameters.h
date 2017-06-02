/*
 * NewMadeleine
 * Copyright (C) 2006-2017 (see AUTHORS file)
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


#ifndef NM_PARAMETERS_H
#define NM_PARAMETERS_H


/** Maximum size of unexpected packets. Used for preallocated header zone within packet wrappers */
#define NM_SO_MAX_UNEXPECTED  (64 * 1024)

/** size of the queue to submit requests */
#define NM_REQ_CHUNK_QUEUE_SIZE 256

/** minimum density to consider chunks as large */
#define NM_LARGE_MIN_DENSITY  1024

/** default window size for aggregation */
#define NM_SO_DEFAULT_WINDOW  64

/** Number of preallocated iovec entries within packet wrappers */
#define NM_SO_PREALLOC_IOV_LEN 32

/** threshold to switch from plain copy to iovec send */
#define NM_DATA_IOV_THRESHOLD 512

#define NM_ALIGN_TYPE      uint32_t
#define NM_ALIGN_FRONTIER  sizeof(NM_ALIGN_TYPE)
#define nm_aligned(x)      tbx_aligned((x), NM_ALIGN_FRONTIER)

/** activate nm profiling 
 * change the define here, there's no autoconf flag */
//#define NMAD_PROFILE

#endif /* NM_PARAMETERS_H */

