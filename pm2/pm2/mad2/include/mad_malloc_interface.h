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
$Log: mad_malloc_interface.h,v $
Revision 1.2  1999/12/15 17:31:24  oaumage
Ajout de la commande de logging de CVS

______________________________________________________________________________
*/

/*
 * mad_malloc_interface.h
 * ----------------------
 */

#ifndef MAD_MALLOC_INTERFACE_H
#define MAD_MALLOC_INTERFACE_H

void
mad_malloc_init(p_mad_memory_t  *mem,
		size_t           block_len,
		long             initial_block_number);

void *
mad_malloc(p_mad_memory_t mem);

void
mad_free(p_mad_memory_t   mem, 
         void            *ptr);

void
mad_malloc_clean(p_mad_memory_t memory);

#endif /* MAD_MALLOC_INTERFACE_H */
