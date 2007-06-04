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

/** Driver capabilities.
 */
struct nm_drv_cap {

        /* track request types						*/


        /** Request boundaries not preserved.
         */
        uint8_t has_trk_rq_stream;

        /** Request boundaries preserved.
         */
        uint8_t has_trk_rq_dgram;

        /** Headerless zero copy send/receive track
         * with rendez-vous.
         */
        uint8_t has_trk_rq_rdv;

        /** Headerless zero copy remote put track
         * with rendez-vous.
         */
        uint8_t has_trk_rq_put;

        /** Headerless zero copy remote get track
         * with rendez-vous.
         */
        uint8_t has_trk_rq_get;

	/** Preferred length for switching to rendez-vous.
	 */
	int rdv_threshold;

        /* recv selection						*/

        /** May receive from a specific gate.
         */
        uint8_t has_selective_receive;

        /** Multiple recv request may be active on different gates
         * concurrently.
         */
        uint8_t has_concurrent_selective_receive;

        /** Driver performance sampling
         */
        double *network_sampling_latency;
        int nb_sampling;
};
