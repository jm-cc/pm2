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
$Log: mad_link.h,v $
Revision 1.6  2000/06/06 12:54:38  oaumage
- Ajout du calcul de la taille des groupes de buffers dynamiques

Revision 1.5  2000/03/27 08:49:11  oaumage
- pre-support pour decoupage des groupes

Revision 1.4  2000/01/13 14:44:32  oaumage
- adaptation pour la prise en compte de la toolbox
- suppression des fichiers redondant

Revision 1.3  2000/01/05 09:39:40  oaumage
- ajout du champs group_mode :
  * aggregate: .. en emission, les groupes sont de taille maximale
  * split:....... en emission, les groupes sont coupes apres un Express

Revision 1.2  1999/12/15 17:31:21  oaumage
Ajout de la commande de logging de CVS

______________________________________________________________________________
*/

/*
 * Mad_link.h
 * ==========
 */

#ifndef MAD_LINK_H
#define MAD_LINK_H
/*
 * link
 * ----
 */

typedef struct s_mad_link
{
  /* Common use fields */
  p_mad_connection_t        connection;
  mad_link_id_t             id;

  /* Internal use fields */
  mad_link_mode_t           link_mode;
  mad_buffer_mode_t         buffer_mode;
  mad_group_mode_t          group_mode;
  tbx_list_t                buffer_list;
  size_t                    cumulated_length;
  tbx_list_t                user_buffer_list;

  /* Driver specific data */
  p_mad_driver_specific_t   specific;
} mad_link_t;

#endif /* MAD_LINK_H */
