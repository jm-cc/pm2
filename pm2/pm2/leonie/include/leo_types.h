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
$Log: leo_types.h,v $
Revision 1.5  2000/05/31 14:17:05  oaumage
- types Leonie

Revision 1.4  2000/05/23 15:32:49  oaumage
- ajout de types cluster et host

Revision 1.3  2000/05/15 13:51:05  oaumage
- Reorganisation des sources de Leonie


______________________________________________________________________________
*/
/*
 * leo_types.h
 * ----------
 */

#ifndef __LEO_TYPES_H
#define __LEO_TYPES_H

typedef void              *leo_file_handle_t;
typedef leo_file_handle_t *p_file_handle_t;

typedef void undefined_t, *p_undefined_t;

typedef struct s_leo_application_cluster
{
  char          *id;
  p_tbx_slist_t  host_list;
  p_tbx_slist_t  channel_list;
} leo_application_cluster_t, *p_leo_application_cluster_t;

typedef struct s_leo_cluster_definition
{
  char          *id;
  p_tbx_slist_t  host_model_list;
  p_tbx_slist_t  host_list;
} leo_cluster_definition_t, *p_leo_cluster_definition_t;

#endif /* __LEO_TYPES_H */
