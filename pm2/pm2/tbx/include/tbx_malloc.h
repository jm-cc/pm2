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
$Log: tbx_malloc.h,v $
Revision 1.3  2000/03/13 09:48:19  oaumage
- ajout de l'option TBX_SAFE_MALLOC
- support de safe_malloc
- extension des macros de logging

Revision 1.2  2000/03/08 17:16:05  oaumage
- support de Marcel sans PM2
- support de tmalloc en mode `Marcel'

Revision 1.1  2000/01/13 14:51:30  oaumage
Inclusion de la toolbox dans le repository

______________________________________________________________________________
*/

/*
 * tbx_malloc.h
 * ------------
 */

#ifndef TBX_MALLOC_H
#define TBX_MALLOC_H

/*
 * tbx_safe_malloc
 * ---------------
 */
typedef enum 
{
  tbx_safe_malloc_ERRORS_ONLY,
  tbx_safe_malloc_VERBOSE
} tbx_safe_malloc_mode_t, *p_tbx_safe_malloc_mode_t;


/*
 * tbx_malloc
 * ----------
 */
typedef struct s_tbx_memory
{
  TBX_SHARED; 
  void    *first_mem;
  void    *current_mem;
  size_t   block_len;
  long     mem_len;
  void    *first_free;
  long     first_new;
} tbx_memory_t;

#endif /* TBX_MALLOC_H */
