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
$Log: tbx_types.h,v $
Revision 1.3  2000/09/05 13:58:54  oaumage
- quelques corrections et reorganisations dans le support des types
  structures

Revision 1.2  2000/05/22 12:19:00  oaumage
- listes de recherche

Revision 1.1  2000/01/13 14:51:31  oaumage
Inclusion de la toolbox dans le repository

______________________________________________________________________________
*/

/*
 * Tbx_types.h
 * ===========
 */

#ifndef TBX_TYPES_H
#define TBX_TYPES_H

/*
 * General purpose types
 * :::::::::::::::::::::______________________________________________________
 */

/*
 * Classical boolean type
 * ----------------------
 */
typedef enum
{
  tbx_false = 0,
  tbx_true
} tbx_bool_t, *p_tbx_bool_t;

/*
 * Flag type
 * ----------------------
 */
typedef enum
{
  tbx_flag_clear = 0,
  tbx_flag_set   = 1
} tbx_flag_t, *p_tbx_flag_t;

/*
 * Comparison type
 * ----------------------
 */
typedef enum
{
  tbx_cmp_eq =  0,  /* == */
  tbx_cmp_gt =  1,  /*  > */
  tbx_cmp_lt = -1   /*  < */
} tbx_cmp_t, *p_tbx_cmp_t;


/*
 * List management related types 
 * :::::::::::::::::::::::::::::______________________________________________
 */
typedef int tbx_list_length_t,        *p_tbx_list_length_t;
typedef int tbx_list_mark_position_t, *p_tbx_list_mark_position_t;


/*
 * SList management related types 
 * ::::::::::::::::::::::::::::::_____________________________________________
 */
typedef int tbx_slist_index_t,  *p_tbx_slist_index_t;
typedef int tbx_slist_length_t, *p_tbx_slist_length_t;
typedef int tbx_slist_offset_t, *p_tbx_slist_offset_t;
typedef tbx_bool_t (*p_tbx_slist_search_func_t)(void *ref_obj, void *obj);
typedef tbx_cmp_t  (*p_tbx_slist_cmp_func_t)(void *ref_obj, void *obj);


#endif /* TBX_TYPES_H */
