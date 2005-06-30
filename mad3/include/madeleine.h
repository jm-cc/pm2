/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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

/*
 * Madeleine.h
 * ===========
 */

#ifndef MADELEINE_H
#define MADELEINE_H

/*
 * Checks
 * ------
 */
#ifndef __GNUC__
#error The GNU C Compiler is required to build the library
#endif // __GNUC__

/*
 * Compilation mode
 * ----------------
 */
// #define MAD_FORWARD_FLOW_CONTROL


/*
 * Headers
 * -------
 */
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "tbx.h"
#include "ntbx.h"
#include "tbx_debug.h"

/* Protocol registration */
#include "mad_registration.h"

/* Structure pointers */
#include "mad_pointers.h"

/* Log generation */
#include "mad_log.h"

/* Fundamental data types */
#include "mad_types.h"
#include "mad_leonie_commands.h"
#include "mad_modes.h"
#include "mad_buffers.h"
#include "mad_link.h"
#include "mad_connection.h"
#include "mad_channel.h"
#include "mad_adapter.h"
#include "mad_directory.h"
#include "mad_driver_interface.h"
#include "mad_driver.h"
#include "mad_forward.h"
#include "mad_mux.h"

/* Function prototypes */
#include "mad_memory_interface.h"
#include "mad_buffer_interface.h"
#include "mad_constructor.h"
#include "mad_communication_interface.h"
#include "mad_channel_interface.h"

#include "mad_buffers_inline.h"
#include "mad_memory_inline.h"

/* connection interfaces */
#ifdef DRV_TCP
#include "connection_interfaces/mad_tcp.h"
#endif /* DRV_TCP */
#ifdef DRV_VRP
#include "connection_interfaces/mad_vrp.h"
#endif /* DRV_VRP */
#ifdef DRV_RAND
#include "connection_interfaces/mad_rand.h"
#endif /* DRV_RAND */
#ifdef DRV_UDP
#include "connection_interfaces/mad_udp.h"
#endif /* DRV_UDP */
#ifdef DRV_VIA
#include "connection_interfaces/mad_via.h"
#endif /* DRV_VIA */
#ifdef DRV_SISCI
#include "connection_interfaces/mad_sisci.h"
#endif /* DRV_SISCI */
#ifdef DRV_SBP
#include "connection_interfaces/mad_sbp.h"
#endif /* DRV_SISCI */
#ifdef DRV_MPI
#include "connection_interfaces/mad_mpi.h"
#endif /* DRV_MPI */
#ifdef DRV_BIP
#include "connection_interfaces/mad_bip.h"
#endif /* DRV_BIP */
#ifdef DRV_GM
#include "connection_interfaces/mad_gm.h"
#endif /* DRV_GM */
#ifdef DRV_MX
#include "connection_interfaces/mad_mx.h"
#endif /* DRV_MX */
#ifdef DRV_QUADRICS
#include "connection_interfaces/mad_quadrics.h"
#endif /* DRV_QUADRICS */

#include "mad_main.h"
#include "mad_exit.h"

#endif /* MADELEINE_H */
