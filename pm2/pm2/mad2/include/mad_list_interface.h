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
$Log: mad_list_interface.h,v $
Revision 1.2  1999/12/15 17:31:22  oaumage
Ajout de la commande de logging de CVS

______________________________________________________________________________
*/

/*
 * mad_list_interface.h
 * --------------------
 */

#ifndef __MAD_LIST_INTERFACE_H
#define __MAD_LIST_INTERFACE_H

void mad_list_manager_init(void);
void mad_list_init(p_mad_list_t list);
void mad_foreach_destroy_list(p_mad_list_t                list,
			      p_mad_list_foreach_func_t   func);
void mad_list_manager_exit(void);
void mad_destroy_list(p_mad_list_t list);
void mad_append_list(p_mad_list_t   list,
		     void*          object);
void *mad_get_list_object(p_mad_list_t list);
void mad_mark_list(p_mad_list_t list);
void mad_duplicate_list(p_mad_list_t   source,
			p_mad_list_t   destination);
void mad_extract_sub_list(p_mad_list_t   source,
			  p_mad_list_t   destination);
mad_bool_t mad_empty_list(p_mad_list_t list);
void mad_list_reference_init(p_mad_list_reference_t   ref,
			     p_mad_list_t             list);
void      *mad_get_list_reference_object(p_mad_list_reference_t ref);
mad_bool_t mad_forward_list_reference(p_mad_list_reference_t ref);
void mad_reset_list_reference(p_mad_list_reference_t ref);
mad_bool_t mad_reference_after_end_of_list(p_mad_list_reference_t ref);

#endif /* __MAD_LIST_INTERFACE_H */
