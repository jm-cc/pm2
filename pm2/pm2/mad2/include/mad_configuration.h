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
$Log: mad_configuration.h,v $
Revision 1.4  2000/09/05 14:01:21  oaumage
- support de configurations a plusieurs executables

Revision 1.3  2000/02/08 17:47:21  oaumage
- prise en compte des types de la net toolbox

Revision 1.2  1999/12/15 17:31:20  oaumage
Ajout de la commande de logging de CVS

______________________________________________________________________________
*/

/*
 * Mad_configuration.h
 * ===================
 */

#ifndef MAD_CONFIGURATION_H
#define MAD_CONFIGURATION_H

/*
 * Madeleine configuration
 * -----------------------
 */
typedef struct s_mad_configuration
{
  mad_configuration_size_t     size;
  ntbx_host_id_t               local_host_id;
  char                       **host_name; /* configuration host name list */
  char                       **program_name; /* program name list */
} mad_configuration_t;

#endif /* MAD_CONFIGURATION_H */
