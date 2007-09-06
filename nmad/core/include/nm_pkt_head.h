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

#ifndef NM_PKT_HEAD_H
#define NM_PKT_HEAD_H

/** Packet header.
 */
struct nm_pkt_head {

        /** Data interpreter.
         */
        uint8_t proto_id;

        /** Sequence number for the given protocol id.
         */
        uint8_t seq;

        /** Driver pre-assigned for transmitting data
         * driver number in gate numbering.
         */
        uint8_t drv_num;

        /** Track pre-assigned for transmitting data
         * track number.
         */
        uint8_t trk_num;

        /** Total length: low to high weight bytes.
         */
        uint8_t length[8];

        /** Last vector index used.
         */
        uint8_t last;

        /** Iovec headers.
         * bit 0:
         * 0 - no main/iovec header
         * 1 - reference main and iovec header in iovec
         *
         * bit 1:
         * 0 - no length vector header
         * 1 - reference length vector header in iovec
         *
         * bit 2: block type
         * 0 - headerless data
         * 1 - packet
         * - wrapped packet types may only be data, packet data or multipacket
         *
         * bit 3-7: reserved, must be 0
         */
        uint8_t iov_flags;
};

#endif /* NM_PKT_HEAD_H */
