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
$Log: tbx_pointers.h,v $
Revision 1.4  2000/09/05 13:58:54  oaumage
- quelques corrections et reorganisations dans le support des types
  structures

Revision 1.3  2000/07/07 14:49:42  oaumage
- Ajout d'un support pour les tables de hachage

Revision 1.2  2000/05/22 12:19:00  oaumage
- listes de recherche

Revision 1.1  2000/01/13 14:51:30  oaumage
Inclusion de la toolbox dans le repository

______________________________________________________________________________
*/

/*
 * Tbx_pointers.h
 * ==============
 */

#ifndef TBX_POINTERS_H
#define TBX_POINTERS_H

typedef struct s_tbx_list_element    *p_tbx_list_element_t;
typedef struct s_tbx_list            *p_tbx_list_t;
typedef struct s_tbx_list_reference  *p_tbx_list_reference_t;
typedef struct s_tbx_slist_element   *p_tbx_slist_element_t;
typedef struct s_tbx_slist           *p_tbx_slist_t;
typedef struct s_tbx_memory          *p_tbx_memory_t;
typedef struct s_tbx_htable_element  *p_tbx_htable_element_t;
typedef struct s_tbx_htable          *p_tbx_htable_t;

#endif /* TBX_POINTERS_H */

