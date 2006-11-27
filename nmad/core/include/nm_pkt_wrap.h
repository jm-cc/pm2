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

#ifndef _NM_PKT_WRAP_H_
#define _NM_PKT_WRAP_H_

#ifdef XPAULETTE
#include "xpaul.h"
#endif

struct nm_iovec {
    /* io vector entry meta-data	*/

    /* Flags */
    uint32_t		 priv_flags;

};

struct nm_iovec_iter {
    /* io vector iterator		*/

        /* Flags */
        uint32_t		 priv_flags;

        /* current entry num
         */
        uint32_t		 v_cur;

        /* current ptr and len copy since the io vector must be modified in-place
         */
    struct iovec		 cur_copy;

    /* current size
         */
        uint32_t		 v_cur_size;
};

struct nm_pkt_wrap {

#ifdef XPAULETTE
	struct xpaul_req	 inst;
	int                      err;  
#endif /* XPAULETTE */

        /* internal packet wrapper	*/


        /* scheduler fields 						*/

        /* assignated driver
         */
        struct nm_drv		*p_drv;

        /* assignated track
         */
        struct nm_trk		*p_trk;

        /* assignated gate, if relevant
         */
        struct nm_gate		*p_gate;

        /* assignated gate driver, if relevant
         */
        struct nm_gate_drv	*p_gdrv;

        /* assignated gate track, if relevant
         */
        struct nm_gate_trk	*p_gtrk;

        /* assignated protocol if relevant
         */
        struct nm_proto		*p_proto;

        /* protocol id
           - communication flow this packet belongs to
           - 0 means any
         */
        uint8_t			 proto_id;

        /* sequence number for the given protocol id
           - rank in the communication flow this packet belongs to
         */
        uint8_t			 seq;

        /* implementation dependent data				*/

        /* scheduler implementation data
         */
        void			*sched_priv;

        /* driver implementation data
         */
        void			*drv_priv;

        /* gate data
         */
        void			*gate_priv;


        /* protocol data
         */
        void			*proto_priv;

        /* packet related fields					*/

        /* packet internal flags
         */
        uint32_t		 pkt_priv_flags;

        /* Cumulated amount of data (everything included) referenced
           by this wrap */
        uint64_t		 length;


        /* io vector setting
           - see pkt_head struct for details
         */
        uint8_t 		 iov_flags;

        /* header
           - shortcut to pkt header vector entry (if relevant), or NULL
           - this field is to be used for convenience when needed, it does
           not imply that every pkt_wrap iov should have only one header,
           or any header at all
        */
        struct nm_pkt_head	*p_pkt_head;

        /* body
           - shortcut to pkt first data block vector entry (if relevant),
           or NULL
           - this field is to be used for convenience when needed, it does
           not imply that every pkt_wrap iov should be organized as a
           header+body msg
        */
        void			*data;

        /* length vector for iov headers (if needed)
           - shortcut and ptr to length vector buffer
           - each entry must by 8-byte long, low to high weight bytes
         */
        uint8_t			*len_v;


        /* io vector internal flags
         */
        uint32_t		 iov_priv_flags;

        /* io vector size
         */
        uint32_t		 v_size;

        /* first used entry in io vector, not necessarily 0, to allow for prepend
           - first unused entry before iov contents is v[v_first - 1]
         */
        uint32_t		 v_first;

        /* number of used entries in io vector
           - first unused entry after iov  contents is v[v_first + v_nb]
         */
        uint32_t		 v_nb;

        /* io vector
         */
        struct iovec		*v;

        /* io vector per-entry meta-data, if needed, or NULL
         */
        struct nm_iovec		*nm_v;
};

#endif /* _NM_PKT_WRAP_H_ */
