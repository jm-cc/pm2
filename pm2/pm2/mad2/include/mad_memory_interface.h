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
 * mad_memory_management.h
 * -----------------------
 */

#ifndef __MAD_MEMORY_MANAGEMENT_H
#define __MAD_MEMORY_MANAGEMENT_H

void mad_memory_manager_init(void);
void mad_memory_manager_clean(void);
p_mad_buffer_t mad_alloc_buffer_struct(void);
void mad_free_buffer_struct(p_mad_buffer_t buffer);
p_mad_buffer_t mad_alloc_buffer(size_t length);
void mad_free_buffer(p_mad_buffer_t buf);
p_mad_buffer_group_t mad_alloc_buffer_group_struct(void);
void mad_free_buffer_group_struct(p_mad_buffer_group_t buffer_group);
void mad_foreach_free_buffer_group_struct(void *object);
void mad_foreach_free_buffer(void *object);
p_mad_buffer_pair_t mad_alloc_buffer_pair_struct(void);
void mad_free_buffer_pair_struct(p_mad_buffer_pair_t buffer_pair);
void mad_foreach_free_buffer_pair_struct(void *object);

#endif /* __MAD_MEMORY_MANAGEMENT_H */
