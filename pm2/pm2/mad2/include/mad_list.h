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
$Log: mad_list.h,v $
Revision 1.3  2000/01/05 15:47:57  oaumage
- mad_list.h: changement de `len' en `length'
- mad_macros.h: ajout de la macros PM2_TRYLOCK_SHARED

Revision 1.2  1999/12/15 17:31:22  oaumage
Ajout de la commande de logging de CVS

______________________________________________________________________________
*/

/*
 * Mad_list.h
 * ==========
 */

#ifndef MAD_LIST_H
#define MAD_LIST_H

typedef void (*p_mad_list_foreach_func_t)(void * ptr);

typedef struct s_mad_list_element
{
  void                  *object;
  p_mad_list_element_t   next ;
  p_mad_list_element_t   previous ;
} mad_list_element_t;

typedef struct s_mad_list
{
  mad_list_length_t        length ;
  p_mad_list_element_t     first ;
  p_mad_list_element_t     last ;
  p_mad_list_element_t     mark;
  mad_list_mark_position_t mark_position ;
  mad_bool_t               mark_next;
  mad_bool_t               read_only;
} mad_list_t;

/* list reference: 
 * allow additional cursor/mark over a given list
 * NOTE: the validity of cursor & mark element cannot
 *       be enforced by the list manager
 */
typedef struct s_mad_list_reference
{
  p_mad_list_t           list ;
  p_mad_list_element_t   reference;
  mad_bool_t             after_end;
} mad_list_reference_t;

#endif /* MAD_LIST_H */
