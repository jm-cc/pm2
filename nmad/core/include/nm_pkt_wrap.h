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

#ifndef NM_PKT_WRAP_H
#define NM_PKT_WRAP_H

#include <sys/uio.h>

#ifdef PIOMAN
#include "pioman.h"
#endif
#include <ccs_public.h>

struct nm_drv;
struct nm_trk;
struct nm_gate;
struct nm_pkt_head;

/** IO vector entry meta-data.
 */
struct nm_iovec {

    /** Flags. */
    uint32_t		 priv_flags;

};

/** Io vector iterator.
 */
struct nm_iovec_iter {

        /** Flags.
         */
        uint32_t		 priv_flags;

        /** Current entry num.
         */
        uint32_t		 v_cur;

        /** Current ptr and len copy since the io vector must be modified in-place.
         */
        struct iovec		 cur_copy;

        /** Current size.
         */
        uint32_t		 v_cur_size;
};

/** Internal packet wrapper.
 */
struct nm_pkt_wrap {

#ifdef PIOMAN
	struct piom_req	 inst;
	int err;
	enum{
		RECV,
		SEND,
		NONE,
	}which;
	p_tbx_slist_t           slist;
#endif /* PIOMAN */

#if(defined(PIOMAN) && defined(PIO_OFFLOAD))
  tbx_bool_t data_to_offload;
#endif

        /* Scheduler fields.
         */

        /** Assignated driver.
         */
        struct nm_drv		*p_drv;

        /** Assignated track.
         */
        struct nm_trk		*p_trk;

        /** Assignated gate, if relevant.
         */
        struct nm_gate		*p_gate;

        /** Assignated gate driver, if relevant.
         */
        struct nm_gate_drv	*p_gdrv;

        /** Assignated gate track, if relevant.
         */
        struct nm_gate_trk	*p_gtrk;

        /** Protocol id.
           - communication flow this packet belongs to
           - 0 means any
         */
        uint8_t			 proto_id;

        /** Sequence number for the given protocol id.
           - rank in the communication flow this packet belongs to
         */
        uint8_t			 seq;

        /* Implementation dependent data.				*/

        /** Scheduler implementation data.
         */
        void			*sched_priv;

        /** Driver implementation data.
         */
        void			*drv_priv;

        /** Gate data.
         */
        void			*gate_priv;


        /** Protocol data.
         */
        void			*proto_priv;

        /* Packet related fields.					*/

        /** Packet internal flags.
         */
        uint32_t		 pkt_priv_flags;

        /** Cumulated amount of data (everything included) referenced
            by this wrap.
        */
        uint32_t		 length;


        /** Io vector setting.
           - see pkt_head struct for details
         */
        uint8_t 		 iov_flags;

        /** Header.
           - shortcut to pkt header vector entry (if relevant), or NULL
           - this field is to be used for convenience when needed, it does
           not imply that every pkt_wrap iov should have only one header,
           or any header at all
        */
        struct nm_pkt_head	*p_pkt_head;

        /** Body.
           - shortcut to pkt first data block vector entry (if relevant),
           or NULL
           - this field is to be used for convenience when needed, it does
           not imply that every pkt_wrap iov should be organized as a
           header+body msg
        */
        void			*data;

        /** Length vector for iov headers (if needed).
           - shortcut and ptr to length vector buffer
           - each entry must by 8-byte long, low to high weight bytes
         */
        uint8_t			*len_v;


        /** IO vector internal flags.
         */
        uint32_t		 iov_priv_flags;

        /** IO vector size.
         */
        uint32_t		 v_size;

        /** First used entry in io vector, not necessarily 0, to allow for prepend.
           - first unused entry before iov contents is v[v_first - 1]
         */
        uint32_t		 v_first;

        /** Number of used entries in io vector.
           - first unused entry after iov  contents is v[v_first + v_nb]
         */
        uint32_t		 v_nb;

        /** IO vector.
         */
        struct iovec		*v;

        /** IO vector per-entry meta-data, if needed, or NULL.
         */
        struct nm_iovec		*nm_v;

        tbx_tick_t start_transfer_time;

        struct DLOOP_Segment *segp;
        DLOOP_Offset datatype_offset; // Current position in the datatype

};

#endif /* NM_PKT_WRAP_H */

