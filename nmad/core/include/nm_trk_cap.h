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


enum nm_trk_request_type {

        /* uninitialized
         */
        nm_trk_rq_unspecified	=  0,

        /* request boundaries not preserved
         */
        nm_trk_rq_stream	=  1,

        /* request boundaries preserved
         */
        nm_trk_rq_dgram		=  2,

        /* headerless zero copy send/receive track
         * with rendez-vous
         */
        nm_trk_rq_rdv		=  3,

        /* headerless zero copy remote put track
         * with rendez-vous
         */
        nm_trk_rq_put		=  4,

        /* headerless zero copy remote get track
         * with rendez-vous
         */
        nm_trk_rq_get		=  5,
};

enum nm_trk_iovec_cap {

        /* uninitialized
         */
        nm_trk_iov_unspecified		=  0,


        /* no iov capability
         */
        nm_trk_iov_none			=  1,

        /* only on the send side
         */
        nm_trk_iov_send_only		=  2,

        /* only on the receive side
         */
        nm_trk_iov_recv_only		=  3,

        /* on both sides
         * iovec must be the same on both sides
         */
        nm_trk_iov_both_sym		=  4,

        /* on both sides
         * iovec do not need to be the same on both sides
         */
        nm_trk_iov_both_assym		=  5,
};

struct nm_trk_cap {
        /* track capabilities	*/

        /* track request type			*/
        uint8_t	rq_type;

        /* iovec type				*/
        uint8_t iov_type;

        /* flow control boundaries		*/
        uint8_t	max_pending_send_request;
        uint8_t max_pending_recv_request;

        /* transfert units boundaries		*/
        uint64_t min_single_request_length;
        uint64_t max_single_request_length;
        uint64_t max_iovec_request_length;
        uint16_t max_iovec_size;
};

