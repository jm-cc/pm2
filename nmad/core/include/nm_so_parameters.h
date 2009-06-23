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



/* The following constant defines the maximum number of consecutive
   packs/unpacks that can be requested without waiting the completion
   of the previous ones.

   WARNING: THE ONLY VALUE CURRENTLY SUPPORTED IS 256 */
#define NM_SO_PENDING_PACKS_WINDOW          256

#if defined(CONFIG_STRAT_AGGREG_AUTOEXTENDED)
#  define NM_SO_MAX_UNEXPECTED                (1024 * 1024)
#else
/** Maximum size of unexpected packets. Used for preallocated header zone within packet wrappers */
#  define NM_SO_MAX_UNEXPECTED                (32 * 1024)
#endif

/* Maximum size of a small message */
#define NM_SO_MAX_SMALL                     (NM_SO_MAX_UNEXPECTED -	\
					     NM_SO_GLOBAL_HEADER_SIZE - \
					     NM_SO_DATA_HEADER_SIZE)

/* Number of preallocated iovec entries within packet wrappers */
#define NM_SO_PREALLOC_IOV_LEN 16

/* Threshold used on the sending side to decide if data should be
   copied (when smaller than the threshold) within the packet wrapper
   header zone */
#if defined(CONFIG_STRAT_AGGREG_AUTOEXTENDED)
#  define NM_SO_COPY_ON_SEND_THRESHOLD        (1024 * 1024)
#else
#  define NM_SO_COPY_ON_SEND_THRESHOLD        (32 * 1024)
#endif

/** Maximum active send request per track.
 * @note This is a legacy parameter. Now always 1.
 */
#define NM_SO_MAX_ACTIVE_SEND_PER_TRACK 1

/** Threshold (in bytes) between by-copy / zero-copy with multi-ack methods
 * for datatype transfers
 */
#define NM_SO_DATATYPE_BLOCKSIZE (2*1024)


#define NM_SO_ALIGN_TYPE      uint32_t
#define NM_SO_ALIGN_FRONTIER  sizeof(NM_SO_ALIGN_TYPE)
#define nm_so_aligned(x)      tbx_aligned((x), NM_SO_ALIGN_FRONTIER)


#endif /* NM_SO_PARAMETERS_H */

