
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include "pm2.h"
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

extern void redessiner(void);

void user_func(int argc, char **argv)
{ int i;

   if(argc > 0) {
      if(!strcmp(argv[0], "-enable_mig")) {
         if(argc < 2) {
            old_mad_pack_str(MAD_IN_HEADER, "Wrong number of parameters");
         } else {
            for(i=1; i<argc; i++)
               marcel_enablemigration((marcel_t)xtoi(argv[i]));
            return;
         }
      } else if(!strcmp(argv[0], "-disable_mig")) {
         if(argc < 2) {
            old_mad_pack_str(MAD_IN_HEADER, "Wrong number of parameters");
         } else {
            for(i=1; i<argc; i++)
               marcel_disablemigration((marcel_t)xtoi(argv[i]));
            return;
         }
      } else if(!strcmp(argv[0], "-colors")) {
         marcel_t pids[64];
         int i, nb;
         marcel_sem_t sem;

         marcel_sem_init(&sem, 0);
	 marcel_freeze_sched();

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
	   old_mad_pack_str(MAD_IN_HEADER, buf);
	 }

	 marcel_unfreeze_sched();

         return;
      } else if(!strcmp(argv[0], "-kill")) {
         marcel_t pids[64];
         int i, nb;

         marcel_freeze_sched();

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
	   old_mad_pack_str(MAD_IN_HEADER, buf);
	 }
	 
	 redessiner();

         marcel_unfreeze_sched();

         return;
      }
   }
   old_mad_pack_str(MAD_IN_HEADER, "Syntax : user [ -t task_id ] command");
   old_mad_pack_str(MAD_IN_HEADER, "command may be:");
   old_mad_pack_str(MAD_IN_HEADER, "\t-save <filename>");
   old_mad_pack_str(MAD_IN_HEADER, "\t-load <filename>");
   old_mad_pack_str(MAD_IN_HEADER, "\t-enable_mig <thread_id> { <thread_id> }");
   old_mad_pack_str(MAD_IN_HEADER, "\t-disable_mig <thread_id> { <thread_id> }");
   old_mad_pack_str(MAD_IN_HEADER, "\t-setprio <prio> <thread_id> { <thread_id> }");
   old_mad_pack_str(MAD_IN_HEADER, "\t-colors { <thread_id> }");
   old_mad_pack_str(MAD_IN_HEADER, "\t-kill { <thread_id> }");
}

