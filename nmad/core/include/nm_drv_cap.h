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

struct nm_drv_cap {
        /* driver capabilities						*/

        /* track request types						*/


        /* request boundaries not preserved
         */
        uint8_t has_trk_rq_stream;

        /* request boundaries preserved
         */
        uint8_t has_trk_rq_dgram;

        /* headerless zero copy send/receive track
         * with rendez-vous
         */
        uint8_t has_trk_rq_rdv;

        /* headerless zero copy remote put track
         * with rendez-vous
         */
        uint8_t has_trk_rq_put;

        /* headerless zero copy remote get track
         * with rendez-vous
         */
        uint8_t has_trk_rq_get;



        /* recv selection						*/

        /* may receive from a specific gate
         */
        uint8_t has_selective_receive;

        /* multiple recv request may be active on different gates
         * concurrently
         */
        uint8_t has_concurrent_selective_receive;;


        double *network_sampling_latency;
        int nb_sampling;
};
