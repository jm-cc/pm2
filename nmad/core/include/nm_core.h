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

/*! \mainpage The NewMadeleine API Documentation
 *
 * \section intro_sec Introduction
 * <div class="sec">
 *  NewMadeleine is a complete redesign and rewrite of the
 *  communication library Madeleine. The new architecture aims at
 *  enabling the use of a much wider range of communication flow
 *  optimization techniques. It is entirely modular: The request
 *  scheduler itself is interchangeable, allowing experimentations
 *  with multiple approaches or on multiple issues with regard to
 *  processing communication flows. We have implemented an
 *  optimizing scheduler called SchedOpt. SchedOpt targets
 *  applications with irregular, multi-flow communication schemes such
 *  as found in the increasingly common application conglomerates made
 *  of multiple programming environments and coupled pieces of code,
 *  for instance. SchedOpt itself is easily extensible through the
 *  concepts of optimization strategies  (what to optimize for, what
 *  the optimization goal is) expressed in terms of tactics (how to
 *  optimize to reach the optimization goal). Tactics themselves are
 *  made of basic communication flows operations such as packet
 *  merging or reordering.
 *  More information on NewMadeleine can be
 *  found at http://runtime.futurs.inria.fr/newmadeleine/.
 * </div>
 *
 * \section user_apis User APIs
 * <div class="sec">
 * Several APIs are provided to NewMadeleine users:
 * - The \ref pack_interface
 * - The \ref sr_interface
 * - The \ref mpi_interface
 * </div>
 *
 */

#ifndef NM_CORE_H
#define NM_CORE_H

#include "nm_public.h"
#include "nm_gate.h"
#include "nm_drv.h"

struct nm_proto;
struct nm_sched;

/** Core NewMadeleine structure.
 */
struct nm_core {

        /** Number of gates.
         */
        nm_gate_id_t             nb_gates;

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
#ifdef PIOMAN
	struct piom_server       server;
	struct piom_req          req;
#endif

};

#endif /* NM_CORE_H */
