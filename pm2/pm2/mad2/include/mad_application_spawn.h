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
$Log: mad_application_spawn.h,v $
Revision 1.1  2000/05/17 14:32:43  oaumage
- reorganisation des sources au niveau de mad_init


______________________________________________________________________________
*/

/*
 * Mad_application_spawn.h
 * =======================
 */

#ifndef MAD_APPLICATION_SPAWN_H
#define MAD_APPLICATION_SPAWN_H

/*
 * Controles du mode de compilation
 * --------------------------------
 */
#ifdef PM2
#error APPLICATION_SPAWN is not yet supported with PM2
#endif /* PM2 */

#ifdef EXTERNAL_SPAWN
#error EXTERNAL_SPAWN cannot be specified with APPLICATION_SPAWN
#endif /* EXTERNAL_SPAWN */

/*
 * Fonctions exportees
 * -------------------
 */
char *
mad_pre_init(p_mad_adapter_set_t adapter_set);

p_mad_madeleine_t
mad_init(ntbx_host_id_t  rank,
	 char           *configuration_file __attribute__ ((unused)),
	 char           *url);

#endif /* MAD_APPLICATION_SPAWN_H */
