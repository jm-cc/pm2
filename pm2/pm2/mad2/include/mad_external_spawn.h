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
$Log: mad_external_spawn.h,v $
Revision 1.3  2000/11/20 10:26:45  oaumage
- initialisation, nouvelle version

Revision 1.2  2000/11/16 14:21:50  oaumage
- correction external spawn

Revision 1.1  2000/05/17 14:32:44  oaumage
- reorganisation des sources au niveau de mad_init


______________________________________________________________________________
*/

/*
 * Mad_external_spawn.h
 * ====================
 */

#ifndef MAD_EXTERNAL_SPAWN_H
#define MAD_EXTERNAL_SPAWN_H

/*
 * Controles du mode de compilation
 * --------------------------------
 */
#ifdef APPLICATION_SPAWN
#error APPLICATION_SPAWN cannot be specified with EXTERNAL_SPAWN
#endif /* APPLICATION_SPAWN */

/*
 * Fonctions exportees
 * -------------------
 */

p_mad_madeleine_t
mad_init(int                  *argc,
	 char                **argv,
	 char                 *configuration_file,
	 p_mad_adapter_set_t   adapter_set);

void
mad_spawn_driver_init(p_mad_madeleine_t   madeleine,
		      int                *argc,
		      char              **argv);

void
mad_exchange_connection_info(p_mad_madeleine_t madeleine);

#endif /* MAD_EXTERNAL_SPAWN_H */
