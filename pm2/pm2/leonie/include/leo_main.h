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
$Log: leo_main.h,v $
Revision 1.4  2000/05/17 12:40:50  oaumage
- reorganisation du code de demarrage de Leonie


______________________________________________________________________________
*/
/*
 * leo_main.h
 * ---------------
 */

#ifndef __LEO_MAIN_H
#define __LEO_MAIN_H

typedef struct s_leo_swann_module
{
  int                   id;
  char                 *cluster_id;
  p_leo_swann_module_t  relay;
  p_ntbx_client_t       net_client;
} leo_swann_module_t;

typedef struct s_leo_mad_module
{
  int                   id;
  char                 *cluster_id;
  p_leo_swann_module_t  relay;
  p_ntbx_client_t       net_client;
} leo_mad_module_t;

typedef struct s_leonie
{
  int             cluster_counter;
  tbx_list_t      swann_modules;
  tbx_list_t      mad_modules;
  p_ntbx_server_t net_server;
} leonie_t;

#endif /* __LEO_MAIN_H */
