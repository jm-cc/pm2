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
$Log: profile.c,v $
Revision 1.3  2000/09/14 20:50:34  rnamyst
Still some changes to the FUT stuff

Revision 1.2  2000/09/14 02:08:32  rnamyst
Put profile.h into common/include and added few FUT_SWITCH_TO calls

______________________________________________________________________________
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "profile.h"

#if !defined(PREPROC) && !defined(DEPEND)
#include "fut_entries.h"
#endif

#ifdef PM2
#include "pm2.h"
#endif

#ifdef MARCEL

#include "marcel.h"

#define PROF_THREAD_ID()  (marcel_self()->number)

#else // MARCEL

#define PROF_THREAD_ID()  0

#endif

#define PROF_BUFFER_SIZE  (1024*1024)

char PROF_FILE[1024];

void profile_init(void)
{
  static unsigned already_called = 0;

  if(!already_called) {
#ifdef PM2
    sprintf(PROF_FILE, "/tmp/prof_file_%d", pm2_self());
#else
    strcpy(PROF_FILE, "/tmp/prof_file");
#endif

    if(fut_setup(PROF_BUFFER_SIZE, 0, PROF_THREAD_ID()) < 0) {
      perror("fut_setup");
      exit(EXIT_FAILURE);
    }
    already_called = 1;
  }
}

void profile_activate(int how, unsigned keymask)
{
  fut_keychange(how, keymask, PROF_THREAD_ID());
}

void profile_stop(void)
{
  fut_endup(PROF_FILE);
}

