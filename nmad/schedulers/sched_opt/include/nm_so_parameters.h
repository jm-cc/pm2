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


#ifndef _NM_SO_PARAMETERS_H_
#define _NM_SO_PARAMETERS_H_

#include "nm_so_headers.h"

/* Maximum number of tags (e.g. logical channels) the application can
   use. 128 means [0..127]. A value smaller than 128 saves space! */
#define NM_SO_MAX_TAGS                      16

/* The following constant defines the maximum number of consecutive
   packs/unpacks that can be requested without waiting the completion
   of the previous ones. */
#define NM_SO_PENDING_PACKS_WINDOW          256

#define NM_SO_MAX_UNEXPECTED                (32 * 1024)

/* Maximum size of a small message */
#define NM_SO_MAX_SMALL                     (NM_SO_MAX_UNEXPECTED - \
					     NM_SO_GLOBAL_HEADER_SIZE - \
					     NM_SO_DATA_HEADER_SIZE)

/* Maximum size of the (preallocated) header zone within packet wrappers */
#define NM_SO_PREALLOC_BUF_LEN              (32 * 1024)

/* Number of preallocated iovec entries within packet wrappers */
#define NM_SO_PREALLOC_IOV_LEN 16

/* Threshold used on the sending side to decide if data should be
   copied (when smaller than the threshold) within the packet wrapper
   header zone */
#define NM_SO_COPY_ON_SEND_THRESHOLD        (32 * 1024)

/* Threasold used on both sides to decide if an optimistic receive
   operation may be attempted */
#define NM_SO_OPTIMISTIC_RECV_LOW_THRESHOLD 32

#define NM_SO_MAX_ACTIVE_SEND_PER_TRACK     1

#define NM_SO_MAX_TRACKS   2

/* Number of available networks when CONFIG_MULTI_RAIL is set */ 
#define NM_SO_MAX_NETS     2

/* Just for clarity of code (when using constant parameters) */
#define NM_SO_TRK(n)  (n)
#define NM_SO_DRV(n)  (n)

/* When CONFIG_MULTI_RAIL is not set but several networks are
   available, NM_SO_DEFAULT_NET selects */
#define NM_SO_DEFAULT_NET  NM_SO_DRV(0)

#define TRK_SMALL         NM_SO_TRK(0)
#define TRK_LARGE         NM_SO_TRK(1)


#endif
