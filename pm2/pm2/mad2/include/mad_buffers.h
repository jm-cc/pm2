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
 * Mad_buffers.h
 * =============
 */

#ifndef MAD_BUFFERS_H
#define MAD_BUFFERS_H

/* mad_buffer_type: indicates the category of the buffer */
typedef enum
{
  mad_user_buffer,   /* buffer given by the user */
  mad_static_buffer, /* buffer allocated by the low level layer */
  mad_dynamic_buffer, /* buffer allocated by the high level layer */
  mad_internal_buffer /* buffer to be used by the internal routines only */
} mad_buffer_type_t, *p_mad_buffer_type_t;

/* mad_buffer: generic buffer object */
typedef struct s_mad_buffer
{
  void                     *buffer;
  size_t                    length;
  size_t                    bytes_written;
  size_t                    bytes_read;
  mad_buffer_type_t         type;
  p_mad_driver_specific_t   specific; /* may be used by connection
				         layer to store data
				         related to static buffers */
} mad_buffer_t;

/* pair of dynamic/static buffer for 'send_later' handling
   with static buffer links */
typedef struct s_mad_buffer_pair
{
  mad_buffer_t dynamic_buffer;
  mad_buffer_t static_buffer;
} mad_buffer_pair_t;

typedef struct s_mad_buffer_group
{
  mad_list_t      buffer_list;
  p_mad_link_t    link; /* associated link */
} mad_buffer_group_t;

#endif /* MAD_BUFFERS_H */
