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
$Log: leonie.h,v $
Revision 1.9  2001/01/29 16:59:31  oaumage
- transmission des donnees de configuration

Revision 1.8  2000/12/11 08:31:14  oaumage
- support Leonie

Revision 1.7  2000/06/09 08:45:46  oaumage
- Progression du code

Revision 1.6  2000/05/17 12:40:50  oaumage
- reorganisation du code de demarrage de Leonie

Revision 1.5  2000/05/15 13:51:05  oaumage
- Reorganisation des sources de Leonie


______________________________________________________________________________
*/

/*
 * leonie.h
 * --------
 */

#ifndef __LEONIE_H
#define __LEONIE_H

/*
 * Checks
 * ------
 */
#ifndef __GNUC__
#error The GNU C Compiler is required to build this tool
#endif __GNUC__

#include "pm2debug.h"
#include "ntbx.h"
#include "tbx.h"
#include "leoparse.h"

/* Leonie: data structures */
#include "leo_pointers.h"
#include "leo_types.h"
#include "leo_main.h"

/* Leonie: internal interface */
#include "leo_net.h"
#include "leo_swann.h"
#include "leo_interface.h"

#endif /* __LEONIE_H */
