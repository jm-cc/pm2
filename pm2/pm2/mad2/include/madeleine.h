
/*
 * Madeleine.h
 * ===========
 */

#ifndef MAD_H
#define MAD_H

/*
 * Checks
 * ------
 */
#ifndef __GNUC__
#error The GNU C Compiler is required to build the library
#endif __GNUC__

/*
 * Compilation mode
 * ----------------
 */

/*
 * Headers
 * -------
 */
#if (defined PM2) || (defined MAD_FORWARDING)
#include "marcel.h"
#endif /* PM2 || FORWARDING */
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "tbx.h"
#include "ntbx.h"
#include "pm2debug.h"

/* Protocol registration */
#include "mad_registration.h"

/* Structure pointers */
#include "mad_pointers.h"

/* Log generation */
#include "mad_log.h"

/* Fundamental data types */
#include "mad_types.h"
#include "mad_modes.h"
#include "mad_buffers.h"
#include "mad_link.h"
#include "mad_connection.h"
#include "mad_channel.h"
#include "mad_adapter.h"
#include "mad_driver_interface.h"
#include "mad_driver.h"
#include "mad_adapter_description.h"
#include "mad_configuration.h"
#include "mad_cluster.h"
#ifdef MAD_FORWARDING
#include "mad_forward.h"
#endif //MAD_FORWARDING

/* Function prototypes */
#include "mad_memory_interface.h"
#include "mad_buffer_interface.h"
#include "mad_communication_interface.h"
#include "mad_channel_interface.h"
#ifdef MAD_FORWARDING
#include "mad_forward_interface.h"
#endif //MAD_FORWARDING

/* connection interfaces */
#ifdef DRV_TCP
#include "connection_interfaces/mad_tcp.h"
#endif /* DRV_TCP */
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
#ifdef MAD_FORWARDING
#include "connection_interfaces/mad_forwarder.h"
#endif /* MAD_FORWARDING */

#include "mad_main.h"

#if     defined (LEONIE_SPAWN)
#include "mad_leonie_spawn.h"
#elif   defined (APPLICATION_SPAWN)
#include "mad_application_spawn.h"
#elif   defined (EXTERNAL_SPAWN)
#include "mad_external_spawn.h"
#else   /* (= REGULAR_SPAWN) */
#define REGULAR_SPAWN
#include "mad_regular_spawn.h"
#endif /* SPAWN cases */

#include "mad_exit.h"

#endif /* MAD_H */
