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
$Log: mad_regular_spawn.h,v $
Revision 1.1  2000/05/17 14:32:45  oaumage
- reorganisation des sources au niveau de mad_init


______________________________________________________________________________
*/

/*
 * Mad_regular_spawn.h
 * ===================
 */

#ifndef MAD_REGULAR_SPAWN_H
#define MAD_REGULAR_SPAWN_H

/*
 * Fonctions exportees
 * -------------------
 */
#ifdef PM2
p_mad_madeleine_t
mad2_init(int                  *argc,
	  char                **argv,
	  char                 *configuration_file,
	  p_mad_adapter_set_t   adapter_set);
#else /* PM2 */
p_mad_madeleine_t
mad_init(
	 int                   *argc,
	 char                 **argv,
	 char                  *configuration_file __attribute__ ((unused)),
	 p_mad_adapter_set_t    adapter_set);
#endif /* PM2 */

#endif /* MAD_REGULAR_SPAWN_H */
