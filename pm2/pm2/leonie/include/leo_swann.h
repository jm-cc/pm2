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
 suppong documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.

______________________________________________________________________________
$Log: leo_swann.h,v $
Revision 1.2  2000/06/09 08:45:45  oaumage
- Progression du code

Revision 1.1  2000/05/15 13:51:05  oaumage
- Reorganisation des sources de Leonie


______________________________________________________________________________
*/

/*
 * Leo_swann.h
 * ===========
 */ 

#ifndef __LEO_SWANN_H
#define __LEO_SWANN_H

p_leo_swann_module_t
leo_launch_swann_module(p_leonie_t                  leonie,
			p_leo_application_cluster_t cluster);

p_leo_mad_module_t
leo_launch_mad_module(p_leonie_t                  leonie,
		      p_leo_application_cluster_t cluster);

#endif /* __LEO_SWANN_H */
