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
$Log: mad_types.h,v $
Revision 1.5  2000/03/07 10:38:58  oaumage
- pre-support multi-cluster

Revision 1.4  2000/02/08 17:47:25  oaumage
- prise en compte des types de la net toolbox

Revision 1.3  2000/01/13 14:44:33  oaumage
- adaptation pour la prise en compte de la toolbox
- suppression des fichiers redondant

Revision 1.2  1999/12/15 17:31:26  oaumage
Ajout de la commande de logging de CVS

______________________________________________________________________________
*/

/*
 * Mad_types.h
 * ===========
 */

#ifndef MAD_TYPES_H
#define MAD_TYPES_H

typedef void *p_mad_driver_specific_t;

/*
 * Data structure elements
 * -----------------------
 */
typedef long    mad_link_id_t,              *p_mad_link_id_t;
typedef long    mad_channel_id_t,           *p_mad_channel_id_t;
typedef int     mad_adapter_id_t,           *p_mad_adapter_id_t;
typedef long    mad_cluster_id_t,           *p_mad_cluster_id_t;
typedef int     mad_configuration_size_t,   *p_mad_configuration_size_t;
typedef int     mad_buffer_alignment_t,     *p_mad_buffer_alignment_t;

#endif /* MAD_TYPES_H */
