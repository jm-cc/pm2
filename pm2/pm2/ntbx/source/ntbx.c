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
$Log: ntbx.c,v $
Revision 1.3  2000/11/07 17:56:48  oaumage
- ajout de commandes de lecture/ecriture resistantes sur sockets

Revision 1.2  2000/03/01 11:03:34  oaumage
- mise a jour des #includes ("")

Revision 1.1  2000/02/08 15:25:28  oaumage
- ajout du sous-repertoire `net' a la toolbox


______________________________________________________________________________
*/

/*
 * Ntbx.c
 * ======
 */

#include "tbx.h"
#include "ntbx.h"

static volatile tbx_bool_t initialized = tbx_false;

void
ntbx_init(int    argc,
	  char **argv)
{
  LOG_IN();
  if (!initialized)
    {
      initialized = tbx_true;
    }
  LOG_OUT();

}

void
ntbx_purge_cmd_line(int   *argc,
		    char **argv)
{
  LOG_IN();
  /* --- */
  LOG_OUT();
}
