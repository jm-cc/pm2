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
$Log: tbx.h,v $
Revision 1.7  2000/12/19 16:57:50  oaumage
- finalisation de leoparse
- exemples pour leoparse
- modification des macros de logging
- version typesafe de certaines macros
- finalisation des tables de hachage
- finalisation des listes de recherche

Revision 1.6  2000/07/07 14:49:41  oaumage
- Ajout d'un support pour les tables de hachage

Revision 1.5  2000/05/22 12:18:59  oaumage
- listes de recherche

Revision 1.4  2000/03/08 17:16:00  oaumage
- support de Marcel sans PM2
- support de tmalloc en mode `Marcel'

Revision 1.3  2000/03/01 11:02:49  oaumage
- mise a jour des commandes #include ("")

Revision 1.2  2000/01/31 15:59:07  oaumage
- detection de l'absence de GCC
- ajout de aligned_malloc

Revision 1.1  2000/01/13 14:51:29  oaumage
Inclusion de la toolbox dans le repository

______________________________________________________________________________
*/

/*
 * Tbx.h
 * ===========
 */

#ifndef TBX_H
#define TBX_H

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
#include "pm2debug.h"

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "tbx_timing.h"
#include "tbx_macros.h"
#include "tbx_pointers.h"
#include "tbx_types.h"

#include "tbx_malloc.h"
#include "tbx_slist.h"
#include "tbx_list.h"
#include "tbx_htable.h"

#include "tbx_interface.h"

#endif /* TBX_H */
