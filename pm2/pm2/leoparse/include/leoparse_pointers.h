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
$Log: leoparse_pointers.h,v $
Revision 1.3  2000/12/19 16:57:48  oaumage
- finalisation de leoparse
- exemples pour leoparse
- modification des macros de logging
- version typesafe de certaines macros
- finalisation des tables de hachage
- finalisation des listes de recherche

Revision 1.2  2000/11/03 10:42:31  oaumage
- integration de Leoparse

Revision 1.1  2000/11/02 14:25:03  oaumage
Leoparse


______________________________________________________________________________
*/
/*
 * leoparse_pointers.h
 * -------------------
 */

#ifndef __LEOPARSE_POINTERS_H
#define __LEOPARSE_POINTERS_H

typedef struct s_leoparse_htable_entry   *p_leoparse_htable_entry_t;
typedef enum   e_leoparse_object_type    *p_leoparse_object_type_t;
typedef enum   e_leoparse_entry_type     *p_leoparse_entry_type_t;
typedef struct s_leoparse_object         *p_leoparse_object_t;

#endif /* __LEOPARSE_POINTERS_H */
