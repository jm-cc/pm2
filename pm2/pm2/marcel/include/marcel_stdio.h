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
$Log: marcel_stdio.h,v $
Revision 1.2  2000/04/11 09:07:13  rnamyst
Merged the "reorganisation" development branch.

Revision 1.1.2.1  2000/03/29 14:23:47  rnamyst
Added the marcel_stdio.h that provides the marcel_printf functions...

______________________________________________________________________________
*/

#ifndef MARCEL_STDIO_EST_DEF
#define MARCEL_STDIO_EST_DEF

#include <stdio.h>
#include <sys/time.h>

// For compatibility purposes :
#define tprintf  marcel_printf
#define tfprintf marcel_fprintf

int marcel_printf(char *format, ...);
int marcel_fprintf(FILE *stream, char *format, ...);

// Still here, but do not use it!
int tselect(int width, fd_set *readfds, fd_set *writefds, fd_set *exceptfds);

#endif
