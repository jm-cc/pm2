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

#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#include "rpc_defs.h"
#include "timing.h"
/* #define DEBUG */

#define MAX_THREADS 10
#define NB_SLOTS 5


int *les_modules, nb_modules;
/*marcel_key_t user_key;*/

typedef struct _item {
  int valeur;
  struct _item *suivant;
} item;

 
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


BEGIN_SERVICE(PROCESSING)

item *ptr ;
int j,i = 0, k;
item *lists[NB_SLOTS];      
isomalloc_dataset_t descr;


     pm2_isomalloc_dataset_init(&descr);
     /* allocate a few pages and write some data into them... */
     for (j = 0 ; j < NB_SLOTS/3 ; j++)
     {
      ptr = lists[j] = (item *)pm2_isomalloc_dataset(&descr, 1000);
      if (ptr == NULL) 
	return;
      pm2_printf("pm2_isomalloc_dataset returned %p\n", ptr);
      creer_liste_a_partir_de(ptr);
     }
#ifdef DEBUG
     fprintf(stderr,"slots dans descr\n");
     slot_print_list(&descr.slot_descr);
#endif
     pm2_dataset_attach(&descr);
#ifdef DEBUG
     fprintf(stderr,"slots dans descr apres attach\n");
     slot_print_list(&descr.slot_descr);
     fprintf(stderr,"slots dans la liste du thread apres attach\n");
     slot_print_list(&((block_descr_t *)marcel_getspecific(_pm2_block_key))->slot_descr);
#endif
     for (j = NB_SLOTS/3 ; j < 2*NB_SLOTS/3 ; j++)
     {
      ptr = lists[j] = (item *)pm2_isomalloc(1000);
      if (ptr == NULL) 
	return;
      pm2_printf("pm2_isomalloc returned %p\n", ptr);
      creer_liste_a_partir_de(ptr);
     }
#ifdef DEBUG 
     fprintf(stderr, "liste des slots apres pm2_isomalloc\n");
     slot_print_list(&((block_descr_t *)marcel_getspecific(_pm2_block_key))->slot_descr);
#endif
     for (j = 2*NB_SLOTS/3 ; j < NB_SLOTS; j++)
     {
      ptr = lists[j] = (item *)pm2_isomalloc_dataset(&descr, 1000);
      if (ptr == NULL) 
	return;
      pm2_printf("pm2_isomalloc_dataset returned %p\n", ptr);
      creer_liste_a_partir_de(ptr);
     }
     pm2_dataset_attach(&descr);
#ifdef DEBUG
     fprintf(stderr,"slots dans la liste du thread apres attach\n");
     slot_print_list(&((block_descr_t *)marcel_getspecific(_pm2_block_key))->slot_descr);
#endif
     pm2_enable_migration();
     /* start never-ending loop */
     for(k = 0; k < 3 ; k++){
       for (j = 0 ; j < NB_SLOTS ; j++)
         {
         ptr = lists[j];
	 while(ptr != NULL) {
           marcel_delay(200);
	   pm2_printf("Element = %d  j=%d, i=%d\n", ptr->valeur, j, i++);
           ptr = ptr->suivant;
         }
       }
      }
 
END_SERVICE(PROCESSING)


int pm2_main(int argc, char **argv)
{
   unsigned nb;
   marcel_t pids[MAX_THREADS];

   pm2_init_rpc();

   DECLARE_LRPC(PROCESSING);

   pm2_init(&argc, argv, 2, &les_modules, &nb_modules);

   if(pm2_self() == les_modules[0]) { /* master process */

      pm2_printf("launching processing...\n");
      ASYNC_LRPC(les_modules[0], PROCESSING, STD_PRIO,DEFAULT_STACK, NULL);
      marcel_delay(2000);

      /* start  migration */
      pm2_printf("the module 0 will freeze!\n");
      pm2_freeze();
      pm2_threads_list(MAX_THREADS, pids, &nb, MIGRATABLE_ONLY);
      pm2_printf("migratable threads on module 0 : %d\n",nb);
      pm2_migrate(pids[0], les_modules[1]);
      pm2_printf("migration ended\n");

      /* count remaining migratable threads */
      pm2_threads_list(MAX_THREADS, pids, &nb, MIGRATABLE_ONLY);
      pm2_printf("migratable threads on module 0 : %d\n",nb);
      marcel_delay(3000);
      pm2_kill_modules(les_modules, nb_modules);
      }
     else
     {
       nb = 0;
       while(nb == 0)
	 {
	   pm2_threads_list(MAX_THREADS, pids, &nb, MIGRATABLE_ONLY);      
	   marcel_delay(200);
	 }
       marcel_delay(1000);

      /* start  migration */
      pm2_printf("the module 1 will freeze!\n");
      pm2_freeze();
      pm2_threads_list(MAX_THREADS, pids, &nb, MIGRATABLE_ONLY);
      pm2_printf("migratable threads on module 1 : %d\n",nb);
      pm2_migrate(pids[0], les_modules[0]);
      pm2_printf("migration ended\n");

      /* count remaining migratable threads */
      pm2_threads_list(MAX_THREADS, pids, &nb, MIGRATABLE_ONLY);
      pm2_printf("migratable threads on module 1 : %d\n",nb);
      /*      marcel_delay(1000);*/
     }
   pm2_exit();
   tfprintf(stderr, "Main is ending\n");
   return 0;
}






