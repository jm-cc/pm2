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


#ifndef NM_SO_PARAMETERS_H
#define NM_SO_PARAMETERS_H


/** Maximum size of unexpected packets. Used for preallocated header zone within packet wrappers */
#define NM_SO_MAX_UNEXPECTED  (16 * 1024)

/* Number of preallocated iovec entries within packet wrappers */
#define NM_SO_PREALLOC_IOV_LEN 32


#define NM_SO_ALIGN_TYPE      uint32_t
#define NM_SO_ALIGN_FRONTIER  sizeof(NM_SO_ALIGN_TYPE)
#define nm_so_aligned(x)      tbx_aligned((x), NM_SO_ALIGN_FRONTIER)


#endif /* NM_SO_PARAMETERS_H */

