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

/** Core NewMadeleine structure.
 */
struct nm_core {

        /** Number of gates.
         */
        uint8_t			 nb_gates;

        /** Array of gates.
         */
        struct nm_gate		 gate_array[NUMBER_OF_GATES];

        /** Number of drivers.
         */
        uint8_t			 nb_drivers;

        /** Array of drivers.
         */
        struct nm_drv	 	 driver_array[NUMBER_OF_DRIVERS];

        /** Array of protocols.
         */
        struct nm_proto		*p_proto_array[NUMBER_OF_PROTOCOLS];

        /** Compiled-in scheduler.
         */
        struct nm_sched 	*p_sched;
};
