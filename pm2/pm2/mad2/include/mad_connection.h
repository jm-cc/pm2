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
           mad_host_id_t           remote_host_id;
           p_mad_channel_t         channel;
           p_mad_connection_t      reverse;
           mad_connection_way_t    way;

  /* Internal use fields */
           int                     nb_link; 
           p_mad_link_t            link;
           mad_list_t              user_buffer_list; 
           mad_list_reference_t    user_buffer_list_reference; 
           mad_list_t              buffer_list;
           mad_list_t              buffer_group_list;
           mad_list_t              pair_list;
           p_mad_link_t            last_link;
           mad_link_mode_t         last_link_mode;

  /* Flags */
  volatile mad_bool_t              lock;
           mad_bool_t              send;
           mad_bool_t              delayed_send; 
           mad_bool_t              flushed;
           mad_bool_t              pair_list_used;
           mad_bool_t              first_sub_buffer_group;
           mad_bool_t              more_data;

  /* Driver specific data */
  p_mad_driver_specific_t          specific;
} mad_connection_t;

#endif /* MAD_CONNECTION_H */
