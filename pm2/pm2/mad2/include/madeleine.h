/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.

______________________________________________________________________________
$Log: madeleine.h,v $
Revision 1.13  2000/03/02 14:51:56  oaumage
- support de detection des protocoles au niveau runtime

Revision 1.12  2000/03/02 09:52:12  jfmehaut
pilote Madeleine II/BIP

Revision 1.11  2000/02/28 11:43:59  rnamyst
Changed #include <> into #include "".

Revision 1.10  2000/02/08 17:47:26  oaumage
- prise en compte des types de la net toolbox

Revision 1.9  2000/02/01 17:22:30  rnamyst
Replaced MAD2_MAD1 by PM2.

Revision 1.8  2000/01/31 15:52:03  oaumage
- mad_main.h  : deplacement de aligned_malloc vers la toolbox
- madeleine.h : detection de l'absence de GCC

Revision 1.7  2000/01/21 17:26:56  oaumage
- mise a jour de l'interface de compatibilite /mad1

Revision 1.6  2000/01/21 11:47:37  oaumage
- interface de compatibilite mad1: remplacement de mad_pack/unpack par
  pm2_pack/unpack

Revision 1.5  2000/01/13 14:44:34  oaumage
- adaptation pour la prise en compte de la toolbox
- suppression des fichiers redondant

Revision 1.4  2000/01/10 10:19:42  oaumage
- mad_macros.h: modification de la macro de commande de trace

Revision 1.3  1999/12/15 17:31:26  oaumage
Ajout de la commande de logging de CVS

______________________________________________________________________________
*/

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
 * Headers
 * -------
 */
#ifdef PM2
#include "marcel.h"
#endif /* PM2 */
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "tbx.h"
#include "ntbx.h"

/* Protocol registration */
#include "mad_registration.h"

/* Structure pointers */
#include "mad_pointers.h"

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

/* Function prototypes */
#include "mad_memory_interface.h"
#include "mad_buffer_interface.h"
#include "mad_communication_interface.h"
#include "mad_channel_interface.h"

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

#include "mad_main.h"

#endif /* MAD_H */
