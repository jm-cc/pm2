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
$Log: safe_malloc.h,v $
Revision 1.2  2000/01/31 15:49:49  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

#ifndef SAFE_MALLOC_EST_DEF
#define SAFE_MALLOC_EST_DEF

#include <stdlib.h>
#ifndef FREEBSD_SYS
#include <malloc.h>
#endif

#ifdef USE_SAFE_MALLOC

#define MALLOC(x)      safe_malloc((x), __FILE__, __LINE__)

#define CALLOC(x, y)   safe_calloc((x), (y), __FILE__, __LINE__)

#define REALLOC(x, y)  safe_realloc((x), (y), __FILE__, __LINE__)

#define FREE(x)        safe_free((x), __FILE__, __LINE__)

#else

#define MALLOC(x)      malloc(x)

#define CALLOC(x, y)   calloc((x), (y))

#define REALLOC(x, y)  realloc((x), (y))

#define FREE(x)        free(x)

#endif

void safe_malloc_init();

void *safe_malloc(size_t size, char *file, unsigned line);

void *safe_calloc(size_t nmemb, size_t size, char *file, unsigned line);

void *safe_realloc(void *ptr, size_t size, char *file, unsigned line);

void safe_free(void *ptr, char *file, unsigned line);

enum {
  SAFE_MALLOC_ERRORS_ONLY,
  SAFE_MALLOC_VERBOSE
};

void safe_malloc_check(int mode);

#endif
