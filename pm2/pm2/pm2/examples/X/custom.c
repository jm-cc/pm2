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

#include <pm2.h>
#include "custom.h"
#include <ctype.h>
#include <stdlib.h>

int xtoi(char *p)
{
  int i = 0;
  char c;

  while (isxdigit(c = *p++))
    i = (i << 4) + c - (isdigit(c) ? '0' : (isupper(c) ? 'A' : 'a') - 10);

  return i;
}

extern marcel_key_t color_key;

extern void redessiner();

void user_func(int argc, char **argv)
{ int i;

   if(argc > 0) {
      if(!strcmp(argv[0], "-save")) {
         if(argc != 2) {
            mad_pack_str(MAD_IN_HEADER, "Wrong number of parameters");
         } else {
            marcel_t pids[64];
            int nb;

            pm2_freeze();
            pm2_threads_list(64, pids, &nb, MIGRATABLE_ONLY);
            BEGIN
               pm2_backup_group(pids, min(nb, 64), argv[1]);

               mad_pack_str(MAD_IN_HEADER, "Save operation completed");
            EXCEPTION
               WHEN(CONSTRAINT_ERROR)
                  mad_pack_str(MAD_IN_HEADER, "Error: Bad filename");
            END

            return;
         }
      } else if(!strcmp(argv[0], "-load")) {
         if(argc != 2) {
            mad_pack_str(MAD_IN_HEADER, "Wrong number of parameters");
         } else {
            BEGIN
               pm2_restore_group(argv[1]);

               mad_pack_str(MAD_IN_HEADER, "Load operation completed");
            EXCEPTION
               WHEN(CONSTRAINT_ERROR)
                  mad_pack_str(MAD_IN_HEADER, "Error: Bad filename");
            END
            return;
         }
      } else if(!strcmp(argv[0], "-enable_mig")) {
         if(argc < 2) {
            mad_pack_str(MAD_IN_HEADER, "Wrong number of parameters");
         } else {
            for(i=1; i<argc; i++)
               marcel_enablemigration((marcel_t)xtoi(argv[i]));
            return;
         }
      } else if(!strcmp(argv[0], "-disable_mig")) {
         if(argc < 2) {
            mad_pack_str(MAD_IN_HEADER, "Wrong number of parameters");
         } else {
            for(i=1; i<argc; i++)
               marcel_disablemigration((marcel_t)xtoi(argv[i]));
            return;
         }
      } else if(!strcmp(argv[0], "-setprio")) {
         if(argc < 3) {
            mad_pack_str(MAD_IN_HEADER, "Wrong number of parameters");
         } else {
            int prio = atoi(argv[1]);
            if(prio < 1 || prio > MAX_PRIO)
               mad_pack_str(MAD_IN_HEADER, "Bad priority value");
            else {
               for(i=2; i<argc; i++)
                  marcel_setprio((marcel_t)xtoi(argv[i]), prio);
               return;
            }
         }
      } else if(!strcmp(argv[0], "-colors")) {
         marcel_t pids[64];
         int i, nb;
         marcel_sem_t sem;

         marcel_sem_init(&sem, 0);
         lock_task();

         if(argc == 1) {
            pm2_threads_list(64, pids, &nb, MIGRATABLE_ONLY);
         } else {
            nb = 0;
            for(i=1; i<argc; i++)
               pids[nb++] = (marcel_t)xtoi(argv[i]);
         }

         for(i=0; i<min(nb, 64); i++) {
	   any_t color_string;
	   char buf[128];

	   color_string = pids[i]->key[color_key];

	   sprintf(buf, "Thread %lx : %s", (unsigned long)pids[i], (char *)color_string);
	   mad_pack_str(MAD_IN_HEADER, buf);
	 }

         unlock_task();

         return;
      } else if(!strcmp(argv[0], "-kill")) {
         marcel_t pids[64];
         int i, nb;

         lock_task();

         if(argc == 1) {
            pm2_threads_list(64, pids, &nb, MIGRATABLE_ONLY);
         } else {
            nb = 0;
            for(i=1; i<argc; i++)
               pids[nb++] = (marcel_t)xtoi(argv[i]);
         }

         for(i=0; i<min(nb, 64); i++) {
	   char buf[128];

	   marcel_cancel(pids[i]);

	   sprintf(buf, "Killed thread %lx", (unsigned long)pids[i]);
	   mad_pack_str(MAD_IN_HEADER, buf);
	 }
	 
	 redessiner();

         unlock_task();

         return;
      }
   }
   mad_pack_str(MAD_IN_HEADER, "Syntax : user [ -t task_id ] command");
   mad_pack_str(MAD_IN_HEADER, "command may be:");
   mad_pack_str(MAD_IN_HEADER, "\t-save <filename>");
   mad_pack_str(MAD_IN_HEADER, "\t-load <filename>");
   mad_pack_str(MAD_IN_HEADER, "\t-enable_mig <thread_id> { <thread_id> }");
   mad_pack_str(MAD_IN_HEADER, "\t-disable_mig <thread_id> { <thread_id> }");
   mad_pack_str(MAD_IN_HEADER, "\t-setprio <prio> <thread_id> { <thread_id> }");
   mad_pack_str(MAD_IN_HEADER, "\t-colors { <thread_id> }");
   mad_pack_str(MAD_IN_HEADER, "\t-kill { <thread_id> }");
}

