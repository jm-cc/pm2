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
$Log: tbx_list.h,v $
Revision 1.1  2000/01/13 14:51:29  oaumage
Inclusion de la toolbox dans le repository

______________________________________________________________________________
*/

/*
 * Tbx_list.h
 * ==========
 */

#ifndef TBX_LIST_H
#define TBX_LIST_H

typedef void (*p_tbx_list_foreach_func_t)(void * ptr);

typedef struct s_tbx_list_element
{
  void                  *object;
  p_tbx_list_element_t   next ;
  p_tbx_list_element_t   previous ;
} tbx_list_element_t;

typedef struct s_tbx_list
{
  tbx_list_length_t        length ;
  p_tbx_list_element_t     first ;
  p_tbx_list_element_t     last ;
  p_tbx_list_element_t     mark;
  tbx_list_mark_position_t mark_position ;
  tbx_bool_t               mark_next;
  tbx_bool_t               read_only;
} tbx_list_t;

/* list reference: 
 * allow additional cursor/mark over a given list
 * NOTE: the validity of cursor & mark element cannot
 *       be enforced by the list manager
 */
typedef struct s_tbx_list_reference
{
  p_tbx_list_t           list ;
  p_tbx_list_element_t   reference;
  tbx_bool_t             after_end;
} tbx_list_reference_t;

#endif /* TBX_LIST_H */
