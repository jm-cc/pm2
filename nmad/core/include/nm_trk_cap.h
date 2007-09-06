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

#ifndef NM_TRK_CAP_H
#define NM_TRK_CAP_H

/** Request type expected on the track.
 */
enum nm_trk_request_type {

        /** Uninitialized.
         */
        nm_trk_rq_unspecified	=  0,

        /** Request boundaries not preserved.
         */
        nm_trk_rq_stream	=  1,

        /** Request boundaries preserved.
         */
        nm_trk_rq_dgram		=  2,

        /** Headerless zero copy send/receive track
         * with rendez-vous.
         */
        nm_trk_rq_rdv		=  3,

        /** Headerless zero copy remote put track
         * with rendez-vous.
         */
        nm_trk_rq_put		=  4,

        /** Headerless zero copy remote get track
         * with rendez-vous.
         */
        nm_trk_rq_get		=  5,
};

/** IOVEC capabilities supported on the track.
 */
enum nm_trk_iovec_cap {

        /** Uninitialized.
         */
        nm_trk_iov_unspecified		=  0,


        /** No iov capability.
         */
        nm_trk_iov_none			=  1,

        /** Only on the send side.
         */
        nm_trk_iov_send_only		=  2,

        /** Only on the receive side.
         */
        nm_trk_iov_recv_only		=  3,

        /** On both sides,
         * iovec must be the same on both sides.
         */
        nm_trk_iov_both_sym		=  4,

        /** On both sides,
         * iovec do not need to be the same on both sides.
         */
        nm_trk_iov_both_assym		=  5,
};

/** Track capabilities.
 */
struct nm_trk_cap {

        /** Track request type.
         */
        uint8_t	rq_type;

        /** Iovec type.
         */
        uint8_t iov_type;

        /** Flow control boundaries for send requests.
         */
        uint8_t	max_pending_send_request;

        /** Flow control boundaries for receive requests.
         */
        uint8_t max_pending_recv_request;

        /** Min length for single requests.
         */
        uint64_t min_single_request_length;

        /** Max length for single requests.
         */
        uint64_t max_single_request_length;

        /** Max iovec request cumulated length.
         */
        uint64_t max_iovec_request_length;

        /** Max number of iovec entries.
         */
        uint16_t max_iovec_size;
};

#endif /* NM_TRK_CAP_H */
