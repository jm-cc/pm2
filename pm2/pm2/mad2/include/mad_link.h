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
  mad_list_t                buffer_list;
  mad_list_t                user_buffer_list;

  /* Driver specific data */
  p_mad_driver_specific_t   specific;
} mad_link_t;

#endif /* MAD_LINK_H */
