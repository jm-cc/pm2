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
$Log: mad_connection.h,v $
Revision 1.4  2000/02/08 17:47:22  oaumage
- prise en compte des types de la net toolbox

Revision 1.3  2000/01/13 14:44:31  oaumage
- adaptation pour la prise en compte de la toolbox
- suppression des fichiers redondant

Revision 1.2  1999/12/15 17:31:21  oaumage
Ajout de la commande de logging de CVS

______________________________________________________________________________
*/

/*
 * Mad_connection.h
 * ================
 */

#ifndef MAD_CONNECTION_H
#define MAD_CONNECTION_H

/*
 * Structures
 * ----------
 */
typedef enum
{
  mad_incoming_connection,
  mad_outgoing_connection
} mad_connection_way_t, *p_mad_connection_way_t;

typedef struct s_mad_connection
{
  /* Common use fields */
           ntbx_host_id_t          remote_host_id;
           p_mad_channel_t         channel;
           p_mad_connection_t      reverse;
           mad_connection_way_t    way;

  /* Internal use fields */
           int                     nb_link; 
           p_mad_link_t            link;
           tbx_list_t              user_buffer_list; 
           tbx_list_reference_t    user_buffer_list_reference; 
           tbx_list_t              buffer_list;
           tbx_list_t              buffer_group_list;
           tbx_list_t              pair_list;
           p_mad_link_t            last_link;
           mad_link_mode_t         last_link_mode;

  /* Flags */
  volatile tbx_bool_t              lock;
           tbx_bool_t              send;
           tbx_bool_t              delayed_send; 
           tbx_bool_t              flushed;
           tbx_bool_t              pair_list_used;
           tbx_bool_t              first_sub_buffer_group;
           tbx_bool_t              more_data;

  /* Driver specific data */
  p_mad_driver_specific_t          specific;
} mad_connection_t;

#endif /* MAD_CONNECTION_H */
