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
$Log: leoparse_types.h,v $
Revision 1.2  2000/12/19 16:57:48  oaumage
- finalisation de leoparse
- exemples pour leoparse
- modification des macros de logging
- version typesafe de certaines macros
- finalisation des tables de hachage
- finalisation des listes de recherche

Revision 1.1  2000/11/02 14:25:04  oaumage
Leoparse


______________________________________________________________________________
*/
/*
 * leoparse_types.h
 * ----------------
 */

#ifndef __LEOPARSE_TYPES_H
#define __LEOPARSE_TYPES_H

typedef enum e_leoparse_entry_type
{
  leoparse_e_undefined = 0,
  leoparse_e_slist,
  leoparse_e_object,
} leoparse_entry_type_t;

typedef struct s_leoparse_htable_entry
{
  char                  *id;
  p_leoparse_object_t    object;
  p_tbx_slist_t          slist;
  leoparse_entry_type_t  type;
} leoparse_htable_entry_t;

typedef enum e_leoparse_object_type
{
  leoparse_o_undefined = 0,
  leoparse_o_id,
  leoparse_o_string,
  leoparse_o_htable,
} leoparse_object_type_t;

typedef struct s_leoparse_object
{
  leoparse_object_type_t  type;
  p_tbx_htable_t          htable;
  char                   *string;
  char                   *id;
} leoparse_object_t;

#endif /* __LEOPARSE_TYPES_H */
