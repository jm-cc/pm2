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
$Log: swann_pointers.h,v $
Revision 1.1  2000/03/27 12:53:57  oaumage
- progression des fonctionnalites:
  * support reseau
  * support execution
  * extension du support fichier


______________________________________________________________________________
*/

/*
 * swann_pointers.h
 * ----------------
 */

#ifndef __SWANN_POINTERS_H
#define __SWANN_POINTERS_H

typedef struct s_swann_net_client *p_swann_net_client_t;
typedef struct s_swann_net_server *p_swann_net_server_t;
typedef struct s_swann_file       *p_swann_file_t;
typedef struct s_swann_command    *p_swann_command_t;

#endif /* __SWANN_POINTERS_H */
