
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

#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include "pm2.h"

#define DEBUG

#define MAX_THREADS 10
#define NB_SLOTS 3


typedef struct _item {
  int valeur;
  struct _item *suivant;
} item;

pm2_completion_t c;

void creer_liste_a_partir_de(void *debut)
{
  item *tableau = (item *)debut;

  tableau[0].valeur = 17;
  tableau[0].suivant = &tableau[13];

  tableau[13].valeur = 18;
  tableau[13].suivant = &tableau[5];

  tableau[5].valeur = 19;
  tableau[5].suivant = NULL;

}


void MIGRATION_ISOMALLOC_func()
{
  item *ptr ;
  int j,i = 0, k;
  item *lists[NB_SLOTS];    

  tfprintf(stderr, "thread starts...\n");
     /* allocate a few pages and write some data into them... */
     for (j = 0 ; j < NB_SLOTS ; j++)
     {
       tfprintf(stderr,"before pm2_malloc\n");
      ptr = lists[j] = (item *)pm2_isomalloc(6000);
      if (ptr == NULL) 
	return;
      tfprintf(stderr,"pm2_malloc returned %p\n", ptr);
      creer_liste_a_partir_de(ptr);
     }
     pm2_enable_migration();

     /* start never-ending loop */
     for(k = 0; k < 3 ; k++){
       for (j = 0 ; j < NB_SLOTS ; j++)
         {
         ptr = lists[j];
	 while(ptr != NULL) {
           marcel_delay(200);
	   tfprintf(stderr,"Element = %d  j=%d, i=%d\n", ptr->valeur, j, i++);
           ptr = ptr->suivant;
         }
       }
      }
     pm2_completion_signal(&c); 
}



int pm2_main(int argc, char **argv)
{
   unsigned nb;
   marcel_t pids[MAX_THREADS];

   pm2_init(argc, argv);

   if(pm2_self() == 0) { /* master process */

      tfprintf(stderr, "launching processing...\n");
      pm2_completion_init(&c, NULL, NULL);
      tfprintf(stderr, "launching thread...\n");
      pm2_thread_create(MIGRATION_ISOMALLOC_func, NULL);
      tfprintf(stderr, "waiting...\n");
      marcel_delay(2000);

      /* start  migration */
      tprintf("the module 0 will freeze!\n");
      pm2_freeze();
      pm2_threads_list(MAX_THREADS, pids, &nb, MIGRATABLE_ONLY);
      tprintf("migratable threads on module 0 : %d\n",nb);
      pm2_migrate(pids[0], 1);
      tprintf("migration ended\n");

      /* count remaining migratable threads */
      pm2_threads_list(MAX_THREADS, pids, &nb, MIGRATABLE_ONLY);
      tprintf("migratable threads on module 0 : %d\n",nb);
      pm2_completion_wait(&c);
//      marcel_delay(10000);
      pm2_halt();
      }
     else
     {
       nb = 0;
       while(nb == 0)
	 {
	   pm2_threads_list(MAX_THREADS, pids, &nb, MIGRATABLE_ONLY);      
	   marcel_delay(200);
	   tprintf("looping...\n");
	 }
       marcel_delay(2000);

      /* start  migration */
      tprintf("the module 1 will freeze!\n");
      pm2_freeze();
      pm2_threads_list(MAX_THREADS, pids, &nb, MIGRATABLE_ONLY);
      tprintf("migratable threads on module 1 : %d\n",nb);
      pm2_migrate(pids[0], 0);
      tprintf("migration ended\n");

      /* count remaining migratable threads */
      pm2_threads_list(MAX_THREADS, pids, &nb, MIGRATABLE_ONLY);
      tprintf("migratable threads on module 1 : %d\n",nb);
     }
   pm2_exit();
   tfprintf(stderr, "Main is ending\n");
   return 0;
}






