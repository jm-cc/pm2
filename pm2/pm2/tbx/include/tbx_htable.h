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
$Log: tbx_htable.h,v $
Revision 1.1  2000/07/07 14:49:42  oaumage
- Ajout d'un support pour les tables de hachage


______________________________________________________________________________

*/

/*
 * Tbx_htable.h : hash-table
 * ============----------------
 */

#ifndef TBX_HTABLE_H
#define TBX_HTABLE_H

/*
 * Data types 
 * ----------
 */
typedef char             *tbx_htable_key_t;
typedef tbx_htable_key_t *p_tbx_htable_key_t;

typedef int                        tbx_htable_bucket_count_t;
typedef tbx_htable_bucket_count_t *p_tbx_htable_bucket_count_t

typedef int                         tbx_htable_element_count_t;
typedef tbx_htable_element_count_t *p_tbx_htable_element_count_t

/*
 * Data structures 
 * ---------------
 */
typedef struct s_tbx_htable_element
{
  tbx_htable_key_t        key;
  void                   *object;
  p_tbx_htable_element_t  suivant;
} tbx_htable_element_t;

typedef struct s_tbx_htable
{
  TBX_SHARED;
  tbx_htable_bucket_count_t   nb_bucket;
  p_tbx_htable_element_t     *bucket_array;
  tbx_htable_element_count_t  nb_element;
} tbx_htable_t;

#endif /* TBX_HTABLE_H */
