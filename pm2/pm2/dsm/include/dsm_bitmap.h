/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 2.0

             Gabriel Antoniu, Luc Bouge, Christian Perez,
                Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 8512 CNRS-INRIA
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


#ifndef DSM_BITMAP_IS_DEF
#define DSM_BITMAP_IS_DEF

#include "sys/bitmap.h"
#include "marcel.h" // tmalloc

typedef unsigned int* dsm_bitmap_t;

/* For the following function, size is in bits */

#define dsm_bitmap_alloc(size) \
  ((dsm_bitmap_t) tcalloc(((size) % 8) ? ((size) / 8 + 1) : ((size) / 8), 1))
  
#define dsm_bitmap_free(b) tfree(b)

#define dsm_bitmap_mark_dirty(offset, length, bitmap) set_bits_to_1(offset, length, bitmap)

#define dsm_bitmap_is_empty(b, size) bitmap_is_empty(b, size)

#define dsm_bitmap_clear(b,size) clear_bitmap(b, size)

#endif
