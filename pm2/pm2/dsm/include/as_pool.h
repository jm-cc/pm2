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

#ifndef DSM_AS_POOL_IS_DEF
#define DSM_AS_POOL_IS_DEF

#include "tbx.h"

/* define as usual by default */
#define AS_STACK_SIZE SIGSTKSZ /* private : do not used directly */
#define AS_NB_AREA 64          /* private : do not used directly */

extern p_tbx_memory_t as_head; /* private : do not used directly */

static void __inline__ dsm_as_free_stack(void *ptr) __attribute__ ((unused));
static void __inline__ dsm_as_free_stack(void *ptr) {
  /* used ptr return by setup_altstack */
  tbx_free(as_head, ptr);
}

static void  __inline__ dsm_as_clean()  __attribute__ ((unused));
static void  __inline__ dsm_as_clean() {
  tbx_malloc_clean(as_head);
}

void  dsm_as_init(); /* allocate memory and setup and altstack */
void* dsm_as_setup_altstack(); /* return old *real* base address */
void  dsm_as_check_altstack();

#endif

