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
$Log: ntbx.h,v $
Revision 1.5  2000/04/27 09:01:29  oaumage
- fusion de la branche pm2_mad2_multicluster

Revision 1.4.2.1  2000/04/03 13:46:00  oaumage
- fichier de commandes `swann'

Revision 1.4  2000/03/08 17:16:38  oaumage
- support de Marcel sans PM2

Revision 1.3  2000/03/01 11:03:23  oaumage
- mise a jour des #includes ("")

Revision 1.2  2000/02/17 09:14:25  oaumage
- ajout du support de TCP a la net toolbox
- ajout de fonctions d'empaquetage de donnees numeriques

Revision 1.1  2000/02/08 15:25:20  oaumage
- ajout du sous-repertoire `net' a la toolbox


______________________________________________________________________________
*/

/*
 * NTbx.h
 * ======
 */

#ifndef NTBX_H
#define NTBX_H

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
#ifdef MARCEL
#include "marcel.h"
#endif /* MARCEL */
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "tbx.h"
#include "ntbx_types.h"
#include "ntbx_net_commands.h"

#ifdef NTBX_TCP
#include "ntbx_tcp.h"
#endif /* NTBX_TCP */

#include "ntbx_interface.h"

#endif /* NTBX_H */
