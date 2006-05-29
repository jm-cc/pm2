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


/* control pkt of mini sched
 *
 * send over trk 0 to announce the arrival of a data pkt over trk 1
 */
struct nm_mini_ctrl {

        /* protocol id for the forthcoming pkt
         */
        uint8_t	proto_id;

        /* sequence number for the forthcoming pkt
         */
        uint8_t seq;
};
