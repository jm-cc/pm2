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
$Log: marcel_stdio.c,v $
Revision 1.2  2000/04/11 09:07:38  rnamyst
Merged the "reorganisation" development branch.

Revision 1.1.2.4  2000/03/31 08:08:12  rnamyst
Added disable_preemption() and enable_preemption().

Revision 1.1.2.3  2000/03/29 17:21:58  rnamyst
Minor cleaning of the code.

Revision 1.1.2.2  2000/03/29 16:49:42  vdanjean
ajout de du champs special_flags dans marcel_t

Revision 1.1.2.1  2000/03/29 14:24:55  rnamyst
Added the marcel_stdio.c that provides the marcel_printf functions.

______________________________________________________________________________
*/

#include "marcel.h"

#include <stdarg.h>

static marcel_lock_t __io_lock = MARCEL_LOCK_INIT;

static __inline__ void io_lock()
{
  disable_preemption();
  marcel_lock_acquire(&__io_lock);
}

static __inline__ void io_unlock()
{
  marcel_lock_release(&__io_lock);
  enable_preemption();
}

int marcel_printf(char *format, ...)
{
  static va_list args;
  int retour;

  io_lock();

  va_start(args, format);
  retour = vprintf(format, args);
  va_end(args);

  io_unlock();

  return retour;
}

int marcel_fprintf(FILE *stream, char *format, ...)
{
  static va_list args;
  int retour;

  io_lock();

  va_start(args, format);
  retour = vfprintf(stream, format, args);
  va_end(args);

  io_unlock();
  return retour;
}

