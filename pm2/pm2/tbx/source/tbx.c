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
$Log: tbx.c,v $
Revision 1.3  2000/03/13 09:48:38  oaumage
- ajout de l'option TBX_SAFE_MALLOC
- support de safe_malloc

Revision 1.2  2000/03/01 11:03:45  oaumage
- mise a jour des #includes ("")

Revision 1.1  2000/01/13 14:51:33  oaumage
Inclusion de la toolbox dans le repository

______________________________________________________________________________
*/

/*
 * Tbx.c
 * =====
 */

#include "tbx.h"

void tbx_init()
{
  /* Safe malloc */
#ifdef TBX_SAFE_MALLOC
  tbx_safe_malloc_init();
#endif /* TBX_SAFE_MALLOC */

  /* Timer */
  tbx_timing_init();

  /* List manager */
  tbx_list_manager_init();
}
