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
$Log: mad_buffer_interface.h,v $
Revision 1.4  2000/06/06 12:54:37  oaumage
- Ajout du calcul de la taille des groupes de buffers dynamiques

Revision 1.3  2000/01/13 14:44:30  oaumage
- adaptation pour la prise en compte de la toolbox
- suppression des fichiers redondant

Revision 1.2  1999/12/15 17:31:18  oaumage
Ajout de la commande de logging de CVS

______________________________________________________________________________
*/

/*
 * Mad_buffer_interface.h
 * ======================
 */

#ifndef MAD_BUFFER_INTERFACE_H
#define MAD_BUFFER_INTERFACE_H

p_mad_buffer_t mad_alloc_buffer(size_t length);
p_mad_buffer_t mad_get_user_send_buffer(void    *ptr, 
					size_t   length);
p_mad_buffer_t mad_get_user_receive_buffer(void    *ptr, 
					   size_t   length);
tbx_bool_t mad_buffer_full(p_mad_buffer_t buffer);
tbx_bool_t mad_more_data(p_mad_buffer_t buffer);
size_t mad_copy_buffer(p_mad_buffer_t   source,
		       p_mad_buffer_t   destination);
p_mad_buffer_t mad_duplicate_buffer(p_mad_buffer_t source);
void mad_make_buffer_group(p_mad_buffer_group_t   buffer_group,
			   p_tbx_list_t           buffer_list, 
			   p_mad_link_t           lnk,
			   size_t                 cumulated_length);
size_t mad_copy_length(p_mad_buffer_t   source, 
		       p_mad_buffer_t   destination);
size_t mad_pseudo_copy_buffer(p_mad_buffer_t   source, 
			      p_mad_buffer_t   destination);
p_mad_buffer_pair_t mad_make_sub_buffer_pair(p_mad_buffer_t source,
					     p_mad_buffer_t destination);

#endif /* MAD_BUFFER_INTERFACE_H */
