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
$Log: tbx_slist.h,v $
Revision 1.2  2000/05/22 13:45:46  oaumage
- ajout d'une fonction de tri aux listes de recherche
- correction de bugs divers

Revision 1.1  2000/05/22 12:19:00  oaumage
- listes de recherche


______________________________________________________________________________
*/

/*
 * Tbx_slist.h : double-linked search lists
 * ===========--------------------------------
 */

#ifndef TBX_SLIST_H
#define TBX_SLIST_H

/*
 * Data structures 
 * ---------------
 */
typedef struct s_tbx_slist_element
{
  p_tbx_slist_element_t  previous;
  p_tbx_slist_element_t  next;
  void                  *object;
} tbx_slist_element_t;

typedef struct s_tbx_slist
{
  TBX_SHARED;
  tbx_slist_length_t    length;
  p_tbx_slist_element_t head;
  p_tbx_slist_element_t tail;
} tbx_slist_t;

typedef struct s_tbx_slist_reference
{
  p_tbx_slist_t slist;
  p_tbx_slist_element_t reference;
} tbx_slist_reference_t;

typedef tbx_bool_t (*p_tbx_slist_search_func_t)(void *ref_obj, void *obj);
typedef int (*p_tbx_slist_cmp_func_t)(void *ref_obj, void *obj);

#endif /* TBX_SLIST_H */
